#include "../server.hpp"

void POST::includeBuild(std::string target, std::string &metaData, int pick)
{
    std::map<std::string, std::string>::iterator it = request.keys.find(target);
    if (it != request.keys.end())
    {
        if (Server::containsOnlyWhitespace(it->second) == false){
            if (pick == 1)
                metaData = it->first;
            else
                metaData = it->second;
        }
        else
            metaData = "empty";
        return;
    }
    metaData = "undefined";
}

void POST::buildFileTransfers()
{
    FileTransferState &state = request.state;
    state.url = Server::parseSpecificRequest(request.header);
    state.offset = 0;
    state.fileSize = Server::getFileSize(PATHC + state.url);
    state.isComplete = false;
    state.mime = Server::getContentType(state.url);
    state.uriLength = state.url.length();
    state.test = 0;
    state.headerFlag = false;
    state.last_activity_time = time(NULL);
}

