#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include "ConfigData.hpp"

class ConfigParsing
{
public:

    std::vector <ConfigData> configData;
    std::vector <std::string> content;

    ConfigParsing();
    ~ConfigParsing();
    void        start(std::string configFile);
    void        checkAndReadFile(std::string configFile);
    void        removeComments();
    int         getFileType(std::string configFile);
    bool        checkAccess(std::string configFile);
    void        readFile(std::string configFile);
    void        parseEachServer();
};

#endif
