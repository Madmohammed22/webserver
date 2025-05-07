#ifndef CONFIGDATA_HPP
#define CONFIGDATA_HPP

#include <iostream>
#include <vector>
#include <fstream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

class Location
{
public:
    std::string path;
    std::string root;
    std::vector<std::string> index;
    std::vector<std::string> methods;
    bool autoindex;
    std::map<std::string, std::string> cgi;
    
    Location() : autoindex(false) {}
};

class ConfigData
{
private:
    std::map<std::string, Location> _locations;
    std::vector<std::string> _content;
    std::string _host;
    int _port;
    std::string _server_name;
    std::string _root;
    std::vector<std::string> _index;
    bool _autoindex;
    size_t _client_max_body_size;
    std::map<int, std::string> _error_pages;
    std::vector<std::string> _methods;
    std::map<std::string, std::string> _cgi;

public:
    ConfigData();
    ~ConfigData();
    
    std::vector<std::string> getContent();
    std::map<std::string, Location> getLocations() const;
    void addLocation(const std::string& path, const Location& location);
    std::string getHost();
    int getPort();
    std::string getServerName();
    std::string getRoot();
    std::vector<std::string> getIndex();
    bool getAutoindex();
    size_t getClientMaxBodySize();
    std::map<int, std::string> getErrorPages();
    std::vector<std::string> getMethods();
    std::map<std::string, std::string> getCgi();

    void setContent(std::string content);
    void setHost(std::string host);
    void setPort(int port);
    void setServerName(std::string server_name);
    void setRoot(std::string root);
    void setIndex(std::vector<std::string> index);
    void addIndex(std::string index);
    void setAutoindex(bool autoindex);
    void setClientMaxBodySize(size_t client_max_body_size);
    void setErrorPages(std::map<int, std::string> error_pages);
    void addErrorPage(int code, std::string path);
    void setMethods(std::vector<std::string> methods);
    void addMethod(std::string method);
    void setCgi(std::map<std::string, std::string> cgi);
    void addCgi(std::string extension, std::string interpreter);


    void parseConfigData();
    void printData();
};

#endif
