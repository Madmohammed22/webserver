#include "../server.hpp"

// void reWrite(std::string &url, ConfigData configData){
//     if (url == "/")
//         url =  configData.get
// }
void Server::printfContentHeader(Server *server, int fd)
{
    std::map<std::string, std::string>::iterator it = server->request[fd].state.mapOnHeader.begin();
    while (it != server->request[fd].state.mapOnHeader.end())
    {
        std::cout << it->first << " " << it->second << std::endl;
        it++;
    }
}

std::string url_decode(const std::string &value)
{
    std::ostringstream decoded;
    std::istringstream encoded(value);
    char c;

    while (encoded >> c)
    {
        if (c == '%')
        {
            char hex[3];
            encoded.read(hex, 2);
            hex[2] = '\0';
            int char_code;
            sscanf(hex, "%2x", &char_code);
            decoded << static_cast<char>(char_code);
        }
        else if (c == '+')
        {
            decoded << ' ';
        }
        else
        {
            decoded << c;
        }
    }

    return decoded.str();
}

std::string Server::redundantSlash(std::string url)
{
    std::string new_url;
    for (size_t i = 0; i < url.size(); i++)
    {
        new_url += url[i];
        for (size_t j = i; url[j] == '/'; j++)
        i = j;
    }
    
    return new_url;
}

std::string Server::parseSpecificRequest(std::string request)
{
    std::string url;
    size_t startPos = request.find("GET ");
    if (startPos != std::string::npos)
    {
        startPos += 4;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            requestedPath = url_decode(redundantSlash(requestedPath));
            if (!requestedPath.empty())
            {
                url = requestedPath;

            }
        }
        return url;
    }

    // Handle DELETE requests
    startPos = request.find("DELETE /");
    if (startPos != std::string::npos)
    {
        startPos += 7;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            requestedPath = url_decode(redundantSlash(requestedPath));
            if (!requestedPath.empty())
            {
                url = requestedPath;
            }
        }
        return url;
    }

    // Handle POST requests
    startPos = request.find("POST /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            requestedPath = url_decode(redundantSlash(requestedPath));
            if (!requestedPath.empty() && requestedPath == "/")
            {
                url = PATHC;
            }
            else
                return requestedPath;
        }
        return url;
    }

    return url;
}

std::string Server::trim(std::string str)
{
    str.erase(str.find_last_not_of(' ') + 1);
    str.erase(0, str.find_first_not_of(' '));
    return str;
}

void Server::key_value_pair_header(int fd, std::string header)
{
    std::map<std::string, std::string> mapv = request[fd].state.mapOnHeader;
    size_t j = 0;
    for (size_t i = 0; i < header.length(); i++)
    {
        std::string result;
        if (static_cast<unsigned char>(header.at(i)) == 10)
        {
            result = header.substr(j, (i - j - 1));
            if (!result.empty())
            {
                mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))),
                                                                trim(result.substr(result.find(" "), result.length()))));
            }
            j = i + 1;
        }
    }
    std::string result = header.substr(j, header.length());
    mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))), trim(result.substr(result.find(" "), result.length()))));
    request[fd].state.mapOnHeader = mapv;
}

std::map<std::string, std::string> Server::key_value_pair(std::string header)
{
    std::map<std::string, std::string> mapv;
    size_t j = 0;
    for (size_t i = 0; i < header.length(); i++)
    {
        std::string result;
        if (static_cast<unsigned char>(header.at(i)) == 10)
        {
            result = header.substr(j, (i - j - 1));
            if (!result.empty())
            {
                mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))),
                                                                trim(result.substr(result.find(" "), result.length()))));
            }
            j = i + 1;
        }
    }
    std::string result = header.substr(j, header.length());
    mapv.insert(std::pair<std::string, std::string>(trim(result.substr(0, result.find(" "))), trim(result.substr(result.find(" "), result.length()))));
    return mapv;
}

bool Server::containsOnlyWhitespace(const std::string &str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        if (!isspace(str.at(i)))
            return false;
    }
    return true;
}

bool Server::searchOnSpecificFile(std::string path, std::string fileTarget)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return perror("opendir"), false;
    struct dirent *dr;
    while ((dr = readdir(dir)))
    {
        if (dr->d_name == fileTarget)
            return true;
    }
    closedir(dir);
    return false;
}

Location getLocation_adder1(std::string targetLocation, ConfigData configIndex)
{
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (targetLocation == configIndex.getLocations()[i].path)
        {
            configIndex.getLocations()[i];
        }
    }
    return configIndex.getLocations()[0];
}
