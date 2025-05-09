#include "ConfigParsing.hpp"
#include "WebservException.hpp"
#include "utils.hpp"

ConfigParsing::ConfigParsing() {};
ConfigParsing::~ConfigParsing() {};


void ConfigParsing::start(std::string configFile)
{

    checkAndReadFile(configFile);
    if (content.empty())
        throw WebservException("Configuration file : config file is empty");
    removeComments();
    /*std::vector<std::string>::iterator it;*/
    /**/
    /*for(it = content.begin(); it != content.end(); it++)*/
    /*{*/
    /*    std::cout<< *it << std::endl;*/
    /*}*/
    parseEachServer();
    
    std::vector<ConfigData>::iterator server_it;

    for (server_it = configData.begin(); server_it != configData.end(); server_it++)
    {
        std::vector<std::string> serverContent = server_it->getContent();
        std::vector<std::string>::iterator line_it;

        std::cout << "-------------------------------------------------" << std::endl;

        for (line_it = serverContent.begin(); line_it != serverContent.end(); line_it++)
        {
            std::string line = *line_it;
            std::cout << line << std::endl;
        }    
        std::cout << "-------------------------------------------------" << std::endl;
    }
    
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

        if (inServerBlock)
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
            }
        }
    }

    if (inServerBlock)
        serverBlocks.push_back(currentServer);
    
    this->configData = serverBlocks;
}


/*void ConfigParsing::parseEachServer()*/
/*{*/
/*    // first we're gonna split the content into each server*/
/*    ConfigData tmp;*/
/*    std::string tmpContent;*/
/*    std::vector<std::string>::iterator it;*/
/**/
/*    // check for the first keyword , it should be server*/
/*    if (rtrim(content.front()) != "server:")*/
/*        throw WebservException("Configuration file : \"server\" keyword not found");*/
/*    for(it = content.begin(); it != content.end(); it++)*/
/*    {*/
/*        tmp.setContent(*it);*/
/*        std::cout<< *it << std::endl;*/
/*    }*/
/*}*/

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

