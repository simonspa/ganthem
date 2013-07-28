#ifndef COMMAND_LINE_OPTIONS_H
#define COMMAND_LINE_OPTIONS_H
#include <string>
#include <map>

/**
    Разбор опций командной строки

        @author Oleg Khudyakov <o.khudyakov@initi.ru>
*/

class CommandLineOptions
{
public:
    CommandLineOptions(int argc, char *argv[], const char *optstring);
    ~CommandLineOptions();

    bool isSet(char);
    bool getParam(char opt, std::string &param);

private:
    std::map<char, std::string> optionsMap;
};

#endif
