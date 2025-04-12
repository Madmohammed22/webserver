#include "../server.hpp"

int Server::processMethodNotAllowed(int fd, Server *server, std::string request){
    std::cout << "-------( REQUEST PARSED )-------\n\n";
    std::cout << request << std::endl;
    std::cout << "-------( END OF REQUEST )-------\n\n";
    
    std::pair<std::string, std::string> pair_request = server->ft_parseRequest(request);
    FileTransferState state;
    state.typeOfConnection = server->key_value_pair_header(pair_request.first,"Connection:");;
    server->fileTransfers[fd] = state;
    return getSpecificRespond(fd, server, "405.html", server->methodNotAllowedResponse);
}
