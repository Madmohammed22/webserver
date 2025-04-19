#include "../server.hpp"

int Server::processMethodNotAllowed(int fd, Server *server, std::string request){
    std::cout << "-------( REQUEST PARSED )-------\n\n";
    std::cout << request << std::endl;
    std::cout << "-------( END OF REQUEST )-------\n\n\n";
    
    std::pair<std::string, std::string> pair_request = server->ft_parseRequest(fd, server, request);
    FileTransferState state;
    state.typeOfConnection = server->fileTransfers[fd].mapOnHeader.find("Connection:")->second;
    server->fileTransfers[fd] = state;
    return getSpecificRespond(fd, server, "405.html", server->methodNotAllowedResponse);
}

