#include "handler.h"

IRCCommandHandler ircCommandTable[NUM_IRC_CMDS] =
{
    { "PRIVMSG",            &IRCBot::HandlePrivMsg                   },
    { "NOTICE",             &IRCBot::HandleNotice                    },
    { "JOIN",               &IRCBot::HandleChannelJoinPart           },
    { "PART",               &IRCBot::HandleChannelJoinPart           },
    { "NICK",               &IRCBot::HandleUserNickChange            },
    { "QUIT",               &IRCBot::HandleUserQuit                  },
    { "353",                &IRCBot::HandleChannelNamesList          },
    { "433",                &IRCBot::HandleNicknameInUse             },
    { "001",                &IRCBot::HandleServerMessage             },
    { "002",                &IRCBot::HandleServerMessage             },
    { "003",                &IRCBot::HandleServerMessage             },
    { "004",                &IRCBot::HandleServerMessage             },
    { "005",                &IRCBot::HandleServerMessage             },
    { "250",                &IRCBot::HandleServerMessage             },
    { "251",                &IRCBot::HandleServerMessage             },
    { "252",                &IRCBot::HandleServerMessage             },
    { "253",                &IRCBot::HandleServerMessage             },
    { "254",                &IRCBot::HandleServerMessage             },
    { "255",                &IRCBot::HandleServerMessage             },
    { "265",                &IRCBot::HandleServerMessage             },
    { "266",                &IRCBot::HandleServerMessage             },
    { "366",                &IRCBot::HandleEndOfNames                },
    { "372",                &IRCBot::HandleMOTDText                  },
    { "375",                &IRCBot::HandleStartOfMOTD               },
    { "376",                &IRCBot::HandleEndOfMOTD                 },
    { "422",                &IRCBot::HandleMissingMOTD               },
    { "439",                &IRCBot::HandleAwayMsgTooLong            },
};

void IRCBot::HandleCTCP(IRCMessage message)
{
    std::string to = message.parts.at(0);
    std::string text = message.parts.at(message.parts.size() - 1);

    // Remove '\001' from start/end of the string
    text = text.substr(1, text.size() - 2);

    std::cout << "[" + message.prefix.nick << " requested CTCP " << text << "]" << std::endl;

    if (to == _nick)
    {
        if (text == "VERSION") // Respond to CTCP VERSION
        {
            SendIRC("NOTICE " + message.prefix.nick + " :\001VERSION " + IRCBot::botctcpver + " \001");
            SendIRC("PRIVMSG " + message.prefix.nick + " :\001VERSION " + IRCBot::botctcpver + " \001");
            std::cout << "Sent CTCP version reply to " << message.prefix.nick << std::endl;
            return;
        }

        // CTCP not implemented
        SendIRC("NOTICE " + message.prefix.nick + " :\001ERRMSG " + text + " :Not implemented\001");
    }
}

void IRCBot::HandlePrivMsg(IRCMessage message)
{
    std::string to = message.parts.at(0);
    std::string text = message.parts.at(message.parts.size() - 1);

    // Handle Client-To-Client Protocol
    if (text[0] == '\001')
    {
        HandleCTCP(message);
        return;
    }

    if (to[0] == '#')
        std::cout << "From " + message.prefix.nick << " @ " + to + ": " << text << std::endl;
    else
        std::cout << "From " + message.prefix.nick << ": " << text << std::endl;
}

void IRCBot::HandleNotice(IRCMessage message)
{
    std::string from = message.prefix.nick != "" ? message.prefix.nick : message.prefix.prefix;
    std::string text;

    if( !message.parts.empty() )
        text = message.parts.at(message.parts.size() - 1);

    if (!text.empty() && text[0] == '\001')
    {
        text = text.substr(1, text.size() - 2);
        if (text.find(" ") == std::string::npos)
        {
            std::cout << "[Invalid " << text << " reply from " << from << "]" << std::endl;
            return;
        }
        std::string ctcp = text.substr(0, text.find(" "));
        std::cout << "[" << from << " " << ctcp << " reply]: " << text.substr(text.find(" ") + 1) << std::endl;
    }
    else
        std::cout << "-" << from << "- " << text << std::endl;
}

void IRCBot::HandleChannelJoinPart(IRCMessage message)
{
    std::string channel = message.parts.at(0);
    std::string action = message.command == "JOIN" ? "joins" : "leaves";
    std::cout << message.prefix.nick << " " << action << " " << channel << std::endl;
}

void IRCBot::HandleUserNickChange(IRCMessage message)
{
    std::string newNick = message.parts.at(0);
    std::cout << message.prefix.nick << " changed his nick to " << newNick << std::endl;
}

void IRCBot::HandleUserQuit(IRCMessage message)
{
    std::string text = message.parts.at(0);
    std::cout << message.prefix.nick << " quits (" << text << ")" << std::endl;
}

void IRCBot::HandleChannelNamesList(IRCMessage message)
{
    std::string channel = message.parts.at(2);
    std::string nicks = message.parts.at(3);
    std::cout << "People on " << channel << ":" << std::endl << nicks << std::endl;
}

void IRCBot::HandleNicknameInUse(IRCMessage message)
{
    std::cout << message.parts.at(1) << " " << message.parts.at(2) << std::endl;
}

void IRCBot::HandleServerMessage(IRCMessage message)
{
    if( message.parts.empty() )
        return;

    std::vector<std::string>::const_iterator itr = message.parts.begin();
    ++itr; // skip the first parameter (our nick)
    for (; itr != message.parts.end(); ++itr) {
        std::cout << *itr << " ";
    }
    std::cout << std::endl;
}

void IRCBot::HandleEndOfNames(IRCMessage message)
{
    std::cout << "SERVER [366 RPL_ENDOFNAMES]:" << std::endl;
    if( message.parts.empty() )
        return;

    std::vector<std::string>::const_iterator itr = message.parts.begin();
    ++itr; // skip the first parameter (our nick)
    for (; itr != message.parts.end(); ++itr) {
        std::cout << *itr << " ";
    }
    std::cout << std::endl;
}

void IRCBot::HandleStartOfMOTD(IRCMessage message)
{
    std::cout << "SERVER [375 RPL_MOTDSTART]:\n" << message.parts[1] << std::endl;
}

void IRCBot::HandleMOTDText(IRCMessage message)
{
    if( message.parts.empty() )
        return;

    std::vector<std::string>::const_iterator itr = message.parts.begin();
    ++itr; // skip the first parameter (our nick)
    for (; itr != message.parts.end(); ++itr) {;
        std::cout << *itr << " ";
    }
    std::cout << std::endl;
}

void IRCBot::HandleEndOfMOTD(IRCMessage message)
{
    std::cout << "SERVER [376 RPL_ENDOFMOTD]:\n" << message.parts[1] << std::endl;
    if (!IRCBot::runonlogin.empty()) {
        this->SendIRC(IRCBot::runonlogin);
        std::cout << "Sent: " + IRCBot::runonlogin << std::endl;
    }

    if (!IRCBot::nspassword.empty()) {
        IRCBot::SendIRC("PRIVMSG NickServ :IDENTIFY " + IRCBot::nspassword);
        std::cout << "CLIENT sent: PRIVMSG NickServ :IDENTIFY " << IRCBot::nspassword << ":\r\n";
    }

    if (!IRCBot::botchannel.empty()) {
        IRCBot::SendIRC("JOIN " + IRCBot::botchannel);
    }
}

void IRCBot::HandleMissingMOTD(IRCMessage message)
{
    std::cout << "SERVER [422 ERR_NOMOTD]: missing MOTD" << std::endl;
    if( message.parts.empty() )
        return;

    std::vector<std::string>::const_iterator itr = message.parts.begin();
    ++itr; // skip the first parameter (our nick)
    for (; itr != message.parts.end(); ++itr) {
        std::cout << *itr << " ";
    }
    std::cout << std::endl;
}

void IRCBot::HandleAwayMsgTooLong(IRCMessage message)
{
    std::cout << "SERVER [439 ERR_AWAYLENEXCEEDED]: AWAY message is too long!" << std::endl;
    if( message.parts.empty() )
        return;

    std::vector<std::string>::const_iterator itr = message.parts.begin();
    ++itr; // skip the first parameter (our nick)
    for (; itr != message.parts.end(); ++itr) {
        std::cout << *itr << " ";
    }
    std::cout << std::endl;
}