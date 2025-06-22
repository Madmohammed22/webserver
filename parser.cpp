#include "server.hpp"
#include "Cgi.hpp"
#include "request.hpp"

template <typename T>

bool header_parser(T method, Request &request, std::string header, std::map<std::string, std::string> tmpMap)
{
    Build build;
    std::ofstream *file = request.state.file;
    Request methaData(header, tmpMap);
    method = T(methaData);
    build.requestBuilder(method);
    if (!build.chainOfResponsibility(method).first)
    {
        return false;
    }

    //[soukaina] can you just assign methaData to request ???

    methaData = method.getRequest();
    request.method = methaData.getMethod();
    request.connection = methaData.getConnection();
    request.transferEncoding = methaData.getTransferEncoding();
    request.contentLength = methaData.getContentLength();
    request.ContentType = methaData.getContentType();
    request.accept = methaData.getAccept();
    request.host = methaData.getHost();
    request.header = header;
    request.Cookie = methaData.getCookie();
    request.state = methaData.getFileTransfers();
    request.state.file = file;
    return true;
}

bool Server::validateHeader(int fd, FileTransferState &state, Binary_String holder)
{
    std::map<std::string, std::string> tmpMap;
    static size_t backup;
    // [soukaina] this is really a bad thing we should figure out another solution
    std::string fileName_backup;
    ConfigData serverConfig;
    
    request[fd].checkHeaderSyntax(holder);

    if (request[fd].getParsingState() == ERROR)
      return (false);
    if (request[fd].getParsingState() == END)
    {
        // [soukaina] here after the request is checked i will store the serverConfig in the request
        serverConfig = getConfigForRequest(multiServers[clientToServer[fd]], request[fd].getHost());
        request[fd].serverConfig = serverConfig;
        state.headerFlag = false;
        if (request[fd].bodyStart)
        {
            Binary_String body = holder.substr(request[fd].bodyStart, holder.length());
            backup += body.length();
            state.file->write(body.c_str(), body.length());
        }
        fileName_backup = state.fileName;
        tmpMap = key_value_pair(ft_parseRequest_T(fd, this, state.header, serverConfig).first);
        if (state.header.find("GET") != std::string::npos)
        {
            GET get;
            if (header_parser(get, request[fd], state.header, tmpMap) == false)
                return false;
            request[fd].state.isComplete = true;
        }
        else if (state.header.find("POST") != std::string::npos)
        {
            POST post;

            if (header_parser(post, request[fd], state.header, tmpMap) == false)
            {
                return false;
            } 
            std::cout << "-------( REQUEST PARSED )-------\n\n";
            std::cout << request[fd].header << std::endl;
            std::cout << "-------( END OF REQUEST )-------\n\n\n";
        }
        else if (state.header.find("DELETE") != std::string::npos)
        {
            DELETE delete_;

            if (header_parser(delete_, request[fd], state.header, tmpMap) == false)
                return false;
            request[fd].state.isComplete = true;
        }
        // [soukaina] this must be changed to right function that extract the correct location
        request[fd].location = serverConfig.getLocations().front();
        // std::cout << request[fd].location.root << std::endl;
        if (request[fd].state.url.find("/cgi-bin") != std::string::npos)
        {
          request[fd].cgi.parseCgi(request[fd]);
          if (request[fd].code != 200)
              return (false);
          request[fd].cgi.setEnv(request[fd]);
        }
        state.buffer.clear();
    }
    state.fileName = fileName_backup;
    state.bytesReceived = backup;
    return true;
}
