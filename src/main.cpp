#include <iostream>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <exception>
#include <algorithm>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <signal.h>
#include <memory> // Для std::shared_ptr

#include "cpptoml.h" // Подключение библиотеки cpptoml
#include "thread.h"
#include "ircbot.h"

volatile bool running;

struct IRCConfig {
    struct Server {
        std::string bothostname;    // Bot host name
        int bothostport;            // Bot host port
        std::string bothostpass;    // Bot host pass
    } serverconf;

    struct Client {
        std::string username;   // Bot usernane
        std::string nickname;   // Bot nickname
        std::string realname;   // Bot realname
        std::string nspasswd;   // NickServ password
        std::string botschan;   // Bot channel
        std::string adminick;   // Bot admin nick
        std::string runatcon;   // Any command sent upon connection
        std::string xdccvers;
        bool connect_runbot;    // Connect at launch
        char command_symbol;    // Bot command symbol
    } clientconf;

    struct Feature
    {
        std::string ipinftkn;
    } featureconf;
    
};

// Функция для парсинга TOML-файла
IRCConfig parseTomlFile(const std::string& filename) {
    IRCConfig config;

    try {
        // Загрузка TOML-файла
        auto table = cpptoml::parse_file(filename);

        // Секция [ircServer]
        const auto& ircServer = table->get_table("ircServer");
        config.serverconf.bothostname = *ircServer->get_as<std::string>("ircServerHost");
        config.serverconf.bothostport = *ircServer->get_as<int>("ircServerPort");
        config.serverconf.bothostpass = *ircServer->get_as<std::string>("ircServerPass");

        // Секция [ircClient]
        const auto& ircClient = table->get_table("ircClient");
        config.clientconf.username = *ircClient->get_as<std::string>("ircBotUser");
        config.clientconf.nickname = *ircClient->get_as<std::string>("ircBotNick");
        config.clientconf.realname = *ircClient->get_as<std::string>("ircBotRnam");
        config.clientconf.nspasswd = *ircClient->get_as<std::string>("ircBotNspw");
        config.clientconf.botschan = *ircClient->get_as<std::string>("ircBotChan");
        config.clientconf.adminick = *ircClient->get_as<std::string>("ircBotAdmi");
        config.clientconf.runatcon = *ircClient->get_as<std::string>("ircBotRcon");
        config.clientconf.xdccvers = *ircClient->get_as<std::string>("ircBotDccv");
        config.clientconf.connect_runbot = *ircClient->get_as<bool>("ircBotAcon");

        // Командный символ читается как строка длиной в один символ
        std::string commandSymbolStr = *ircClient->get_as<std::string>("ircBotCsym");
        if (commandSymbolStr.length() == 1) {
            config.clientconf.command_symbol = commandSymbolStr[0];
        } else {
            throw std::runtime_error("ircBotCsym must be a single character.");
        }

        // Секция [botComset] - параметры дополнительных команд бота
        auto botComset = table->get_table("botComset");

        if (botComset)
        {
            auto ipinftkn = botComset->get_as<std::string>("ipInfToken");
            if (ipinftkn)
            {
                config.featureconf.ipinftkn = *ipinftkn;
            }
            else
            {
                std::cerr << "[botComset] exists, but 'ipInfToken' is missing or not a string." << std::endl;
                // Обработка ошибки или установка значения по умолчанию
            }
        }
        else
        {
            std::cerr << "[botComset] section is missing in the TOML file." << std::endl;
            // Здесь можно использовать значения по умолчанию или завершить программу
        }

    } catch (const cpptoml::parse_exception& e) {
        std::cerr << "TOML parsing error: " << e.what() << "\n";
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing TOML file: " << e.what() << "\n";
        throw;
    }

    return config;
}

// Функция для вывода конфигурации
void printConfig(const IRCConfig& config) {
    std::cout << "IRC Server Configuration:\n";
    std::cout << "Host: " << config.serverconf.bothostname << "\n";
    std::cout << "Port: " << config.serverconf.bothostport << "\n";
    std::cout << "Password: " << config.serverconf.bothostpass << "\n\n";

    std::cout << "IRC Client Configuration:\n";
    std::cout << "Username: " << config.clientconf.username << "\n";
    std::cout << "Nickname: " << config.clientconf.nickname << "\n";
    std::cout << "Realname: " << config.clientconf.realname << "\n";
    std::cout << "NickServ Password: " << config.clientconf.nspasswd << "\n";
    std::cout << "On IRC connect run: " << config.clientconf.runatcon << "\n";
    std::cout << "Channel: " << config.clientconf.botschan << "\n";
    std::cout << "Admin Nick: " << config.clientconf.adminick << "\n";
    std::cout << "CTCP version: " << config.clientconf.xdccvers << "\n";
    std::cout << "Auto Connect: " << (config.clientconf.connect_runbot ? "true" : "false") << "\n";
    std::cout << "Command Symbol: '" << config.clientconf.command_symbol << "'\n";

    std::cout << "Bot features:\n";
    std::cout << "IP info token: " << config.featureconf.ipinftkn << "\n";
}

void signalHandler(int signal)
{
    running = false;
}

class ConsoleCommandHandler
{
public:
    bool AddCommand(std::string name, int argCount, void (*handler)(std::string /*params*/, IRCBot* /*client*/))
    {
        CommandEntry entry;
        entry.argCount = argCount;
        entry.handler = handler;
        std::transform(name.begin(), name.end(), name.begin(), towlower);
        _commands.insert(std::pair<std::string, CommandEntry>(name, entry));
        return true;
    }

    void ParseCommand(std::string command, IRCBot* client)
    {
        if (_commands.empty())
        {
            std::cout << "No commands available." << std::endl;
            return;
        }

        if (command[0] == '/')
            command = command.substr(1); // Remove the slash

        std::string name = command.substr(0, command.find(" "));
        std::string args = command.substr(command.find(" ") + 1);
        int argCount = std::count(args.begin(), args.end(), ' ');

        std::transform(name.begin(), name.end(), name.begin(), towlower);

        std::map<std::string, CommandEntry>::const_iterator itr = _commands.find(name);
        if (itr == _commands.end())
        {
            std::cout << "Command not found." << std::endl;
            return;
        }

        if (++argCount < itr->second.argCount)
        {
            std::cout << "Insuficient arguments." << std::endl;
            return;
        }

        (*(itr->second.handler))(args, client);
    }

private:
    struct CommandEntry
    {
        int argCount;
        void (*handler)(std::string /*arguments*/, IRCBot* /*client*/);
    };

    std::map<std::string, CommandEntry> _commands;
};

ConsoleCommandHandler commandHandler;

void msgCommand(std::string arguments, IRCBot* client)
{
    std::string to = arguments.substr(0, arguments.find(" "));
    std::string text = arguments.substr(arguments.find(" ") + 1);

    std::cout << "To " + to + ": " + text << std::endl;
    client->SendIRC("PRIVMSG " + to + " :" + text);
};

void joinCommand(std::string channel, IRCBot* client)
{
    if (channel[0] != '#')
        channel = "#" + channel;

    client->SendIRC("JOIN " + channel);
}

void partCommand(std::string channel, IRCBot* client)
{
    if (channel[0] != '#')
        channel = "#" + channel;

    client->SendIRC("PART " + channel);
}

void ctcpCommand(std::string arguments, IRCBot* client)
{
    std::string to = arguments.substr(0, arguments.find(" "));
    std::string text = arguments.substr(arguments.find(" ") + 1);

    std::transform(text.begin(), text.end(), text.begin(), towupper);

    client->SendIRC("PRIVMSG " + to + " :\001" + text + "\001");
}

ThreadReturn inputThread(void* client)
{
    std::string command;

    commandHandler.AddCommand("msg", 2, &msgCommand);
    commandHandler.AddCommand("join", 1, &joinCommand);
    commandHandler.AddCommand("part", 1, &partCommand);
    commandHandler.AddCommand("ctcp", 2, &ctcpCommand);

    while(true)
    {
        getline(std::cin, command);
        if (command == "")
            continue;

        if (command[0] == '/')
            commandHandler.ParseCommand(command, (IRCBot*)client);
        else
            ((IRCBot*)client)->SendIRC(command);

        if (command == "quit")
            break;
    }

    pthread_exit(NULL);
}


int main(int argc, char* argv[]) {

    IRCConfig config;

    try {
        // Определение имени конфигурационного файла
        std::string filename = "config.toml";  // значение по умолчанию
        if (argc > 1) {
            filename = argv[1];
        }

        // Проверка существования файла
        if (!std::filesystem::exists(filename)) {
            std::cerr << "Config file " << filename << " not found! Please specify config file in arguments.\n";
            return 1; // Завершение программы с кодом ошибки
        }

        std::cout << "Using config file \"" + filename + "\":\n" << std::endl;

        // Парсинг файла
        config = parseTomlFile(filename);

        // Вывод конфигурации
        printConfig(config);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << "\n";
        return 1;
    }

    if (!config.clientconf.connect_runbot)
    {
        std::cout << "Is this correct? Y/n:";
        std::string input;
        std::getline(std::cin, input);
        if (input.empty() || input[0] == 'y' || input[0] == 'Y')
        {
            std::cout << "Starting IRC client\n";
        }
        else
        {
            std::cout << "Program will close\n";
            return 0;
        }
    }

    IRCBot client;

    client.startTime = time(nullptr);

    if (!config.clientconf.botschan.empty()) {
        std::cout << "CH value assigned: " << config.clientconf.botschan << std::endl;
        client.botchannel = config.clientconf.botschan;
    }

    if (!config.clientconf.nspasswd.empty()) {
        std::cout << "NS pass var assigned: " << config.clientconf.nspasswd << std::endl;
        client.nspassword = config.clientconf.nspasswd;
    }

    if (!config.clientconf.adminick.empty()) {
        std::cout << "Bot admin nick: " << config.clientconf.adminick << std::endl;
        client.botadmnick = config.clientconf.adminick;
    }

    if (!config.clientconf.xdccvers.empty()) {
        std::cout << "Bot DCC version: " << config.clientconf.xdccvers << std::endl;
        client.botctcpver = config.clientconf.xdccvers;
    }

    if (config.clientconf.command_symbol != 0) {
        std::cout << "Bot command symbol: " << config.clientconf.command_symbol << std::endl;
        client.commsymbol.assign(1, config.clientconf.command_symbol);
    }

    if (!config.clientconf.runatcon.empty()) {
        std::cout << "Run on connect: " << config.clientconf.runatcon << std::endl;
        client.runonlogin = config.clientconf.runatcon;
    }

    if (!config.featureconf.ipinftkn.empty()) {
        std::cout << "IP info token: " << config.featureconf.ipinftkn << std::endl;
        client.ipInfoToken = config.featureconf.ipinftkn;
    }

     // Hook PRIVMSG
    client.HookIRCCommand("PRIVMSG", &onPrivMsg);

    client.Debug(true);

    // Start the input thread
    Thread thread;
    thread.Start(&inputThread, &client);

    if (client.InitSocket())
    {
        std::cout << "[->] Socket initialized. Connecting..." << std::endl;

        if (client.Connect(config.serverconf.bothostname.c_str(), config.serverconf.bothostport))
        {
            std::cout << "[>>] Connected. Loggin in..." << std::endl;

            if (client.Login(config.clientconf.nickname, config.clientconf.username, config.serverconf.bothostpass, config.clientconf.realname))
            {
                std::cout << "[+] Login completed." << std::endl;
                running = true;
                signal(SIGINT, signalHandler);
                while (client.Connected() && running) {
                    client.ReceiveData();
                }
            }

            if (client.Connected()) {
                client.Disconnect();
            }
            
            std::cout << "[-] Disconnected." << std::endl;
        }
    }
    return 0;
}