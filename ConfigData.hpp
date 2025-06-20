#ifndef CONFIGDATA_HPP
#define CONFIGDATA_HPP

#include <iostream>
#include <vector>
#include "WebservException.hpp"
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

class Location
{
public:
    std::string path;
    std::string root;
    std::string redirect;
    std::string upload;
    std::vector<std::string> index;
    std::vector<std::string> methods;
    bool autoindex;
    std::map<std::string, std::string> cgi;
    
    Location() : autoindex(false) {}
};

class ConfigData
{


private:
    std::vector<Location> _locations;
    std::vector<std::string> _content;
    std::string _host;
    int _port;
    std::string _server_name;
    size_t _client_max_body_size;
    std::map<int, std::string> _error_pages;

public:
    bool inErrorPage;

public:
    ConfigData();
    ~ConfigData();
    
    std::vector<std::string> getContent();
    std::vector<Location> getLocations() const;
    void addLocation(const std::string& path, const Location& location);
    std::string getHost();
    int getPort();
    std::string getServerName();
    size_t getClientMaxBodySize();
    std::map<int, std::string> getErrorPages();
    std::vector<std::string> getMethods();
    std::map<std::string, std::string> getCgi();

    //[soukaina] some of that shit should be deleted
    void setContent(std::string content);
    void setHost(std::string host);
    void setPort(int port);
    void setRedirect(std::string);
    void setServerName(std::string server_name);
    void setClientMaxBodySize(size_t client_max_body_size);
    void setErrorPages(std::map<int, std::string> error_pages);
    void addErrorPage(int code, std::string path);


    void parseConfigData();
    void parseLine(std::string line, std::string &key, std::string &value, int &baseIndent);
    void parseLocation(std::string key, std::string value, Location &currentLocation);
    void parseServerDirective(std::string line, int &firstIndent, std::string& currentKey, Location*& currentLocation, int& currentIndent, int indent);
    void createNewLocation(std::string value, Location& currentLocation);
    void handleServerConfigDirective(const std::string& key, const std::string& value);
    void parseArrayValue(std::string value, std::vector<std::string>& target);
    void parseCgiPair(const std::string& value, std::map<std::string, std::string>& target);
    void parseBodySize(const std::string& value);
    void parseErrorPage(const std::string& value);
    void printData();
};

#endif
