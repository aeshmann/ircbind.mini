#ifndef HANDLER_H_
#define HANDLER_H_

#include "ircbot.h"

#define NUM_IRC_CMDS 27

struct IRCCommandHandler
{
    std::string command;
    void (IRCBot::*handler)(IRCMessage /*message*/);
};

extern IRCCommandHandler ircCommandTable[NUM_IRC_CMDS];

inline int GetCommandHandler(const std::string& command)
{
    for (int i = 0; i < NUM_IRC_CMDS; ++i)
    {
        if (ircCommandTable[i].command == command)
            return i;
    }

    return NUM_IRC_CMDS;
}

#endif