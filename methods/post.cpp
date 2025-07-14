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


bool    checkRedirect(Location &location, Request &req)
{
    if (!location.redirect.empty())
    {
        req.code = 301;
        if (location.redirect[0] != '/')
            location.redirect.insert(location.redirect.begin(), '/');
        return (false);
    }
    return (true);
}

int Server::parsePostRequest(int fd, ConfigData &configIndex, Request &req)
{
    std::string contentType;
    size_t boundaryStart;
    Location &location = req.location;
    
    if (checkAvailability(fd, location) == false)
    {
        req.code = 405;
        return (1);
    }

    if (checkRedirect(location, req) == false)
         return (1);

    if (req.state.bytesReceived > (int) configIndex.getClientMaxBodySize())
    {
         req.code = 413;
         return (1);
    }
   
    if (location.upload.empty())
    {
      req.code = 403;
      return (1);
    }
    
    if (getFileType(location.upload) != 1)
    {
      req.code = 403;
      return (1);
    }

    if (location.upload[location.upload.length() - 1] != '/')
      location.upload.insert(location.upload.end(), '/');
    

    contentType = request[fd].getContentType();
    if (contentType.find("boundary=") != std::string::npos)
    {
        boundaryStart = contentType.find("boundary=") + 9;
        request[fd].multp.boundary = contentType.substr(boundaryStart, contentType.length());
        if (request[fd].multp.boundary.length() == 0)
        {
          req.code = 400;
          return (1);
        }
    }
    else
    {
      req.code = 400;
      return (1);
    }
    
    req.state.last_activity_time = time(NULL);
    req.state.PostHeaderIsValid = true;
    return (0);
}
