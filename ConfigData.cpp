#include "ConfigData.hpp"
#include "utils.hpp"

ConfigData::ConfigData(){}
ConfigData::~ConfigData(){}

std::vector<std::string> ConfigData::getContent()
{
    return _content;
}

std::string ConfigData::getHost()
{
    return _host;
}

int ConfigData::getPort()
{
    return _port;
}

std::string ConfigData::getServerName()
{
    return _server_name;
}

std::string ConfigData::getRoot()
{
    return _root;
}

std::vector<std::string> ConfigData::getIndex()
{
    return _index;
}

bool ConfigData::getAutoindex()
{
    return _autoindex;
}

size_t ConfigData::getClientMaxBodySize()
{
    return _client_max_body_size;
}

std::map<int, std::string> ConfigData::getErrorPages()
{
    return _error_pages;
}

std::vector<std::string> ConfigData::getMethods()
{
    return _methods;
}

std::map<std::string, std::string> ConfigData::getCgi()
{
    return _cgi;
}

void ConfigData::setContent(std::string content)
{
    _content.push_back(content);
}

void ConfigData::setHost(std::string host)
{
    _host = host;
}

void ConfigData::setPort(int port)
{
    _port = port;
}

void ConfigData::setServerName(std::string server_name)
{
    _server_name = server_name;
}

void ConfigData::setRoot(std::string root)
{
    _root = root;
}

void ConfigData::setIndex(std::vector<std::string> index)
{
    _index = index;
}

void ConfigData::addIndex(std::string index)
{
    _index.push_back(index);
}

void ConfigData::setAutoindex(bool autoindex)
{
    _autoindex = autoindex;
}

void ConfigData::setClientMaxBodySize(size_t client_max_body_size)
{
    _client_max_body_size = client_max_body_size;
}

void ConfigData::setErrorPages(std::map<int, std::string> error_pages)
{
    _error_pages = error_pages;
}

void ConfigData::addErrorPage(int code, std::string path)
{
    _error_pages[code] = path;
}

void ConfigData::setMethods(std::vector<std::string> methods)
{
    _methods = methods;
}

void ConfigData::addMethod(std::string method)
{
    _methods.push_back(method);
}

void ConfigData::setCgi(std::map<std::string, std::string> cgi)
{
    _cgi = cgi;
}

void ConfigData::addCgi(std::string extension, std::string interpreter)
{
    _cgi[extension] = interpreter;
}

void ConfigData::parseConfigData()
{
    std::string currentKey;
    Location* currentLocation = NULL;
    int currentIndent = 0;
    
    std::vector<std::string>::iterator it;

    for (it = _content.begin(); it != _content.end(); ++it)
    {
        std::string line = rtrim(*it);
        if (line.find("server:") == 0)
        {
            continue;
        }
        if (line.empty()) continue;

        int indent = countIndent(line);
        line = line.substr(indent);

        if (line.find("locations:") == 0)
        {
            continue;
        }
        else if (indent > currentIndent && currentLocation)
        {
            line = trim(line);
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = line.substr(0, colonPos);
                std::string value = trim(line.substr(colonPos + 1));

                if (key == "methods")
                {
                    if (value[0] == '[')
                    {
                        value = value.substr(1, value.size() - 2);
                        std::vector<std::string> methods = split(value, ',');
                        for (size_t i = 0; i < methods.size(); i++) {
                            currentLocation->methods.push_back(trim(methods[i], " \"'"));
                        }
                    }
                    else
                    {
                        currentLocation->methods.push_back(value);
                    }
                }
                else if (key == "root")
                {
                    currentLocation->root = value;
                }
                else if (key == "index")
                {
                    if (value[0] == '[')
                    {
                        value = value.substr(1, value.size() - 2);
                        std::vector<std::string> indexes = split(value, ',');
                        for (size_t i = 0; i < indexes.size(); i++)
                            currentLocation->index.push_back(trim(indexes[i], " \"'"));
                    }
                    else
                    {
                        currentLocation->index.push_back(value);
                    }
                }
                else if (key == "autoindex")
                {
                    currentLocation->autoindex = (value == "on" || value == "true");
                }
                else if (key == "cgi")
                {
                    size_t cgiColonPos = value.find(':');
                    if (cgiColonPos != std::string::npos) {
                        std::string ext = trim(value.substr(0, cgiColonPos));
                        std::string interpreter = trim(value.substr(cgiColonPos + 1));
                        currentLocation->cgi[ext] = interpreter;
                    }
                }
            }
        }
        else if (indent == currentIndent || currentLocation == NULL)
        {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos)
            {
                currentKey = line.substr(0, colonPos);
                std::string value = trim(line.substr(colonPos + 1));

                if (currentKey == "- path")
                {
                    std::string path = trim(value, " \"'");
                    _locations[path] = Location();
                    _locations[path].path = path;
                    currentLocation = &_locations[path];
                    currentIndent = indent;
                }
                else
                {
                    if (currentKey == "host")
                    {
                        _host = value;
                    }
                    else if (currentKey == "port")
                    {
                        _port = atoi(value.c_str());
                    }
                    else if (currentKey == "server_name")
                    {
                        _server_name = value;
                    }
                    else if (currentKey == "root")
                    {
                        _root = value;
                    }
                    else if (currentKey == "index")
                    {
                        if (value[0] == '[')
                        {
                            value = value.substr(1, value.size() - 2);
                            std::vector<std::string> indexes = split(value, ',');
                            for (size_t i = 0; i < indexes.size(); i++)
                                _index.push_back(trim(indexes[i], " \"'"));
                        }
                        else
                        {
                            _index.push_back(value);
                        }
                    }
                    else if (currentKey == "autoindex")
                    {
                        _autoindex = (value == "on" || value == "true");
                    }
                    else if (currentKey == "client_max_body_size")
                    {
                        size_t multiplier = 1;
                        if (!value.empty())
                        {
                            char lastChar = tolower(value[value.size() - 1]);
                            if (lastChar == 'k')
                            {
                                multiplier = 1024;
                                value = value.substr(0, value.size() - 1);
                            }
                            else if (lastChar == 'm')
                            {
                                multiplier = 1024 * 1024;
                                value = value.substr(0, value.size() - 1);
                            }
                            _client_max_body_size = atoi(value.c_str()) * multiplier;
                        }
                    }
                    else if (currentKey == "error_page")
                    {
                        size_t colonPos = value.find(':');
                        if (colonPos != std::string::npos)
                        {
                            int code = atoi(value.substr(0, colonPos).c_str());
                            std::string path = trim(value.substr(colonPos + 1));
                            _error_pages[code] = path;
                        }
                    }
                    else if (currentKey == "cgi")
                    {
                        size_t cgiColonPos = value.find(':');
                        if (cgiColonPos != std::string::npos) {
                            std::string ext = trim(value.substr(0, cgiColonPos));
                            std::string interpreter = trim(value.substr(cgiColonPos + 1));
                            _cgi[ext] = interpreter;
                        }
                    }
                }
            }
        }
    }
}

void ConfigData::printData()
{
    std::cout << "Host: " << _host << std::endl;
    std::cout << "Port: " << _port << std::endl;
    std::cout << "Server Name: " << _server_name << std::endl;
    std::cout << "Client Max Body Size: " << _client_max_body_size << std::endl;

    std::cout << "Error Pages: ";
    std::map<int, std::string>::const_iterator it;
    for (it = _error_pages.begin(); it != _error_pages.end(); ++it)
        std::cout << it->first << ":" << it->second << (it != --_error_pages.end() ? ", " : "");
    std::cout << std::endl;

    std::cout << "Locations:" << std::endl;
    std::map<std::string, Location>::const_iterator loc_it;
    for (loc_it = _locations.begin(); loc_it != _locations.end(); ++loc_it)
    {
        std::cout << "-------------------------------------------------\n";
        std::cout << "  " << loc_it->first << ":" << std::endl;
        std::cout << "    Root: " << loc_it->second.root << std::endl;
        
        std::cout << "    Index: ";
        for (size_t i = 0; i < loc_it->second.index.size(); ++i)
            std::cout << loc_it->second.index[i] << (i != loc_it->second.index.size() - 1 ? ", " : "");
        std::cout << std::endl;

        std::cout << "    Methods: ";
        for (size_t i = 0; i < loc_it->second.methods.size(); ++i)
            std::cout << loc_it->second.methods[i] << (i != loc_it->second.methods.size() - 1 ? ", " : "");
        std::cout << std::endl;

        std::cout << "    Autoindex: " << (loc_it->second.autoindex ? "on" : "off") << std::endl;

        std::cout << "    CGI: ";
        std::map<std::string, std::string>::const_iterator cgi_it;
        for (cgi_it = loc_it->second.cgi.begin(); cgi_it != loc_it->second.cgi.end(); ++cgi_it)
            std::cout << cgi_it->first << ":" << cgi_it->second << (cgi_it != --loc_it->second.cgi.end() ? ", " : "");
        std::cout << std::endl;
        std::cout << "-------------------------------------------------\n";
    }
}
