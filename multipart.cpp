#include "server.hpp"

Binary_String Server::readFileChunk_post(int fd)
{
    std::ifstream *file;

    file = request[fd].multp.file;
    if (request[fd].multp.readPosition == 0)
    {
        request[fd].multp.file = new std::ifstream();
        file = request[fd].multp.file;
        file->open(request[fd].state.fileName.c_str());
        if (!file->is_open())
        {
            std::cerr << "File is not open for reading" << std::endl;
            return Binary_String();
        }
    }
    
    if (!file->is_open())
    {
        std::cerr << "File is not open for reading" << std::endl;
        return Binary_String();
    }

    char *buffer = new char[CHUNK_SIZE];
    file->read(buffer, CHUNK_SIZE);
    size_t bytesRead = file->gcount();
    
    request[fd].multp.readPosition += bytesRead;
    
    Binary_String chunk(buffer, bytesRead);
    delete[] buffer;
    return chunk;
}

void Server::createFileName(std::string line, int fd) 
{
    size_t start = line.find("filename=\"");
    
    start += 10;
    size_t end = line.find("\"", start);
   
    std::string fileName = request[fd].state.url + line.substr(start, end - start);
    std::ofstream *newFile = new std::ofstream(fileName.c_str(), std::ios::binary | std::ios::trunc);
    if (!newFile->is_open())
    {
        std::cerr << "Error: Failed to open file : " << fileName << std::endl;
        delete newFile;
        //[soukaina] here what should i do ???? quit the program !
        return;
    }
    request[fd].multp.outFiles.push_back(newFile);
    request[fd].multp.currentFileIndex = request[fd].multp.outFiles.size() - 1;
   
    std::cout << "Opened file #" << request[fd].multp.currentFileIndex 
              << ": " << fileName << std::endl;
}

void Server::writeData(Binary_String& chunk, int fd)
{
    std::string boundary = request[fd].multp.boundary;
    std::string boundaryEnd = boundary + "--";

    request[fd].multp.partialHeaderBuffer += chunk.to_string();

    while (true)
    {
        if (request[fd].multp.isInHeader)
        {
            size_t headerEnd = request[fd].multp.partialHeaderBuffer.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
                break;

            std::string headers = request[fd].multp.partialHeaderBuffer.substr(0, headerEnd);
            size_t namePos = headers.find("filename=\"");
            if (namePos != std::string::npos)
                createFileName(headers, fd);

            request[fd].multp.partialHeaderBuffer.erase(0, headerEnd + 4);
            request[fd].multp.isInHeader = false;
        }
        else
        {
            size_t boundaryPos = request[fd].multp.partialHeaderBuffer.find(boundary);
            if (boundaryPos == std::string::npos)
            {
                if (!request[fd].multp.outFiles.empty())
                {
                    std::ofstream* file = request[fd].multp.outFiles.back();
                    file->write(request[fd].multp.partialHeaderBuffer.data(),
                                request[fd].multp.partialHeaderBuffer.size());
                }
                request[fd].multp.partialHeaderBuffer.clear();
                break;
            }
            else
            {
                bool isFinalBoundary = (request[fd].multp.partialHeaderBuffer.substr(boundaryPos, boundaryEnd.size()) == boundaryEnd);
                if (!request[fd].multp.outFiles.empty() && boundaryPos > 0)
                {
                    std::ofstream* file = request[fd].multp.outFiles.back();
                    file->write(request[fd].multp.partialHeaderBuffer.data(), boundaryPos - 2);
                }

                request[fd].multp.partialHeaderBuffer.erase(0, boundaryPos + boundary.size());

                if (isFinalBoundary)
                {
                    request[fd].multp.partialHeaderBuffer.clear();
                    request[fd].multp.outFiles.back()->close();
                    break;
                }
                else
                {
                    request[fd].multp.outFiles.back()->close();
                    request[fd].multp.isInHeader = true;
                }
            }
        }
    }
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

int Server::parsePostRequest(int fd, ConfigData& configIndex)
{
    std::string contentType;
    std::string filePath;
    FileTransferState &state = request[fd].state;
    size_t boundaryStart;

    contentType = request[fd].getContentType();
    if (contentType.find("boundary=") != std::string::npos) 
    {
         boundaryStart = contentType.find("boundary=") + 9;
        request[fd].multp.boundary = contentType.substr(boundaryStart, contentType.length());
        if (request[fd].multp.boundary.length() == 0)
            return getSpecificRespond(fd, configIndex.getErrorPages().find(400)->second, createBadRequest);
    }
    else
        return getSpecificRespond(fd, configIndex.getErrorPages().find(400)->second, createBadRequest);
    state.url = "root/Upload";
    /*if (checkEndPoint(state.filePath) == false)*/
    /*    return getSpecificRespond(fd, configIndex.getErrorPages().find(404)->second, createNotFoundResponse);*/
    return (0);
}

void Server::handlePostRequest(int fd)
{
    Binary_String chunkedData;

    request[fd].state.url = "root/Upload";
    chunkedData = readFileChunk_post(fd);
    if (chunkedData.empty())
      request.erase(fd);
    /*std::cout << chunkedData << std::endl ; */
    writeData(chunkedData, fd);
}
