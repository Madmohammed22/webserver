#include "ConfigData.hpp"
#include "helper/utils.hpp"

ConfigData::ConfigData() {}
ConfigData::~ConfigData() {}

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

size_t ConfigData::getClientMaxBodySize()
{
    return _client_max_body_size;
}

std::map<int, std::string>& ConfigData::getErrorPages()
{
    return _error_pages;
}

void ConfigData::setContent(std::string content)
{
    _content.push_back(content);
}

void ConfigData::setHost(std::string host)
{
	if (host == "localhost")
  {
		  _host = "127.0.0.1";
      return ;
  }
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

void ConfigData::setClientMaxBodySize(size_t client_max_body_size)
{
    _client_max_body_size = client_max_body_size;
}

void ConfigData::setErrorPages(std::map<int, std::string> error_pages)
{
    _error_pages = error_pages;
}

void ConfigData::setDefaultRoot(std::string root)
{
  _default_root = root;
}

std::string ConfigData::getDefaultRoot( void )
{
  return (_default_root);
}

void ConfigData::addErrorPage(int code, std::string path)
{
    _error_pages[code] = path;
}

std::vector<Location> ConfigData::getLocations() const
{
    return (_locations);
}

void ConfigData::parseConfigData()
{
    int baseIndent = 0;
    std::vector<std::string>::iterator it;

    _port = -1;
    for (it = _content.begin(); it != _content.end(); ++it)
    {
        std::string line = rtrim(*it);
        if (line.find("server:") == 0 || line.empty())
            continue;
        size_t colonPos = line.find(':');
        std::string key;

        if (baseIndent == 0)
            baseIndent = countIndent(line);
        if (baseIndent != countIndent(line))
            throw WebservException("Configuration file : wrong indentation");
        if (colonPos == std::string::npos)
            throw WebservException("Configuration file : Invalid format");

        key = line.substr(0, colonPos);
        std::string value = trim(line.substr(colonPos + 1));
        key = trim(key);
        if (key == "host")
        {
            if (!_host.empty())
            {
                throw WebservException("Configuration file : Host duplicated");
            }
            if (value == "localhost")
		        _host = "127.0.0.1";
            else
                _host = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
            if (_port > 65536 || _port < 0)
                throw WebservException("Configuration file : unvalid Port number");
        }
        else if (key == "server_name")
        {
            if (!_server_name.empty())
                throw WebservException("Configuration file : duplicated");
            _server_name = value;
        }
        else if (key == "client_max_body_size")
        {
            parseBodySize(value);
        }
        else if (key == "root")
        {
            if (!_default_root.empty())
                throw WebservException("Configuration file : duplicated");
            _default_root = value;
        }
        else if (key == "locations")
        {
            if (!value.empty())
                throw WebservException("Configuration file : invalid config file");
            int baseIndent2 = 0;
            it++;
            while (1)
            {
                if (it == _content.end())
                {
                    it--;
                    break;
                }
                line = rtrim(*it);
                if (line.empty())
                    continue;
                if (baseIndent >= countIndent(line))
                {
                    it--;
                    break;
                }
                parseLine(line, key, value, baseIndent2);
                if (!value.empty())
                    throw WebservException("Configuration file : invalid config file");
                int baseIndent3 = 0;
                it++;
                Location currentLocation;
                currentLocation.path = key;
                while (1)
                {
                    if (it == _content.end())
                        break;
                    line = rtrim(*it);
                    if (line.empty())
                        continue;
                    if (baseIndent2 >= countIndent(line))
                        break;
                    parseLine(line, key, value, baseIndent3);
                    parseLocation(key, value, currentLocation);
                    it++;
                }
                _locations.push_back(currentLocation);
            }
        }
        else if (key == "error_page")
        {
            if (!value.empty())
                throw WebservException("Configuration file : invalid config file");
            int baseIndent2 = 0;
            it++;
            while (1)
            {
                if (it == _content.end())
                {
                    it--;
                    break;
                }
                line = rtrim(*it);
                if (line.empty())
                    continue;
                if (baseIndent >= countIndent(line))
                {
                    it--;
                    break;
                }
                parseLine(line, key, value, baseIndent2);
                int code = atoi(key.c_str());
                _error_pages[code] = value;
                it++;
            }
        }
        else
            throw WebservException("Configuration file : invalid key");
    }
}

void ConfigData::parseLine(std::string line, std::string &key, std::string &value, int &baseIndent)
{

    size_t colonPos = line.find(':');
    if (baseIndent == 0)
        baseIndent = countIndent(line);
    if (baseIndent != countIndent(line))
        throw WebservException("Configuration file : wrong indentation");
    if (colonPos == std::string::npos)
        throw WebservException("Configuration file : Invalid format");

    key = line.substr(0, colonPos);
    value = trim(line.substr(colonPos + 1));
    key = trim(key);
}

void ConfigData::parseLocation(std::string key, std::string value, Location &currentLocation)
{
    if (key == "methods")
    {
        parseArrayValue(value, currentLocation.methods);
    }
    else if (key == "root")
    {
        if (!currentLocation.root.empty())
            throw WebservException("Configuration File: duplicated key");
        currentLocation.root = value;
    }
    else if (key == "index")
    {
        if (!currentLocation.index.empty())
            throw WebservException("Configuration File: duplicated key");
        parseArrayValue(value, currentLocation.index);
    }
    else if (key == "autoindex")
    {
        currentLocation.autoindex = (value == "on" || value == "true");
    }
    else if (key == "cgi")
    {
        if (!currentLocation.cgi.empty())
            throw WebservException("Configuration File: duplicated key");
        parseCgiPair(value, currentLocation.cgi);
    }
    else if (key == "redirect")
    {
        if (!currentLocation.redirect.empty())
            throw WebservException("Configuration File: duplicated key");
        currentLocation.redirect = value;
    }
    else if (key == "upload")
    {
        if (!currentLocation.upload.empty())
            throw WebservException("Configaration File: duplicated key");
        currentLocation.upload = value;
    }
    else
    {
        throw WebservException("Configuration file : invalid key");
    }
}

void ConfigData::parseBodySize(const std::string &value)
{
    if (value.empty())
        return;

    size_t multiplier = 1;
    std::string sizeValue = value;
    char lastChar = tolower(value[value.size() - 1]);

    if (lastChar == 'k')
    {
        multiplier = 1024;
        sizeValue = value.substr(0, value.size() - 1);
    }
    else if (lastChar == 'm')
    {
        multiplier = 1024 * 1024;
        sizeValue = value.substr(0, value.size() - 1);
    }
    _client_max_body_size = atoi(sizeValue.c_str()) * multiplier;
}

void ConfigData::parseCgiPair(const std::string &value, std::map<std::string, std::string> &target)
{
    if (value.empty())
        return;

    if (value[0] == '[')
    {
        std::string cleaned = value.substr(1, value.size() - 2);
        std::vector<std::string> pairs = split(cleaned, ',');
        
        for (size_t i = 0; i < pairs.size(); i++)
        {
            std::string pair = trim(pairs[i], " \"'");
            size_t colonPos = pair.find(':');
            if (colonPos == std::string::npos)
                throw WebservException("Configuration file: invalid CGI pair format");
            
            std::string ext = trim(pair.substr(0, colonPos));
            std::string interpreter = trim(pair.substr(colonPos + 1));
            
            if (ext.empty() || interpreter.empty())
                throw WebservException("Configuration file: empty cgi");
            
            target[ext] = interpreter;
        }
    }
    else
    {
        size_t colonPos = value.find(':');
        if (colonPos == std::string::npos)
            throw WebservException("Configuration file: invalid CGI pair format");
        
        std::string ext = trim(value.substr(0, colonPos));
        std::string interpreter = trim(value.substr(colonPos + 1));
        
        if (ext.empty() || interpreter.empty())
            throw WebservException("Configuration file: empty extension or interpter");
        
        target[ext] = interpreter;
    }
}

void ConfigData::parseArrayValue(std::string value, std::vector<std::string> &target)
{
    if (value[0] == '[')
    {
        value = value.substr(1, value.size() - 2);
        std::vector<std::string> items = split(value, ',');
        for (size_t i = 0; i < items.size(); i++)
        {
            target.push_back(trim(items[i], " \"'"));
        }
    }
    else
    {
        target.push_back(value);
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
    std::vector<Location>::const_iterator loc_it;
    for (loc_it = _locations.begin(); loc_it != _locations.end(); ++loc_it)
    {
        std::cout << "-------------------------------------------------\n";
        std::cout << "  " << loc_it->path << ":" << std::endl;
        std::cout << "    Root: " << loc_it->root << std::endl;

        std::cout << "    Redirect: " << loc_it->redirect << std::endl;
        std::cout << "    Index: ";
        for (size_t i = 0; i < loc_it->index.size(); ++i)
            std::cout << loc_it->index[i] << (i != loc_it->index.size() - 1 ? ", " : "");
        std::cout << std::endl;

        std::cout << "    Methods: ";
        for (size_t i = 0; i < loc_it->methods.size(); ++i)
            std::cout << loc_it->methods[i] << (i != loc_it->methods.size() - 1 ? ", " : "");
        std::cout << std::endl;

        std::cout << "    Autoindex: " << (loc_it->autoindex ? "on" : "off") << std::endl;

        std::cout << std::endl;
        std::cout << "-------------------------------------------------\n";
    }
}
