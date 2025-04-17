/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 15:08:56 by mmad              #+#    #+#             */
/*   Updated: 2025/04/17 23:11:53 by mmad             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../server.hpp"

// Helper function to get file size
std::ifstream::pos_type Server::getFileSize(const std::string &path)
{
    std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return 0;

    return file.tellg();
}

// Read file in chunks
bool readFileChunk(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    // std::cout << "[" << path.c_str() << "]" << std::endl;
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
        new_path = PATHC + filePath;
    std::ifstream file(new_path.c_str());
    if (!file.is_open())
        return std::cerr << "Failed to open file:: " << filePath << std::endl, false;
    filePath = new_path;
    return true;
}

bool sendChunk(int fd, const char *data, size_t size)
{
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << size << "\r\n";
    std::string header = chunkHeader.str();

    send(fd, header.c_str(), header.length(), MSG_NOSIGNAL);

    // Send chunk data
    send(fd, data, size, MSG_NOSIGNAL);

    // Send chunk terminator
    send(fd, "\r\n", 2, MSG_NOSIGNAL);
    return true;
}

// Send the final empty chunk to indicate end of chunked transfer
bool sendFinalChunk(int fd)
{
    return sendChunk(fd, "", 0) &&
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

// Continue sending chunks for an in-progress file transfer
int Server::continueFileTransfer(int fd, Server *server, std::string Connection)
{
    if (server->fileTransfers.find(fd) == server->fileTransfers.end())
        return std::cerr << "No file transfer in progress for fd: " << fd << std::endl, 0;

    FileTransferState &state = server->fileTransfers[fd];
    if (state.isComplete == true)
    {
        // std::cout << "i was here" << std::endl;
        time_t current_time = time(NULL);
        if (current_time - state.last_activity_time > TIMEOUT)
        {
            std::cerr << "Client " << fd << " timed out." << std::ends;
            state.isComplete = false;
            state.last_activity_time = 0;
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
        if (Connection != " keep-alive")
            return server->fileTransfers.erase(fd), close(fd), 0;
        return state.isComplete = true, state.last_activity_time = time(NULL), 0;
    }

    if (!sendChunk(fd, buffer, bytesRead))
    {
        if (Connection != " keep-alive")
            return server->fileTransfers.erase(fd), close(fd), 0;
        return state.isComplete = true, state.last_activity_time = time(NULL), 0;
    }

    state.offset += bytesRead;

    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            if (Connection != " keep-alive")
                return server->fileTransfers.erase(fd), close(fd), 0;
            state.isComplete = true;
            state.last_activity_time = time(NULL);
            return 0;
        }

        if (Connection != " keep-alive")
            return close(fd), server->fileTransfers.erase(fd), 0;
        state.isComplete = true;
        state.last_activity_time = time(NULL);
    }
    return 0;
}

// Handle initial file request
int Server::handleFileRequest(int fd, Server *server, const std::string &filePath, std::string Connection)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = server->getFileSize(filePath);

    // Determine if file is large enough to warrant chunked encoding
    const size_t LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB

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
        FileTransferState state;
        state.isComplete = false;
        std::string httpResponse = server->httpResponse(contentType, fileSize);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;

        // Read and send the entire file at once
        char buffer[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(filePath, buffer, 0, fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << filePath << std::endl, 0;

        if (send(fd, buffer, bytesRead, MSG_NOSIGNAL) == -1)
        {
            close(fd), server->fileTransfers.erase(fd), 0;
        }

        if (Connection != " keep-alive")
        {
            return close(fd), server->fileTransfers.erase(fd), 0;
        }
        else
        {
            state.isComplete = true;
            state.last_activity_time = time(NULL);
        }
        return 0;
    }
}

// Original readFile function - kept for error pages
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

int Server::serve_file_request(int fd, Server *server, std::string request)
{
    std::pair<std::string, std::string> pair_request = server->ft_parseRequest(request);
    std::string Connection = server->key_value_pair_header(pair_request.first, "Connection:");
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
    {
        return server->continueFileTransfer(fd, server, Connection);
    }

    std::string filePath = server->parseRequest(request, server);
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
