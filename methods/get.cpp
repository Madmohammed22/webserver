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

bool check(std::string filePath){
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return false;
    return true;
}
bool Server::canBeOpen(int fd, std::string &filePath, Location location)
{
    if (filePath == location.path)
    {
        if (location.redirect.size() > 0)
        {
            request[fd].flag = 1;
            filePath = location.root + location.redirect;
            return check(filePath);
        }
        else if (location.index.size() > 0){
            filePath = location.root + "/" + location.index[0];
            return check(filePath) && location.autoindex;
        }
    }
    else
    {
        std::cout << "[" << location.path << ", " << filePath << "]" << std::endl;
        // if (location.path == filePath)
        filePath = location.root + filePath;
    }

    return check(filePath);
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

int Server::continueFileTransfer(int fd, std::string filePath, Location configIndex)
{
    (void)configIndex;
    // canBeOpen(filePath, configIndex);
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

int Server::handleFileRequest(int fd, const std::string &filePath, std::string Connection, Location configIndex)
{
    request[fd].state.filePath = filePath;
    std::string contentType = Server::getContentType(filePath);
    request[fd].state.fileSize = getFileSize(filePath);
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
        if (configIndex.redirect.size() > 0 && request[fd].flag == 1)
        {
            request[fd].flag = 0;
            httpRespons = MovedPermanently(contentType, configIndex.redirect);
        }
        else
            httpRespons = httpResponse(contentType, request[fd].state.fileSize);
        if (send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;
        if (send(fd, readFile(filePath).c_str(), request[fd].state.fileSize, MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send file content." << std::endl, -1;
        if (send(fd, "\r\n\r\n", 4, MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send final CRLF." << std::endl, -1;
        if (Connection == "close" || Connection.empty())
            return request[fd].state.isComplete = true,  close(fd), request.erase(fd), 0;
        return request[fd].state.isComplete = true, close(fd), request.erase(fd), 0;
    }
    return 0;
}

std::string Server::readFile(std::string path)
{
    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

Location Server::getExactLocationBasedOnUrl(std::string target, ConfigData configIndex)
{
    for (size_t i = 0; i < configIndex.getLocations().size(); i++)
    {
        if (target == configIndex.getLocations()[i].path)
        {
            return configIndex.getLocations()[i];
        }
    }
    return configIndex.getLocations()[0];
}

bool Server::checkAvailability(int fd, Location location){
    for (size_t i = 0; i < location.methods.size(); i++){
        std::transform(location.methods[i].begin(), location.methods[i].end(), location.methods[i].begin(), ::toupper);
        if (location.methods[i] == request[fd].getMethod())
            return true;
    }
    return false;
}

int Server::serve_file_request(int fd, ConfigData configIndex)
{
    std::string Connection = request[fd].connection;
    std::string filePath = request[fd].state.filePath;
    Location location = getExactLocationBasedOnUrl(filePath, configIndex);
    if (checkAvailability(fd,location) == false)
        return getSpecificRespond(fd, configIndex.getErrorPages().find(405)->second, methodNotAllowedResponse);
        
    if (request[fd].state.test == 1)
    {
        if (continueFileTransfer(fd, filePath, location) == -1)
            return std::cerr << "Failed to continue file transfer" << std::endl, 0;
        return 0;
    }
    if (canBeOpen(fd, filePath, location))
    {
        if (handleFileRequest(fd, filePath, Connection, location) == -1)
            return close(fd), request.erase(fd), 0;
        return 0;
    }
    else
    {
        return getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);
    }
    return 0;
}