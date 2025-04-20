#include <iostream>
#include <algorithm>
#include <sstream>
#include <thread> // Для std::this_thread::sleep_for
#include <chrono> // Для std::chrono::milliseconds

//#include "irccom.h"
#include "socket.h"
#include "ircbot.h"
#include "handler.h"

std::vector<std::string> splitStrBySep(std::string const& text, char sep)
{
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos)
    {
        tokens.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    tokens.push_back(text.substr(start));
    return tokens;
}

std::vector<std::string> splitStrBySpc(const std::string& input) {
    std::vector<std::string> result; // Вектор для хранения подстрок
    std::istringstream stream(input); // Поток для чтения строки
    std::string word;
    // Читаем слова из потока, используя пробел как разделитель
    while (stream >> word) {
        result.push_back(word); // Добавляем каждое слово в вектор
    }
    return result;
}

bool IRCBot::InitSocket()
{
    return _socket.Init();
}

bool IRCBot::Connect(const char* host, int port)
{
    return _socket.Connect(host, port);
}

void IRCBot::Disconnect()
{
    _socket.Disconnect();
}

bool IRCBot::SendIRC(std::string data)
{
    data.append("\n");
    return _socket.SendData(data.c_str());
}

bool IRCBot::Login(std::string nick, std::string user, std::string pass, std::string rnam)
{
    _nick = nick;
    _user = user;

    if (SendIRC("HELLO"))
    {
        if (!pass.empty() && !SendIRC("PASS " + user + ':' + pass))
            return false;
        if (SendIRC("NICK " + nick))
            if (SendIRC("USER " + user + " 8 * :" + rnam))
                return true;
    }

    return false;
}

void IRCBot::ReceiveData()
{
    std::string buffer = _socket.ReceiveData();
    std::string line;
    std::istringstream iss(buffer);
    while(getline(iss, line))
    {
        if (line.find("\r") != std::string::npos) {
            line = line.substr(0, line.size() - 1);
        }
        //std::replace(buffer.begin(), buffer.end(), '\r', '\n');
        Parse(line);
    }
}

void IRCBot::Parse(std::string data)
{
    std::string original(data);
    IRCCommandPrefix cmdPrefix;
    static int pongCount = 0;

    // if command has prefix
    if (data.substr(0, 1) == ":")
    {
        cmdPrefix.Parse(data);
        data = data.substr(data.find(" ") + 1);
    }

    std::string command = data.substr(0, data.find(" "));
    std::transform(command.begin(), command.end(), command.begin(), ::towupper);
    if (data.find(" ") != std::string::npos)
        data = data.substr(data.find(" ") + 1);
    else
        data = "";

    std::vector<std::string> parts;

    if (data != "")
    {
        if (data.substr(0, 1) == ":")
            parts.push_back(data.substr(1));
        else
        {
            size_t pos1 = 0, pos2;
            while ((pos2 = data.find(" ", pos1)) != std::string::npos)
            {
                parts.push_back(data.substr(pos1, pos2 - pos1));
                pos1 = pos2 + 1;
                if (data.substr(pos1, 1) == ":")
                {
                    parts.push_back(data.substr(pos1 + 1));
                    break;
                }
            }
            if (parts.empty())
                parts.push_back(data);
        }
    }

    if (command == "ERROR")
    {
        std::cout << original << std::endl;
        Disconnect();
        return;
    }

    if (command == "PRIVMSG") {
        // std::cout << "DATA string on PRIVMSG:" << data << std::endl;
    }

    if (command == "PING")
    {
        //std::cout << "Ping? Pong! - " << getDateVal(0) << std::endl;
        SendIRC("PONG :" + parts.at(0));
        //std::cout << getDateVal(4) << " Sent: PONG :" + parts.at(0) + '\n';
        pongCount++;
        if (pongCount >= 10) {
            std::cout << "[pong!] to " << parts.at(0) << " sent " << pongCount << " times for now - " << getDateVal(4) << '\r';
            pongCount = 0;
        }
        return;
    }

    IRCMessage ircMessage(command, cmdPrefix, parts);

    // Default handler
    int commandIndex = GetCommandHandler(command);
    if (commandIndex < NUM_IRC_CMDS)
    {
        IRCCommandHandler& cmdHandler = ircCommandTable[commandIndex];
        (this->*cmdHandler.handler)(ircMessage);
    }
    else if (_debug)
        std::cout << original << std::endl;

    // Try to call hook (if any matches)
    CallHook(command, ircMessage);
}

void IRCBot::HookIRCCommand(std::string command, void (*function)(IRCMessage /*message*/, IRCBot* /*client*/))
{
    IRCCommandHook hook;

    hook.command = command;
    hook.function = function;

    _hooks.push_back(hook);
}

void IRCBot::CallHook(std::string command, IRCMessage message)
{
    if (_hooks.empty())
        return;

    for (std::list<IRCCommandHook>::const_iterator itr = _hooks.begin(); itr != _hooks.end(); ++itr)
    {
        if (itr->command == command)
        {
            (*(itr->function))(message, this);
            break;
        }
    }
}

void onPrivMsg(IRCMessage message, IRCBot* client)
{

    std::string text;
    if (message.parts.at(message.parts.size() - 1)[0] != IRCBot::commsymbol[0]) {
        return;
    } else {
        text = message.parts.at(message.parts.size() - 1).substr(1);
    }
    
    std::vector<std::string> botReplyMsg = botReply(text, message, client);

    for (size_t i = 0; i < botReplyMsg.size(); i++) {
        if (message.parts.at(message.parts.size() - 2)[0] == '#') {
            replyChan(botReplyMsg[i], message, client);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        else {
            replyNick(botReplyMsg[i], message, client);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void replyChan(std::string msgChan, IRCMessage message, IRCBot* client) {
    client->SendIRC("PRIVMSG " + message.parts.at(0) + " :" + msgChan);
}

void replyNick(std::string msgNick, IRCMessage message, IRCBot* client) {
    client->SendIRC("PRIVMSG " + message.prefix.nick + " :" + msgNick);
}

std::vector<std::string> botReply(const std::string text, IRCMessage message, IRCBot* client) {
    std::vector<std::string> commSet = splitStrBySpc(text);
    int execCase = 0;
    std::string reply;
    std::cout << "Command received: " << commSet[0] << '\n';
    if (commSet.size() > 1) {
        for (size_t i = 1; i < commSet.size(); i++) {
            std::cout << "Command argument: " << i << ") " << commSet[i] << '\n';
        }
    }
    

    for (size_t i = 1; i < client->icmd.size(); i++) {
        if (commSet[0] == client->icmd[i].first) {
            execCase = i;
            break;
        }
    }
    switch (execCase) {
        case 0: {
            reply += "Error! Command \"" + text + "\" not understood";
            break;
        }

        case 1:
        {
            if (commSet.size() == 1) {
                reply += "Avaliable commans are: ";
                for (size_t i = 1; i < client->icmd.size(); i++) {
                    if (i == client->icmd.size() - 1) {
                        reply += client->icmd[i].first + '.';
                    }
                    else {
                        reply += client->icmd[i].first + ", ";
                    }

                }
                reply += " Type .help command for more";
            }
            else {
                bool command_found = false;
                for (size_t i = 1; i < client->icmd.size(); i++) {
                    if (commSet[1] == client->icmd[i].first) {
                        reply += client->icmd[i].first + ": ";
                        reply += client->icmd[i].second;
                        command_found = true;
                        break;
                    }
                }
                if (!command_found) {
                    reply += "No help avaliable for " + commSet[1];
                }
            }
            
            break;
        }

        case 2: {
            reply += "Hello! I'm a bot, written in C++";
            break;
        }

        case 3: {
            if (message.prefix.nick != IRCBot::botadmnick) {
                reply += "Not my admin!";
            } else {
                client->SendIRC("QUIT :Quit command received from " + client->botadmnick);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                client->Disconnect();
            }
            break;
        }

        case 4: {
            reply += getDateVal(4);
            break;
        }

        case 5: {
            reply += getDateVal(0);
            break;
        }

        case 6: {
            reply += getTimeRun(client->startTime);
            break;
        }

        case 7: {
            reply += "Bot admin is " + client->botadmnick;
            break;
        }

        case 8: {
            if (commSet.size() == 1) {
                reply += message.prefix.nick + ", your host is: " + message.prefix.host;
            }
            else {
                if (!client->ipInfoToken.empty()) {
                    std::vector<std::string>ipSetStr = getIpAddr(commSet[1]);
                    if (!ipSetStr.empty()) {
                        for (size_t i = 0; i < ipSetStr.size(); i++) {
                            if (i > 0) {
                                reply += '\n' + getIpInfo(ipSetStr[i], client->ipInfoToken);
                            }
                            else {
                                reply += getIpInfo(ipSetStr[i], client->ipInfoToken);
                            }
                        }
                    }
                    else {
                        reply += std::string("\x02\x03") + "04Error! Name or address not understood" + "\x03";
                    }
                }
                else {
                    reply += std::string("\x02\x03") + "04Token for ipinfo.io not specified, function doesn't work" + "\x03";
                }
                
            }
            
            break;
        }

        case 9: {
            reply += message.prefix.nick + ' ';
            if (!client->ipInfoToken.empty()) {
                std::vector<std::string> ipSetStr = getIpAddr(message.prefix.host);
                if (!ipSetStr.empty()) {
                    for (size_t i = 0; i < ipSetStr.size(); i++) {
                        reply += getIpInfo(ipSetStr[i], client->ipInfoToken);
                    }
                }
                else {
                    reply += std::string("\x02\x03") + "04Error! Name or address not understood" + "\x03";
                }
            }
            else {
                reply += std::string("\x02\x03") + "04Token for ipinfo.io not specified, function doesn't work" + "\x03";
            }
            
            break;
        }

        case 10: {
            reply += "Memory usage: " + umemStat() + "kB (RSS)";
            break;
        }

        case 11: {
            reply += message.parts.at(0);
            break;
        }
        
    }
    return splitStrBySep(reply, '\n');
}

std::string getTimeRun(time_t initialTime) {
    time_t currentTime = time(nullptr);
    time_t uprunTime = currentTime - initialTime;
    std::string upTime;
    int up_dd = uprunTime / 86400;
    int up_hh = (uprunTime / 3600) % 24;
    int up_mm = (uprunTime / 60 ) % 60;
    int up_ss = uprunTime % 60;

    if (up_dd > 0) {
        upTime += std::to_string(up_dd) + " d ";
    }

    if (up_hh < 10) {
        upTime += '0';
    }
    upTime += std::to_string(up_hh) + ':';

    if (up_mm < 10) {
        upTime += '0';
    }
    upTime += std::to_string(up_mm) + ':';

    if (up_ss < 10) {
        upTime += '0';
    }
    upTime += std::to_string(up_ss);
    
    return upTime;
}

std::string getDateVal(int vt) {
    time_t tnow = time(nullptr);
    struct tm *timeinfo;
    time(&tnow);
    timeinfo = localtime(&tnow);
    char tmc[64];
    switch(vt)
    {
  case 0:
    strftime(tmc, 64, "%T", timeinfo); // ISO8601 (HH:MM:SS)
    break;
  case 1:
    strftime(tmc, 64, "%R", timeinfo); // HH:MM
    break;
  case 2:
    strftime(tmc, 64, "%a %d %h", timeinfo); // Thu 23 Aug
    break;
  case 3:
    strftime(tmc, 64, "%d.%m.%g", timeinfo); // 23.08.24
    break;
  case 4:
    strftime(tmc, 64, "%c", timeinfo); // Thu Aug 23 14:55:02 2001
    break;
  }
  return tmc;
}

std::string umemStat() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage); // Вывод Max RSS (в кб)
    std::string umemMrss = std::to_string(usage.ru_maxrss);
    std::cout << "Requested max RSS: " << umemMrss << " KB" << std::endl;
    return umemMrss;
}

time_t IRCBot::startTime;           // Bot startup time
std::string IRCBot::botchannel;     // Bot initial channel
std::string IRCBot::botadmnick;     // Bot admin nick
std::string IRCBot::commsymbol;     // command start symbol
std::string IRCBot::runonlogin;     // run this on login
std::string IRCBot::nspassword;     // NickServ password
std::string IRCBot::botctcpver;     // Bot CTCP version
std::string IRCBot::ipInfoToken;    // Token at ipinfo.io


