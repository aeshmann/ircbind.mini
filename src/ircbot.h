#ifndef IRCBOT_H_
#define IRCBOT_H_

#include <string>
#include <vector>
#include <list>
#include <sys/resource.h>
#include "socket.h"


class IRCBot;

extern std::vector<std::string> splitStrBySep(std::string const&, char);

class IRCCommandPrefix
{
public:
    
    std::string prefix;     // prefix nick!user@host
    std::string nick;       // nick name
    std::string user;       // user name
    std::string host;       // host name

    void Parse(std::string data)
    {
        if (data == "")
            return;

        prefix = data.substr(1, data.find(" ") - 1);
        std::vector<std::string> tokens;

        if (prefix.find("@") != std::string::npos)
        {
            tokens = splitStrBySep(prefix, '@');
            nick = tokens.at(0);
            host = tokens.at(1);
        }
        if (nick != "" && nick.find("!") != std::string::npos)
        {
            tokens = splitStrBySep(nick, '!');
            nick = tokens.at(0);
            user = tokens.at(1);
        }
    };
};

struct IRCMessage
{
    IRCMessage();
    IRCMessage(std::string cmd, IRCCommandPrefix p, std::vector<std::string> params) :
        command(cmd), prefix(p), parts(params) {};

    std::string command;
    IRCCommandPrefix prefix;
    std::vector<std::string> parts;
};

struct IRCCommandHook
{
    IRCCommandHook() : function(NULL) {};

    std::string command;
    void (*function)(IRCMessage /*message*/, IRCBot* /*client*/);
};

class IRCBot
{
public:
    IRCBot() : _debug(false) {};

    bool InitSocket();
    bool Connect(const char* /*host*/, int /*port*/);
    void Disconnect();
    bool Connected() { return _socket.Connected(); };
    bool SendIRC(std::string /*data*/);
    bool Login(std::string /*nick*/, std::string /*user*/, std::string /*password*/, std::string /*realname*/);
    void ReceiveData();
    void HookIRCCommand(std::string /*command*/, void (*function)(IRCMessage /*message*/, IRCBot* /*client*/));
    void Parse(std::string /*data*/);
    void HandleCTCP(IRCMessage /*message*/);

    // Default internal handlers
    void HandlePrivMsg(IRCMessage /*message*/);
    void HandleNotice(IRCMessage /*message*/);
    void HandleChannelJoinPart(IRCMessage /*message*/);
    void HandleUserNickChange(IRCMessage /*message*/);
    void HandleUserQuit(IRCMessage /*message*/);
    void HandleChannelNamesList(IRCMessage /*message*/);
    void HandleNicknameInUse(IRCMessage /*message*/);
    void HandleServerMessage(IRCMessage /*message*/);
    void HandleEndOfNames(IRCMessage /*message*/);
    void HandleStartOfMOTD(IRCMessage /*message*/);
    void HandleMOTDText(IRCMessage /*message*/);
    void HandleEndOfMOTD(IRCMessage /*message*/);
    void HandleMissingMOTD(IRCMessage /*message*/);
    void HandleAwayMsgTooLong(IRCMessage /*message*/);

    void Debug(bool debug) { _debug = debug; };

    static std::string botchannel;    // Bot initial channel
    static std::string nspassword;    // NickServ password
    static std::string runonlogin;    // Run on connect commands
    static std::string botadmnick;    // Bot admin nickname
    static std::string commsymbol;    // Command first symbol
    static std::string botctcpver;    // Bot CTCP version reply
    static time_t startTime;    	  // Bot startup time

    static std::string ipInfoToken;

    std::vector<std::pair<std::string, std::string>> icmd = {
        {"err", "reserved for err message"      },  // 0
        {"help", "returns command list"         },  // 1
        {"helo", "Greets you"                   },  // 2
        {"quit", "Quit command"                 },  // 3
        {"date", "Returns current date, time"   },  // 4
        {"time", "Returns current time"         },  // 5
        {"uptm", "Shows bot uptime"             },  // 6
        {"admi", "Shows, who is bot admin"      },  // 7
        {"host", "Shows host information"       },  // 8
        {"myip", "Shows your ip information"    },  // 9
        {"rmem", "RAM max resident set size"    },  // 10
        {"chan", "Shows current channel"        }   // 11
    };

private:
    void HandleCommand(IRCMessage /*message*/);
    void CallHook(std::string /*command*/, IRCMessage /*message*/);

    IRCSocket _socket;

    std::list<IRCCommandHook> _hooks;

    std::string _nick;
    std::string _user;

    bool _debug;
};

void onPrivMsg(IRCMessage message, IRCBot* client);

std::vector<std::string> botReply(std::string, IRCMessage, IRCBot*);
void replyChan(std::string, IRCMessage, IRCBot*);
void replyNick(std::string, IRCMessage, IRCBot*);

std::string getTimeRun(time_t);
std::string getDateVal(int);
std::string umemStat();

std::string getIpInfo(std::string ipAddrStr, std::string ipinfo_token);
std::vector<std::string> getIpAddr(const std::string& hostname);
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

#endif
