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


// void check_timeout(Server *server)
// {
//     (void)server;
//     // std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin();
//     // (void)it;
//     // time_t current_time = time(NULL);
//     // while (it != server->fileTransfers.end())
//     // {
//     //     if (current_time - it->second.last_activity_time > TIMEOUT)
//     //     {
//     //         // std::cerr << "Client " << it->first << " timed out." << std::endl;
//     //         // close(it->first);
//     //         // std::map<int, FileTransferState>::iterator tmp = it;
//     //         // returnTimeoutRequest(tmp->second.saveFd, server);
//     //         // ++it;
//     //         // close(it->first);
//     //         // server->fileTransfers.erase(tmp);
//     //     }
//     //     else
//     //         ++it;
//     // }
// }


// if (nfds == 0){
//     std::cout << "TIMEOUT VIA EPOLL_WAIT" << std::endl;
// }