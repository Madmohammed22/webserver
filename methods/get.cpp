#include "../server.hpp"

int Server::serve_file_request(int fd, Server *server, std::string request)
{
    // Check if we already have a file transfer in progress
    if (server->fileTransfers.find(fd) != server->fileTransfers.end())
        return server->continueFileTransfer(fd, server);
    
    std::string filePath = server->parseRequest(request,server);
    if (server->canBeOpen(filePath) && server->getFileType(filePath) == 2)
        return server->handleFileRequest(fd, server, filePath);
    else
    {
        return getSpecificRespond(fd, server, "404.html", server->createNotFoundResponse);
    }
    return 0;
}
