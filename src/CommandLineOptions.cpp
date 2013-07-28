#include "CommandLineOptions.h"
#include <unistd.h>
#include <stdlib.h>

CommandLineOptions::CommandLineOptions(int argc, char *argv[], const char *optstring)
{
    int opt;
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        if (optarg)
        {
            optionsMap[opt] = optarg;
        }
        else
        {
            optionsMap[opt] = std::string();
        }
    }
}

CommandLineOptions::~CommandLineOptions()
{
}

bool CommandLineOptions::isSet(char opt)
{
    std::map<char, std::string>::iterator optionsMapIt = optionsMap.find(opt);
    if (optionsMapIt == optionsMap.end())
    {
        return false;
    }

    return true;
}

bool CommandLineOptions::getParam(char opt, std::string &param)
{
    if (!isSet(opt))
    {
        return false;
    }

    if (optionsMap[opt].empty())
    {
        return false;
    }

    param = optionsMap[opt];

    return true;
}
