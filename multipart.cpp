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
    std::string fileName = request[fd].location.upload + line.substr(start, end - start);
    // if (access(fileName.c_str(), R_OK | W_OK) == -1){
    //     getResponse(fd, 403);
    //     return;
    // }
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
}

void Server::writeData(Binary_String &chunk, int fd)
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
            if (!(boundaryPos != std::string::npos))
            {
                if (!request[fd].multp.outFiles.empty())
                {
                    std::ofstream *file = request[fd].multp.outFiles.back();
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
                    std::ofstream *file = request[fd].multp.outFiles.back();
                    file->write(request[fd].multp.partialHeaderBuffer.data(), boundaryPos - 4);
                }

                request[fd].multp.partialHeaderBuffer.erase(0, boundaryPos + boundary.size());

                if (isFinalBoundary)
                {
                    request[fd].multp.partialHeaderBuffer.clear();
                    request[fd].multp.outFiles.back()->close();
                    delete request[fd].multp.outFiles.back();
                    request[fd].multp.readPosition = -2;
                    break;
                }
                else
                {
                    if (request[fd].multp.outFiles.back()
                      && request[fd].multp.outFiles.back()->is_open())
                      request[fd].multp.outFiles.back()->close();
                    if (request[fd].multp.outFiles.back())
                      delete request[fd].multp.outFiles.back();
                    /*printf(" this is the back %p \n", request[fd].multp.outFiles.back());*/
                    /*if (!request[fd].multp.outFiles.empty())*/
                    /*  delete request[fd].multp.outFiles.back();*/
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


void Server::handlePostRequest(int fd)
{
    Binary_String chunkedData;
  
    if (request[fd].multp.readPosition == -2)
    {
        if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)
        {
            close(fd);
            request.erase(fd);
        }
        return ;
    }
    chunkedData = readFileChunk_post(fd);
    if (chunkedData.empty())
        request.erase(fd);

    writeData(chunkedData, fd);
    if (request[fd].multp.readPosition == -2)
    {
        std::string httpRespons;
        httpRespons = httpResponse(request[fd].ContentType, request[fd].state.fileSize);

        int faild = send(fd, httpRespons.c_str(), httpRespons.length(), MSG_NOSIGNAL);
        if (faild == -1)
        {
            close(fd), request.erase(fd);
            return ;
        }
        if (request[fd].getConnection() == "close" || request[fd].getConnection().empty())
            close(fd), request.erase(fd);

        /*if (timedFunction(TIMEOUTREDIRACTION, request[fd].state.last_activity_time) == false)*/
        /*{*/
        /*    close(fd);*/
        /*    request.erase(fd);*/
        /*}*/
    }
}

