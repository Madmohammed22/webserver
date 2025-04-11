/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   get.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mmad <mmad@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/09 15:08:56 by mmad              #+#    #+#             */
/*   Updated: 2025/04/11 10:59:48 by mmad             ###   ########.fr       */
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
        return std::cerr << "Failed to open file: " << filePath << std::endl, false;
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
    if (state.isComplete == true){
        time_t current_time = time(NULL);
        if (current_time - state.last_activity_time > TIMEOUT){
            close(fd);
            server->fileTransfers.erase(fd);
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
        std::cerr << "Failed to read chunk from file::::: " << state.filePath << std::endl;
        return server->fileTransfers.erase(fd), close(fd), 0;
    }

    if (!sendChunk(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        return server->fileTransfers.erase(fd), close(fd), 0;
    }

    state.offset += bytesRead;

    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk(fd))
        {
            std::cerr << "Failed to send final chunk." << std::endl;
            return server->fileTransfers.erase(fd), close(fd), 0;
        }
    
        state.isComplete = true;
        if (Connection != " keep-alive"){
            close(fd);
            server->fileTransfers.erase(fd);
        }
        state.last_activity_time = time(NULL);
    }

    return 0;
}

// Handle initial file request
int Server::handleFileRequest(int fd, Server *server, const std::string &filePath, std::string Connection)
{
    std::string contentType = server->getContentType(filePath);
    size_t fileSize = server->getFileSize(filePath);

    if (fileSize == 0)
        return std::cerr << "Failed to get file size or empty file: " << filePath << std::endl, -1;

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
        // state.last_activity_time = time(NULL); // Initialize this!
        server->fileTransfers[fd] = state;
        state.saveFd = fd;
        return server->continueFileTransfer(fd, server, Connection);
    }
    else
    {
        FileTransferState state;
        // Use standard Content-Length for smaller files
        std::string httpResponse = server->httpResponse(contentType, fileSize);
        if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            return std::cerr << "Failed to send HTTP header." << std::endl, -1;

        // Read and send the entire file at once
        char *buffer = new char[fileSize];
        size_t bytesRead = 0;
        if (!readFileChunk(filePath, buffer, 0, fileSize, bytesRead))
            return std::cerr << "Failed to read file: " << filePath << std::endl, delete[] buffer, -1;

        send(fd, buffer, bytesRead, MSG_NOSIGNAL);
        
        delete[] buffer;
        if (Connection != " keep-alive"){
            server->fileTransfers.erase(fd);
        }
        else{
            // state.isCompleteShortFile = true;
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
    std::string Connection = server->key_value_pair_header(pair_request.first,"Connection:");

    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
        return server->continueFileTransfer(fd, server, Connection);
    
    std::string filePath = server->parseRequest(request,server);
    if (server->canBeOpen(filePath) && server->getFileType(filePath) == 2)
        return server->handleFileRequest(fd, server, filePath, Connection);
    else
        return getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
    return 0;
}
