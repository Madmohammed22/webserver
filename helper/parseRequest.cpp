#include "../server.hpp"

void Server::printfContentHeader(Server *server, int fd)
{
    std::map<std::string, std::string>::iterator it = server->request[fd].state.mapOnHeader.begin();
    while (it != server->request[fd].state.mapOnHeader.end())
    {
        std::cout << it->first << " " << it->second << std::endl;
        it++;
    }
}

std::string Server::parseSpecificRequest(std::string request)
{
    std::string filePath;
    filePath = "/index.html";
    // if (searchOnSpecificFile(PATHC, "index.html") == true)
    // else
    //     filePath = "undefined";
    // Handle GET requests
    size_t startPos = request.find("GET /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
        return filePath;
    }

    // Handle DELETE requests
    startPos = request.find("DELETE /");
    if (startPos != std::string::npos)
    {
        startPos += 8;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
        return filePath;
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
            if (!requestedPath.empty() && requestedPath == "/")
            {
                filePath = PATHC;
            }
            else
                return requestedPath;
        }
        return filePath;
    }

    return filePath;
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
