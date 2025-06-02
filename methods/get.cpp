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
    state.filePath = Server::parseSpecificRequest(request.header);
    state.offset = 0;
    state.fileSize = Server::getFileSize(state.filePath);
    state.isComplete = false;
    state.mime = Server::getContentType(state.filePath);
    state.uriLength = state.filePath.length();
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

bool Server::check(std::string filePath)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
    {
        return false;
    }
    return true;
}

bool matchPath(std::string filePath, Location location)
{
    if (filePath == "/")
        return false;

    if (filePath == location.path)
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
    std::cout << "dir: " << dir_path << std::endl;
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
                // std::cout << "[" << file_name << "]-> " << "[" << ent->d_name << "]" << std::endl;
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
            std::cout << "I was here" << std::endl;
            return false;
        }
    }
    return true;
}
bool Server::canBeOpen(int fd, std::string &filePath, Location location, size_t &checkState)
{

    if (filePath == location.path)
    {
        if (location.redirect.size() > 0)
        {
            request[fd].flag = 1;
            filePath = redundantSlash(location.redirect);
            checkState = 301;
            return check(location.root + location.path + filePath);
        }
        else if (location.index.size() > 0 && validateSearch(location.index, location.root + filePath) == true)
        {
            filePath = redundantSlash(location.root + location.path + fetchIndex(location.root + "/", location.index));
            // filePath = location.root + "/large.pdf";
            std::cout << "filePath-> " << filePath << std::endl;
            return check(filePath);
        }
        else
        {
            filePath = location.root + filePath;
            checkState = !location.autoindex ? 403 : 201;
            return checkState == 403 ? false : true;
        }
    }
    else
    {
        filePath = location.root + filePath;
        checkState = (check(filePath) == true) ? 200 : 404;
        return (checkState == 200) ? true : false;
    }

    return check(filePath);
}

bool sendChunk(int fd, const char *data, size_t size)
{
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string data_size = chunkHeader.str();
    int faild;
    faild = send(fd, data_size.c_str(), data_size.length(), MSG_NOSIGNAL);
    faild = send(fd, data, size, MSG_NOSIGNAL);
    faild = send(fd, "\r\n", 2, MSG_NOSIGNAL);
    return (faild == -1) ? false : true;
}

bool sendFinalChunk(int fd)
{
    return sendChunk(fd, "", 0) &&
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

int Server::continueFileTransfer(int fd, std::string filePath, Location location)
{
    (void)location;
    char buffer[CHUNK_SIZE];
    size_t remainingBytes = request[fd].state.fileSize - request[fd].state.offset;
    size_t bytesToRead;
    remainingBytes > CHUNK_SIZE ? bytesToRead = CHUNK_SIZE : bytesToRead = remainingBytes;
    size_t bytesRead = 0;
    if (!readFileChunk(filePath, buffer, request[fd].state.offset, bytesToRead, bytesRead))
    {
        return request[fd].getConnection() == "close" ? -1 : 0;
    }

    if (!sendChunk(fd, buffer, bytesRead))
    {
        return request[fd].getConnection() == "close" ? -1 : 0;
    }
    request[fd].state.offset += bytesRead;

    if (request[fd].state.offset >= request[fd].state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            return request[fd].getConnection() == "close" ? -1 : 0;
        }
        if (request[fd].getConnection() == "close")
        {
            return request[fd].state.test = 0, 0;
        }
        request[fd].state.test = 0;
    }
    return 0;
}

int Server::handleFileRequest(int fd, const std::string &filePath, std::string Connection, Location location)
{
    request[fd].state.filePath = filePath;
    std::string contentType = Server::getContentType(filePath);
    request[fd].state.fileSize = getFileSize(filePath);
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1mb
    if (request[fd].state.fileSize > LARGE_FILE_THRESHOLD)
    {
        request[fd].state.test = 1;
        std::string httpRespons = createChunkedHttpResponse(contentType);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, 0;
        return continueFileTransfer(fd, request[fd].state.filePath, location);
    }
    else
    {

        std::string httpRespons;
        if (location.redirect.size() > 0 && request[fd].flag == 1)
        {
            request[fd].flag = 0;
            httpRespons = MovedPermanently(contentType, redundantSlash(location.path + location.redirect));
            int faild = send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL);
            if (faild == -1){
                return close(fd), request.erase(fd), 0;
            }
            return 0;
        }
        else
            httpRespons = httpResponse(contentType, request[fd].state.fileSize);
        int faild = send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL);
        faild = send(fd, readFile(filePath).c_str(), request[fd].state.fileSize, MSG_NOSIGNAL);
        faild = send(fd, "\r\n\r\n", 4, MSG_NOSIGNAL);
        if (faild == -1)
            return close(fd), request.erase(fd), 0;
        if (Connection == "close" || Connection.empty())
            return request[fd].state.isComplete = true, close(fd), request.erase(fd), 0;
        return request[fd].state.isComplete = true, close(fd), request.erase(fd), 0;
    }
    return 0;
}

std::string Server::readFile(std::string path)
{

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file:: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

Location findRoot(ConfigData configIndex, std::string path)
{
    Location location;
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (path == "/"){
            if (path == configIndex.getLocations()[i].path)
            {
                return configIndex.getLocations()[i];;
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
        if ("/" == configIndex.getLocations()[i].path){
            location = configIndex.getLocations()[i]; 
            return  location;
        }
    }
    return location;
}
Location getExactLocationBasedOnUrlContainer(std::string target, ConfigData configIndex)
{
    Location location;
    if (target == "/" && !returnDefault(configIndex).path.empty()){
        return returnDefault(configIndex);
    }
    if (returnDefault(configIndex).path.empty())
        return location;
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (target == configIndex.getLocations()[i].path)
        {
            return location = configIndex.getLocations()[i];
        }
    }
    return getExactLocationBasedOnUrlContainer(returnNewPath(target), configIndex);
}

void Server::addSlashBasedOnMethod(std::string &target, std::string method)
{
    if (method == "DELETE")
    {
        if (target.at(target.size() - 1) != '/')
            target = target + "/";
    }
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


int Server::helper(int fd, std::string &filePath, ConfigData configIndex, Location location)
{

    if (t_stat(filePath, location) == 1 && filePath.at(filePath.size() - 1) != '/')
    {
        std::string httpRespons = MovedPermanently(getContentType(filePath), location.path);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
    if (checkAvailability(fd, location) == false)
    {
        return getSpecificRespond(fd, configIndex.getErrorPages().find(405)->second, methodNotAllowedResponse), EXIT_FAILURE;
    }
    return EXIT_FAILURE;
}

// try to find url inside 
int Server::serve_file_request(int fd, ConfigData configIndex)
{
    std::string Connection = request[fd].connection;
    std::string filePath = request[fd].state.filePath;
    Location location = getExactLocationBasedOnUrl(filePath, configIndex);
    if (location.path.empty() == true){
        return getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);
    }
    if (helper(fd, filePath, configIndex, location) == EXIT_SUCCESS){
        return 0;
    }
    if (request[fd].state.test == 1)
    {
        if (continueFileTransfer(fd, filePath, location) == -1)
            return std::cerr << "Failed to continue file transfer" << std::endl, 0;
        return 0;
    }
    size_t checkState;
    if (canBeOpen(fd, filePath, location, checkState))
    {
        std::cout << "get location: " << filePath << std::endl;

        if (checkState == 201)
        {
            std::string mime;
            size_t fileSize = listDirectory(filePath, request[fd].state.filePath, mime).size();
            std::string httpRespons = httpResponse(mime, fileSize);
            int faild = send(fd, httpRespons.c_str(), httpRespons.size(), MSG_NOSIGNAL);
            faild = send(fd, listDirectory(filePath, request[fd].state.filePath, mime).c_str(), fileSize, MSG_NOSIGNAL);
            if (faild == -1)
                return close(fd), request.erase(fd), checkState = 0, 0;
            return close(fd), checkState = 0, 0;
        }
        return (handleFileRequest(fd, filePath, Connection, location) == -1) ? ((close(fd), request.erase(fd)) && 0) : 0;
    }
    else
    {
        std::cout << "location :" << location.path << std::endl;
        if (checkState == 301){
            location = getExactLocationBasedOnUrl(filePath, configIndex);
            std::cout << "filepath [301]: " << filePath << "|location: " << t_stat(filePath, location) << std::endl;
            if (t_stat(filePath, location) == -1)
                getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);
        }
        size_t tmp = checkState;
        checkState = 0;
        return (tmp == 404) ? getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse)
                            : getSpecificRespond(fd, configIndex.getErrorPages().find(403)->second, Forbidden);
    }
    return 0;
}