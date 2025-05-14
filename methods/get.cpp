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

/*
bool Server::canBeOpen(std::string &filePath)
{

    std::string new_path;
    if (filePath.at(0) != '/' && getFileType("/" + filePath) == 2)
        new_path = "/" + filePath;
    else if (filePath.at(0) != '/' && getFileType("/" + filePath) == 1)
        return true;
    else
        new_path = TEST + filePath;
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
        return std::cerr << "" << std::ends, false;
    filePath = new_path;
    return true;
}
 */

bool Server::canBeOpen(std::string &filePath, ConfigData configIndex)
{

    std::string new_path;
    bool check = filePath == "/" && !configIndex.getLocations()[0].redirect.empty();
    if (check)
    {
        new_path = configIndex.getLocations()[0].root + configIndex.getLocations()[0].redirect;
    }
    else if (filePath != "/" && configIndex.getLocations()[0].redirect.empty()){
        new_path = configIndex.getLocations()[0].root + "/" + configIndex.getLocations()[0].index[0];        
    }
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
        return false;
    filePath = new_path;
    return true;
}

bool sendChunk(int fd, const char *data, size_t size)
{
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string header = chunkHeader.str();

    if (send(fd, header.c_str(), header.length(), MSG_NOSIGNAL) == -1)
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

int Server::continueFileTransfer(int fd, std::string filePath, ConfigData configIndex)
{
    canBeOpen(filePath, configIndex);
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

int Server::handleFileRequest(int fd, const std::string &filePath, std::string Connection, ConfigData configIndex)
{
    request[fd].state.filePath = filePath;
    std::string contentType = request[fd].state.mime;
    request[fd].state.fileSize = getFileSize(request[fd].state.filePath);
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024;

    if (request[fd].state.fileSize > LARGE_FILE_THRESHOLD)
    {
        request[fd].state.test = 1;
        std::string httpRespons = createChunkedHttpResponse(contentType);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
        {
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, 0;
        }
        return continueFileTransfer(fd, request[fd].state.filePath, configIndex);
    }
    else
    {
        std::string httpRespons;
        if (!configIndex.getLocations()[0].redirect.empty()){
            std::cout << "[" << request[fd].state.filePath << "]" << std::endl;
            std::string location_ = "http://" + configIndex.getHost() + configIndex.getLocations()[0].redirect;
            httpRespons = MovedPermanently(Server::getContentType(location_), location_);
        }
        else{
            std::cout << "(" << filePath << ")" << std::endl;
            std::string location_ = configIndex.getLocations()[0].index[0];            
            httpRespons = httpResponse(contentType, request[fd].state.fileSize);
        }
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;
        char buffer[request[fd].state.fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(request[fd].state.filePath, buffer, 0, request[fd].state.fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << request[fd].state.filePath << std::endl, -1;

        if (send(fd, buffer, bytesRead, MSG_NOSIGNAL) == -1)
            return -1;

        if (Connection == "close")
            return close(fd), request.erase(fd), 0;
        close(fd), request.erase(fd);
        return 0;
    }
    return 0;
}

std::string Server::readFile(const std::string &path)
{
    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

int Server::serve_file_request(int fd, ConfigData configIndex)
{
    std::string Connection = request[fd].connection;
    std::string filePath = request[fd].state.filePath;
    if (request[fd].state.test == 1)
    {
        if (continueFileTransfer(fd, request[fd].state.filePath, configIndex) == -1)
            return std::cerr << "Failed to continue file transfer" << std::endl, 0;
        return 0;
    }
    if (canBeOpen(filePath, configIndex) && getFileType(filePath) == 2)
    {
        if (handleFileRequest(fd, filePath, Connection, configIndex) == -1)
            return 0;
        return 0;
    }
    else{
        return getSpecificRespond(fd, this, "404.html", createNotFoundResponse);
    }
    return 0;
}