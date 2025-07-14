#include "ConfigParsing.hpp"
#include "WebservException.hpp"
#include "helper/utils.hpp"

ConfigParsing::ConfigParsing() {};
ConfigParsing::~ConfigParsing() {};


void ConfigParsing::start(std::string configFile)
{

    checkAndReadFile(configFile);
    if (content.empty())
        throw WebservException("Configuration file : config file is empty");
    removeComments();
    parseEachServer();    
    std::vector<ConfigData>::iterator server_it;
    for (server_it = configData.begin(); server_it != configData.end(); server_it++)
    {
        server_it->parseConfigData();
        // server_it->printData();
    }
}
 
void    ConfigParsing::removeComments( void )
{
    std::vector< std::string >::iterator line;

    for (line = content.begin(); line != content.end(); line++)
    {
        if (line->find("#", 0) != std::string::npos)
        {
            line->erase(line->find("#", 0), line->size());        
        }
    }
}

void ConfigParsing::parseEachServer()
{
    std::vector<ConfigData> serverBlocks;
    ConfigData currentServer;
    bool inServerBlock = false;
    int baseIndent = -1;

    if (content.empty())
        throw WebservException("Configuration file: file is empty");
    
    for (std::vector<std::string>::iterator it = content.begin(); it != content.end(); ++it)
    {
        std::string line = rtrim(*it);
    
        if (line.empty())
            continue;
        if (line.find("server:") == 0 && !inServerBlock)
        {
            inServerBlock = true;
            currentServer = ConfigData();
            baseIndent = countIndent(*it);
            currentServer.setContent(line);
            continue;
        }
        else if (inServerBlock)
        {
            int currentIndent = countIndent(*it);
            
            if (currentIndent > baseIndent)
                currentServer.setContent(line);
            else
            {
                serverBlocks.push_back(currentServer);
                inServerBlock = false;
                if (line.find("server:") == 0)
                {
                    inServerBlock = true;
                    currentServer = ConfigData();
                    baseIndent = countIndent(*it);
                    currentServer.setContent(line);
                }
                else 
                    throw WebservException("Invalid Config file");
            }
        }
        else 
            throw WebservException("Invalid Config file");
    }

    if (inServerBlock)
        serverBlocks.push_back(currentServer);
    
    this->configData = serverBlocks;
}

void ConfigParsing::checkAndReadFile(std::string configFile)
{
    if ( getFileType(configFile) != 2 )
        throw WebservException("Configuration file : file type is invalid");    
    if ( !checkAccess(configFile) )
        throw WebservException("Configuration file : file not accessable");
    readFile(configFile);
}

int ConfigParsing::getFileType(std::string path){
    struct stat s;

    if( stat(path.c_str(), &s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
            return 1;
        if( s.st_mode & S_IFREG )
            return 2;
    }
    return -1;
}

bool    ConfigParsing::checkAccess(std::string configFile)
{
    return (access(configFile.c_str(), F_OK) != -1);
}

void ConfigParsing::readFile(std::string configFile)
{
    std::string buffer;
    std::ifstream file;
    
    file.open(configFile.c_str());
    if (!file.is_open() || file.bad())
        throw WebservException("Configuration file : bad file");
    while (getline (file, buffer))
    {
        content.push_back(buffer);
    }
    file.close();
}

