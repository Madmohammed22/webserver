/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 15:08:56 by mmad              #+#    #+#             */
/*   Updated: 2025/04/21 16:40:13 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../server.hpp"

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
        return false;

    file.seekg(offset);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();

    return true;
}

bool Server::canBeOpen(std::string &filePath)
{

    std::string new_path;
    if (filePath.at(0) != '/' && getFileType("/" + filePath) == 2)
        new_path = "/" + filePath;
    else if (filePath.at(0) != '/' && getFileType("/" + filePath) == 1)
        return true;
    else
    {
        new_path = STATIC + filePath;
        // new_path = PATHC + filePath;
    }
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
        return std::cerr << "" << std::ends, false;
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


// Continue sending chunks for an in-progress file transfer
int Server::continueFileTransfer(int fd, Server *server, std::string Connection)
{
    if (server->fileTransfers.find(fd) == server->fileTransfers.end())
        return std::cerr << "No file transfer in progress for fd: " << fd << std::endl, close(fd), 0;

    FileTransferState &state = server->fileTransfers[fd];
    if (state.isComplete == true)
    {
        time_t current_time = time(NULL);
        if (current_time - state.last_activity_time > TIMEOUT)
        {
            std::cerr << "Client " << fd << " timed out." << "[continueFileTransfer]" << std::ends;
            return close(fd), server->fileTransfers.erase(fd), 0;
        }
        return 0;
    }

    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead;
    if (remainingBytes > CHUNK_SIZE)
        bytesToRead = CHUNK_SIZE;
    else
        bytesToRead = remainingBytes;
    size_t bytesRead = 0;
    if (!readFileChunk(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        if (Connection != "keep-alive")
            return server->fileTransfers.erase(fd), close(fd), 0;
        return state.isComplete = true, state.last_activity_time = time(NULL), 0;
    }

    if (!sendChunk(fd, buffer, bytesRead))
    {
        if (Connection != "keep-alive")
            return server->fileTransfers.erase(fd), close(fd), 0;
        return state.isComplete = true, state.last_activity_time = time(NULL), 0;
    }

    state.offset += bytesRead;

    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            if (Connection != "keep-alive")
                return server->fileTransfers.erase(fd), close(fd), 0;
            state.isComplete = true;
            state.last_activity_time = time(NULL);
            return 0;
        }

        if (Connection != "keep-alive")
            return close(fd), server->fileTransfers.erase(fd), 0;
        state.isComplete = true;
        state.last_activity_time = time(NULL);
    }
    return 0;
}

int Server::handleFileRequest(int fd, Server *server, const std::string &filePath, std::string Connection)
{
    
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = server->getFileSize(filePath);
    
    FileTransferState &state = server->fileTransfers[fd];
    if (state.isComplete == true)
    {
        time_t current_time = time(NULL);
        if (current_time - state.last_activity_time > TIMEOUT)
        {
            std::cerr << "Client " << fd << " timed out." << "[continueFileTransfer]" << std::ends;
            return close(fd), server->fileTransfers.erase(fd), 0;
        }
        return 0;
    }

    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024;

    if (fileSize > LARGE_FILE_THRESHOLD)
    {
        std::string httpResponse = server->createChunkedHttpResponse(contentType);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send chunked HTTP header." << std::endl, -1;
        FileTransferState state;
        state.filePath = filePath;
        state.fileSize = fileSize;
        state.offset = 0;
        state.isComplete = false;
        server->fileTransfers[fd] = state;
        state.saveFd = fd;
        return server->continueFileTransfer(fd, server, Connection);
    }
    else
    {
        std::string httpResponse = server->httpResponse(contentType, fileSize);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;
        FileTransferState state;
        state.isComplete = false;
        server->fileTransfers[fd] = state;
        char buffer[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(filePath, buffer, 0, fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << filePath << std::endl, 0;

        if (send(fd, buffer, bytesRead, MSG_NOSIGNAL) == -1)
        {
            close(fd), server->fileTransfers.erase(fd), 0;
        }

        if (Connection != "keep-alive")
            return close(fd), server->fileTransfers.erase(fd), 0;
        else
        {
            state.last_activity_time = time(NULL);
            state.isComplete = true;
        }
        return 0;
    }
}

std::string Server::readFile(const std::string &path)
{
    if (path.empty())
        return "";

    std::ifstream infile(path.c_str(), std::ios::binary);
    if (!infile)
        return std::cerr << "Failed to open file: " << path << std::endl, "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    return oss.str();
}

int Server::serve_file_request(int fd, Server *server, std::string request, std::map<int, Client> &client)
{
    (void)client;
    server->key_value_pair_header(fd, server, ft_parseRequest_T(fd, server, request).first);
    std::string Connection = server->fileTransfers[fd].mapOnHeader.find("Connection:")->second;
    std::string filePath = server->parseSpecificRequest(request, server);
    if (server->canBeOpen(filePath) && server->getFileType(filePath) == 2)
    {
        return server->handleFileRequest(fd, server, filePath, Connection);
    }
    else
    {
        FileTransferState state;
        state.typeOfConnection = Connection;
        server->fileTransfers[fd] = state;
        return getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
    }
    return 0;
}
