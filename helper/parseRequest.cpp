#include "../server.hpp"

void Server::printfContentHeader(Server *server, int fd){
    std::map<std::string, std::string>::iterator it = server->fileTransfers[fd].mapOnHeader.begin();
    while(it != server->fileTransfers[fd].mapOnHeader.end()){
        std::cout << it->first << " " << it->second << std::endl;
        it++;
    }
}
std::string Server::parseSpecificRequest(int fd, std::string request, Server *server)
{
    std::cout << "-------( REQUEST PARSED )-------\n\n";
    server->printfContentHeader(server, fd);
    std::cout << "\n-------( END OF REQUEST )-------\n\n\n";

    std::string filePath = "/index.html";

    // Handle GET requests
    size_t startPos = request.find("GET /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
        return filePath;
    }

    // Handle DELETE requests
    startPos = request.find("DELETE /");
    if (startPos != std::string::npos)
    {
        startPos += 8;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath != "/")
            {
                filePath = requestedPath;
            }
        }
        return filePath;
    }

    // Handle POST requests
    startPos = request.find("POST /");
    if (startPos != std::string::npos)
    {
        startPos += 5;
        size_t endPos = request.find(" HTTP/", startPos);
        if (endPos != std::string::npos)
        {
            std::string requestedPath = request.substr(startPos, endPos - startPos);
            if (!requestedPath.empty() && requestedPath == "/")
            {
                filePath = PATHC;
            }
            else
                return requestedPath;
        }
        return filePath;
    }

    return filePath;
}
