#include "../server.hpp"

// Read file in chunks with error handling
bool readFileChunk_post(const std::string &path, char *buffer, size_t offset, size_t chunkSize, size_t &bytesRead)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return false;
    }

    file.seekg(offset, std::ios::beg);
    file.read(buffer, chunkSize);
    bytesRead = file.gcount();
    return true;
}

// Send a chunk of data using chunked encoding.
bool sendChunk_post(int fd, const char *data, size_t size)
{
    try
    {
        // Send chunk size in hex
        std::ostringstream chunkHeader;
        chunkHeader << std::hex << size << "\r\n";
        std::string header = chunkHeader.str();

        // Combine chunk header, data, and terminator for atomic send
        std::vector<struct iovec> iov(3);
        iov[0].iov_base = const_cast<char *>(header.c_str());
        iov[0].iov_len = header.length();
        iov[1].iov_base = const_cast<char *>(data);
        iov[1].iov_len = size;
        char terminator[2] = {'\r', '\n'};
        iov[2].iov_base = terminator;
        iov[2].iov_len = 2;

        struct msghdr msg = {};
        msg.msg_iov = iov.data();
        msg.msg_iovlen = iov.size();

        ssize_t sentBytes = sendmsg(fd, &msg, MSG_NOSIGNAL);
        return sentBytes != -1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in sendChunk_post: " << e.what() << std::endl;
        return false;
    }
}

// Send the final empty chunk to indicate end of chunked transfer
bool sendFinalChunk_post(int fd)
{
    return sendChunk_post(fd, "", 0) &&
           send(fd, "\r\n", 2, MSG_NOSIGNAL) != -1;
}

int redirectTheParh(std::vector<char> buffer, std::string filePath, size_t bytesRead)
{
    // Define the redirect path
    const std::string REDIRECTPATH = PATHU;

    // Ensure the directory exists
    struct stat st;
    if (stat(REDIRECTPATH.c_str(), &st) != 0)
    { // Check if path exists
        if (mkdir(REDIRECTPATH.c_str(), 0777) != 0)
        { // Create directory
            std::cerr << "Failed to create directory: " << REDIRECTPATH
                      << " (" << strerror(errno) << ")" << std::endl;
            return -1;
        }
    }

    // Extract filename from the original path
    std::string filename = filePath.substr(filePath.find_last_of("/\\") + 1);

    // Construct the full new path
    std::string newFilePath = REDIRECTPATH + "/" + filename;

    // Save the file to the redirected path
    std::ofstream localFile(newFilePath.c_str(), std::ios::binary);
    if (!localFile)
    {
        std::cerr << "Failed to create file in redirected path: " << newFilePath << std::endl;
        return -1;
    }
    localFile.write(buffer.data(), bytesRead);
    localFile.close();
    std::cout << "File successfully saved to: " << newFilePath << std::endl;
    return 0;
}

int continueFileTransferPost(int fd, Server *server)
{
    std::map<int, FileTransferState>::iterator transferIt = server->fileTransfers.find(fd);
    // if (transferIt == server->fileTransfers.end())
    //     return std::cerr << "No file transfer in progress for fd: " << fd << std::endl, -1;

    FileTransferState &state = transferIt->second;
    if (state.isComplete)
        return server->fileTransfers.erase(transferIt), 0;

    char buffer[CHUNK_SIZE];
    size_t remainingBytes = state.fileSize - state.offset;
    size_t bytesToRead = std::min(remainingBytes, static_cast<size_t>(CHUNK_SIZE));
    size_t bytesRead = 0;

    if (!readFileChunk_post(state.filePath, buffer, state.offset, bytesToRead, bytesRead))
    {
        std::cerr << "Failed to read chunk from file: " << state.filePath << std::endl;
        server->fileTransfers.erase(transferIt);
        return -1;
    }

    if (!sendChunk_post(fd, buffer, bytesRead))
    {
        std::cerr << "Failed to send chunk." << std::endl;
        server->fileTransfers.erase(transferIt);
        return -1;
    }

    state.offset += bytesRead;
    // Check if we've sent the entire file
    if (state.offset >= state.fileSize)
    {
        if (!sendFinalChunk_post(fd))
        {
            std::cerr << "Failed to send final chunk." << std::endl;
            server->fileTransfers.erase(transferIt);
            return -1;
        }

        state.isComplete = true;
        server->fileTransfers.erase(transferIt);
    }
    std::vector<char> bufferVector(buffer, buffer + bytesRead);
    return redirectTheParh(bufferVector, server->fileTransfers[fd].filePath, bytesRead);
}

int handleFileRequest_post(int fd, Server *server, const std::string &filePath)
{
    try
    {
        std::string contentType = server->getContentType(filePath);
        size_t fileSize = server->getFileSize(filePath);

        if (fileSize > server->LARGE_FILE_THRESHOLD)
        {
            std::string httpResponse = server->createChunkedHttpResponse(contentType);
            if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            {
                std::cerr << "Failed to send chunked HTTP header." << std::endl;
                return -1;
            }

            FileTransferState state;
            state.filePath = filePath;
            state.fileSize = fileSize;
            state.offset = 0;
            state.isComplete = false;
            server->fileTransfers[fd] = state;

            return continueFileTransferPost(fd, server);
        }
        else
        {
            std::string httpResponse = server->httpResponse(contentType, fileSize);

            if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
            {
                std::cerr << "Failed to send HTTP header." << std::endl;
                return -1;
            }

            std::vector<char> buffer(fileSize);
            size_t bytesRead = 0;

            if (!readFileChunk_post(filePath, buffer.data(), 0, fileSize, bytesRead))
            {
                std::cerr << "Failed to read file: " << filePath << std::endl;
                return -1;
            }
            return redirectTheParh(buffer, filePath, bytesRead);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in handleFileRequest_post: " << e.what() << std::endl;
        return -1;
    }
}



void createFileName(std::string line, Server *server, int fd) 
{
    size_t start = line.find("filename=\"");
    
    start += 10;
    size_t end = line.find("\"", start);
   
    std::string fileName = server->fileTransfers[fd].filePath + line.substr(start, end - start);
    std::ofstream *newFile = new std::ofstream(fileName.c_str(), std::ios::binary | std::ios::trunc);
    if (!newFile->is_open())
    {
        std::cerr << "Error: Failed to open file: " << fileName << std::endl;
        delete newFile;
        //[soukaina] here what should i do ???? quit the program ! 
        return;
    }

    server->fileTransfers[fd].multp.outFiles.push_back(newFile);
    server->fileTransfers[fd].multp.currentFileIndex = server->fileTransfers[fd].multp.outFiles.size() - 1;
   
    std::cout << "Opened file #" << server->fileTransfers[fd].multp.currentFileIndex 
              << ": " << fileName << std::endl;
}

void Server::writeData(Server* server, Binary_String& chunk, int fd)
{
    FileTransferState& state = server->fileTransfers[fd];
    
    std::string boundaryEnd = "--" + state.multp.boundary;

    state.multp.partialHeaderBuffer += chunk.to_string();
    while (true)
    {
        if (state.multp.isInHeader) 
        {
            size_t headerEnd = state.multp.partialHeaderBuffer.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
                break;
            std::string headers = state.multp.partialHeaderBuffer.substr(0, headerEnd);
            size_t namePos = headers.find("filename=\"");
            if (namePos != std::string::npos)
                createFileName(headers, server, fd);
            state.multp.partialHeaderBuffer.erase(0, headerEnd + 4);
            state.multp.isInHeader = false;
        }
        else
        {
            size_t boundaryPos = state.multp.partialHeaderBuffer.find(boundaryEnd);
            if (boundaryPos == std::string::npos)
            {
                if (!state.multp.outFiles.empty())
                {
                    std::ofstream *file = state.multp.outFiles.back();
                    file->write(state.multp.partialHeaderBuffer.data(), 
                                state.multp.partialHeaderBuffer.size());
                }
                state.multp.partialHeaderBuffer.clear();
                break;
            }
            else
            {
                if (!state.multp.outFiles.empty())
                {
                    std::ofstream* file = state.multp.outFiles.back();
                    file->write(state.multp.partialHeaderBuffer.data(), boundaryPos);
                }
                state.multp.partialHeaderBuffer.erase(0, boundaryPos + boundaryEnd.size());
                state.multp.isInHeader = true;
            }
        }
    }
}

std::pair<size_t, std::string> Server::returnTargetFromRequest(std::string header, std::string target)
{
    std::pair<size_t, std::string> pair_target;
    std::set<std::string> node;
    char *result = strstr((char *)header.c_str(), (char *)target.c_str());
    std::string reachTarget = result;
    reachTarget = reachTarget.substr(reachTarget.find(" ", 0), reachTarget.length());
    pair_target.first = static_cast<size_t>(atoi((reachTarget.substr(reachTarget.find(" ", 0), reachTarget.length())).c_str())); 
    pair_target.second = reachTarget.substr(reachTarget.find(" ", 0), reachTarget.length());
    return pair_target;
}

bool checkEndPoint(std::string &filePath)
{
    struct stat info;
   
    if (stat(filePath.c_str(), &info) != 0)
        return false;   
    if (S_ISDIR(info.st_mode))
    {
        return (true);
    }
    return (false);
}

int Server::parsePostRequest(Server *server, int fd, std::string header)
{
    std::string contentType;
    std::string filePath;
    FileTransferState &state = server->fileTransfers[fd];
    size_t boundaryStart;

    // [soukaina] here i have to check if the content length is 0 so i can threw an error
    // no centent check if == 0
    /*std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";*/
    /*std::cout << header;*/
    /*std::cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";*/
    // if (server->key_value_pair_header(header, "Content-Length:"). == "")
    // std::map<std::string, std::string>::iterator it = server->fileTransfers[fd].mapOnHeader.find("Connection:");
    // if (it != server->fileTransfers[fd].mapOnHeader.end()){

    // }
    if (state.mapOnHeader.find("Content-Length:")->second == "")
        return getSpecificRespond(fd, server, "400.html", server->createBadResponse);
    contentType = state.mapOnHeader.find("Content-Type:")->second;  
    filePath = server->parseRequest(header, server); 
    if (contentType.find("multipart/form-data") == std::string::npos)
        return getSpecificRespond(fd, server, "415.html", server->createUnsupportedMediaResponse);
    if (contentType.find("boundary=") != std::string::npos) 
    {
        boundaryStart = contentType.find("boundary=") + 9;         
        state.multp.boundary = contentType.substr(boundaryStart, contentType.length());
        if (state.multp.boundary.length() == 0)
            return getSpecificRespond(fd, server, "400.html", server->createBadResponse);
    }
    if (checkEndPoint(state.filePath) == false)
        getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
    return (0);
}

int Server::handlePostRequest(int fd, Server *server, Binary_String request)
{
    int exitCode;
    std::pair <Binary_String, Binary_String> pairRequest;

    pairRequest = ft_parseRequest_T(fd, server, request);
    exitCode = 0;
    if (server->fileTransfers[fd].multp.containHeader == true)
    {
        server->key_value_pair_header(fd, server, pairRequest.first.to_string());
        exitCode = parsePostRequest(server, fd, pairRequest.first.to_string());
        server->fileTransfers[fd].multp.containHeader = false;
        if (!pairRequest.second.empty())
            writeData(server, pairRequest.second, fd);       
    }
    // [soukaina]  after writing all data in the fd i should turn the containHeader to true
    // or i have just to ereas the fd from the fileTransfers
    else
        writeData(server, request, fd);
    return exitCode;
}
