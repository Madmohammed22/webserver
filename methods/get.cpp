/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 15:08:56 by mmad              #+#    #+#             */
/*   Updated: 2025/04/21 16:55:45 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../server.hpp"
#include "../request.hpp"
#include "../helper/utils.hpp"

void GET::includeBuild(std::string target, std::string &metaData, int pick)
{
    std::map<std::string, std::string>::iterator it = request.keys.find(target);
    if (it != request.keys.end())
    {
        if (Server::containsOnlyWhitespace(it->second) == false)
        {
            pick == 1 ? metaData = it->first : metaData = it->second;
        }
        else
            metaData = "empty";
        return;
    }
    metaData = "undefined";
}

void GET::buildFileTransfers()
{
    FileTransferState &state = request.state;
    state.url = Server::parseSpecificRequest(request.header);
    state.offset = 0;
    state.fileSize = Server::getFileSize(state.url);
    state.isComplete = false;
    state.mime = Server::getContentType(state.url);
    state.uriLength = state.url.length();
    state.test = 0;
    state.last_activity_time = time(NULL);
}

std::ifstream::pos_type Server::getFileSize(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return 0;
    return file.tellg();
}

bool readFileChunk(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    file.seekg(offset);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();

    return true;
}

bool Server::check(std::string url)
{
    url = redundantSlash(url);
    std::ifstream file(url.c_str());

    if (!file.is_open())
    {
        return false;
    }
    return true;
}

bool matchPath(std::string url, Location location)
{
    if (url == "/")
        return false;

    if (url == location.path)
        return true;
    return false;
}

std::string Server::fetchIndex(std::string root, std::vector<std::string> indexFile)
{
    for (size_t i = 0; i < indexFile.size(); i++)
    {
        if (check(root + indexFile[i]) == true)
            return indexFile[i];
    }
    return "";
}

bool is_directory_empty(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        if (errno == ENOENT)
        {
            std::cerr << "Error: Directory not found: " << dir_path << std::endl;
        }
        else
        {
            std::cerr << "Error opening directory: " << dir_path << std::endl;
        }
        return false;
    }

    struct dirent *entry;
    bool is_empty = true;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            is_empty = false;
            break;
        }
    }
    closedir(dir);
    return is_empty;
}

bool searchOnFile(std::string dir_name, std::string file_name)
{
    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(dir_name.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG && file_name == ent->d_name)
            {
                closedir(dir);
                return true;
            }
        }
        closedir(dir);
        return false;
    }
    else
    {
        return false;
    }
    return false;
}

bool validateSearch(std::vector<std::string> indexFile, std::string dir_name)
{

    for (size_t i = 0; i < indexFile.size(); i++)
    {
        if (searchOnFile(dir_name, indexFile[i]) == false)
        {
            return false;
        }
    }
    return true;
}

bool Server::canBeOpen(int fd, std::string &url, Location location, size_t &checkState, ConfigData configIndex)
{
    std::string root = configIndex.getDefaultRoot();
    if (redundantSlash((resolveUrl(url))) == redundantSlash(location.path))
    {
        if (location.redirect.size() > 0)
        {
            request[fd].flag = 1;
            url = redundantSlash(location.redirect);
            request[fd].state.url = redundantSlash(location.redirect);
            checkState = 301;
            return check(redundantSlash(location.root + location.path + url));
        }
        else if (location.index.size() > 0 && validateSearch(location.index, location.root + url) == true)
        {
            checkState = 302;
            url = redundantSlash(location.root + location.path + fetchIndex(location.root + location.path, location.index));
            return check(url);
        }
        else
        {
            url = location.root + url;
            checkState = 201;
            return check(url);
        }
    }
    else
    {
        if (url.find(location.root) != std::string::npos)
        {
            url = url.substr(url.rfind("/"), url.length());
        }
        url = location.root + url;
        checkState = (check(url) == true) ? 200 : 404;
        return (checkState == 200) ? true : false;
    }

    return check(url);
}

bool sendChunk(int fd, const char *data, size_t size)
{
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string data_size = chunkHeader.str();
    if (send(fd, data_size.c_str(), data_size.length(), MSG_NOSIGNAL) == -1)
        return false;
    if (send(fd, data, size, MSG_NOSIGNAL) == -1)
        return false;
    if (send(fd, "\r\n", 2, MSG_NOSIGNAL) == -1)
        return false;
    return true;
}

bool sendFinalChunk(int fd)
{
    return sendChunk(fd, "", 0) &&
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

int Server::continueFileTransfer(int fd, std::string url, Location location)
{
    (void)location;
    char buffer[CHUNK_SIZE];
    size_t remainingBytes = request[fd].state.fileSize - request[fd].state.offset;
    size_t bytesToRead;
    remainingBytes > CHUNK_SIZE ? bytesToRead = CHUNK_SIZE : bytesToRead = remainingBytes;
    size_t bytesRead = 0;
    if (!readFileChunk(url, buffer, request[fd].state.offset, bytesToRead, bytesRead))
    {
        return request[fd].getConnection() == "close" ? -1 : 0;
    }

    if (!sendChunk(fd, buffer, bytesRead))
    {
        getResponse(fd, 500);
        close(fd), request.erase(fd);
        return request[fd].getConnection() == "close" ? -1 : 0;
    }
    request[fd].state.offset += bytesRead;

    if (request[fd].state.offset >= request[fd].state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            getResponse(fd, 500);
            close(fd), request.erase(fd);
            return request[fd].getConnection() == "close" ? -1 : 0;
        }
        if (request[fd].getConnection() == "close")
        {
            return request[fd].state.test = 0, 0;
        }
        request[fd].state.test = 0;
    }
    return 200;
}

int Server::handleFileRequest(int fd, std::string &url, std::string Connection, Location location)
{

    request[fd].state.url = url;
    std::string contentType = Server::getContentType(url);
    request[fd].state.fileSize = getFileSize(url);
    size_t LARGE_FILE_THRESHOLD = 1024 * 1024;

    if (request[fd].state.fileSize > LARGE_FILE_THRESHOLD)
    {
        request[fd].state.test = 1;
        std::string httpRespons = createChunkedHttpResponse(contentType);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1){
            getResponse(fd, 500);
            close(fd), request.erase(fd);
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, 0;
        }
        return continueFileTransfer(fd, request[fd].state.url, location);
    }
    else
    {

        std::string httpRespons;
        if (location.redirect.size() > 0 && request[fd].flag == 1)
        {
            request[fd].flag = 0;
            httpRespons = MovedPermanently(contentType, redundantSlash(location.path + location.redirect));
            if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1){
                getResponse(fd, 500);
                return close(fd), request.erase(fd), 0;
            }
            return 0;
        }
        else
                httpRespons = httpResponse(contentType, request[fd].state.fileSize);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
            return getResponse(fd, 500), close(fd), request.erase(fd), 0;
        if (send(fd, readFile(url).c_str(), request[fd].state.fileSize, MSG_NOSIGNAL) == -1)
            return getResponse(fd, 500), close(fd), request.erase(fd), 0;
        if (send(fd, "\r\n\r\n", 4, MSG_NOSIGNAL) == -1)
            return getResponse(fd, 500), close(fd), request.erase(fd), 0;
        if (Connection == "close" || Connection.empty())
        {
            return request[fd].state.isComplete = true, close(fd), request.erase(fd), 0;
        }
        return request[fd].state.isComplete = true, 200;
    }
    return 0;
}

std::string Server::readFile(std::string path)
{

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

Location findRoot(ConfigData configIndex, std::string path)
{
    Location location;
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (path == "/")
        {
            if (path == configIndex.getLocations()[i].path)
            {
                return configIndex.getLocations()[i];
                ;
            }
        }
    }
    return location;
}

std::string returnNewPath(std::string path)
{
    size_t pos = 0;

    pos = path.rfind('/', path.size() - 2);

    return path.substr(0, pos + 1);
}

Location returnDefault(ConfigData configIndex)
{
    Location location;
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if ("/" == configIndex.getLocations()[i].path)
        {
            location = configIndex.getLocations()[i];
            return location;
        }
    }
    return location;
}

Location Server::getExactLocationBasedOnUrlContainer(std::string target, ConfigData configIndex)
{
    Location location;
    std::string root = configIndex.getDefaultRoot();
    if (target == "/" && !returnDefault(configIndex).path.empty())
    {
        return returnDefault(configIndex);
    }
    if (returnDefault(configIndex).path.empty())
        return location;
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (redundantSlash((resolveUrl(target))) == redundantSlash(configIndex.getLocations()[i].path))
            return location = configIndex.getLocations()[i];
    }
    return getExactLocationBasedOnUrlContainer(returnNewPath(target), configIndex);
}

Location Server::getExactLocationBasedOnUrl(std::string target, ConfigData configIndex)
{
    if (target.at(target.size() - 1) != '/')
        target = target + "/";
    return getExactLocationBasedOnUrlContainer(target, configIndex);
}

bool Server::checkAvailability(int fd, Location location)
{
    for (size_t i = 0; i < location.methods.size(); i++)
    {
        std::transform(location.methods[i].begin(), location.methods[i].end(), location.methods[i].begin(), ::toupper);
        if (location.methods[i] == request[fd].getMethod())
        {
            return true;
        }
    }
    return false;
}

int Server::t_stat(std::string path, Location location)
{
    std::string new_path = location.root + path;
    struct stat s;
    if (stat(new_path.c_str(), &s) == 0)
    {
        return (s.st_mode & S_IFDIR) ? 1 : s.st_mode & S_IFREG ? 2
                                                               : -1;
    }
    return -1;
}

int Server::t_stat_wait(std::string path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
    {
        return (s.st_mode & S_IFDIR) ? 1 : s.st_mode & S_IFREG ? 2
                                                               : -1;
    }
    return -1;
}

int Server::helper(int fd, std::string &url, Location location)
{

    if (t_stat(url, location) == 1 && url.at(url.size() - 1) != '/')
    {
        std::string httpRespons = MovedPermanently(getContentType(url), location.path);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1){
            getResponse(fd, 500);
            close(fd), request.erase(fd);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (checkAvailability(fd, location) == false)
    {
        return getResponse(fd, 405);
    }
    return EXIT_FAILURE;
}

int Server::sendFinalReques(int fd, std::string url, Location location, size_t checkState)
{
    if (location.autoindex == true)
    {
        std::string httpRespons;
        location.path = redundantSlash(location.path);
        std::string mime;
        size_t fileSize = listDirectory(url, redundantSlash(resolveUrl(request[fd].state.url)), mime).size();
        httpRespons = httpResponse(mime, fileSize);
        if (send(fd, httpRespons.c_str(), httpRespons.size(), MSG_NOSIGNAL) == -1)
            return getResponse(fd, 500), close(fd), request.erase(fd), checkState = 0, 0;

        if (send(fd, listDirectory(url, redundantSlash(resolveUrl(request[fd].state.url)), mime).c_str(), fileSize, MSG_NOSIGNAL) == -1)
            return getResponse(fd, 500), close(fd), request.erase(fd), checkState = 0, 0;
        return checkState = 0, 200;
    }
    else
    {
        return getResponse(fd, 403);
    }
}

bool Server::timedFunction(int timeoutSeconds, time_t startTime)
{
    time_t currentTime = time(NULL);
    if (difftime(currentTime, startTime) >= timeoutSeconds)
    {
        return false;
    }
    return true;
}

int Server::serve_file_request(int fd, ConfigData configIndex)
{
    std::string Connection = request[fd].connection;
    std::string url = request[fd].state.url;
    Location location = getExactLocationBasedOnUrl(url, configIndex);
    if (location.path.empty() == true)
    {
        return getResponse(fd, 404);
    }
    if (helper(fd, url, location) == EXIT_SUCCESS)
    {
        return 0;
    }
    if (request[fd].state.test == 1)
    {
        if (continueFileTransfer(fd, url, location) == -1)
            return std::cerr << "Failed to continue file transfer" << std::endl, 0;
        return 0;
    }

    size_t checkState;
    bool checkCan = false;
    if ((checkCan = canBeOpen(fd, url, location, checkState, configIndex)))
    {
        if (t_stat_wait(url) == 1 )
        {
            return sendFinalReques(fd, url, location, checkState);
        }
        return (handleFileRequest(fd, url, Connection, location) == -1) ? 0 : 200;
    }
    else
    {
        if (checkState == 301)
        {
            location = getExactLocationBasedOnUrl(url, configIndex);
            if (t_stat_wait(location.root + url) == -1)
                getResponse(fd, 404);
            else
            {
                if (canBeOpen(fd, url, location, checkState, configIndex))
                {
                    if (t_stat_wait(url) == 1){
                        
                        return sendFinalReques(fd, redundantSlash(url), location, checkState);
                    }
                    if (handleFileRequest(fd, url, Connection, location) == 201)
                    {
                        if (timedFunction(TIMEOUT, request[fd].state.last_activity_time) == false)
                            return 310;
                        return 0;
                    }
                }
                else
                {
                    if (checkState == 301)
                    {
                        if (timedFunction(TIMEOUT, request[fd].state.last_activity_time) == false)
                            return 310;
                        else
                        {
                            std::string httpRespons = MovedPermanently(getContentType(url), location.path);
                            if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
                                return getResponse(fd, 500), close(fd), request.erase(fd), std::cerr << "Failed to send HTTP header." << std::endl, EXIT_FAILURE;
                            return 0;
                        }
                    }
                    return getResponse(fd, 404);
                }
            }
        }
        return getResponse(fd, 404);
    }
    return 0;
}
