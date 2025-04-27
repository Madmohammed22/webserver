#include "../server.hpp"

int Server::processMethodNotAllowed(int fd, Server *server, std::string request){
    // server->key_value_pair_header(fd, server, ft_parseRequest_T(fd, server, request).first);
    std::cout << "-------( REQUEST PARSED )-------\n\n";
    std::cout << request << std::endl;
    std::cout << "-------( END OF REQUEST )-------\n\n\n";
    return getSpecificRespond(fd, server, "405.html", server->methodNotAllowedResponse);
}

