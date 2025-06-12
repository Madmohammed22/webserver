#include "server.hpp"
#include "request.hpp"

template <typename T>

bool header_parser(T method, Request &request, std::string header, std::map<std::string, std::string> tmpMap)
{
    Build build;
    std::ofstream *file = request.state.file;
    Request metaData(header, tmpMap);
    method = T(metaData);
    build.requestBuilder(method);
    if (!build.chainOfResponsibility(method).first)
    {
        return false;
    }
    metaData = method.getRequest();
    request.method = metaData.getMethod();
    request.connection = metaData.getConnection();
    request.transferEncoding = metaData.getTransferEncoding();
    request.contentLength = metaData.getContentLength();
    request.ContentType = metaData.getContentType();
    request.accept = metaData.getAccept();
    request.host = metaData.getHost();
    request.header = header;
    request.state = metaData.getFileTransfers();
    request.state.file = file;
    request.Cookie = metaData.getCookie();
    return true;
}

bool Server::validateHeader(int fd, FileTransferState &state, ConfigData serverConfig)
{
    std::map<std::string, std::string> tmpMap;
    size_t header_end = state.buffer.find("\r\n\r\n");
    static size_t backup;

    if (header_end != std::string::npos)
    {
        state.header = state.buffer.substr(0, header_end + 4);
        state.headerFlag = true;
        size_t body_start = header_end + 4;
        if (body_start < state.buffer.length())
        {

            Binary_String body_part = state.buffer.substr(body_start, state.buffer.length());
            backup += body_part.length();
            state.file->write(body_part.c_str(), body_part.length());
        }
        try
        {
            
            tmpMap = key_value_pair(ft_parseRequest_T(fd, this, state.header.to_string(), serverConfig).first);
        }
        catch (const std::exception &e)
        {
            return false;
        }

        if (state.header.find("GET") != std::string::npos)
        {
            GET get;
            if (header_parser(get, request[fd], state.header.to_string(), tmpMap) == false)
                return false;
            request[fd].state.isComplete = true;
        }
        else if (state.header.find("POST") != std::string::npos)
        {
            POST post;

            if (header_parser(post, request[fd], state.header.to_string(), tmpMap) == false)
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

            if (header_parser(delete_, request[fd], state.header.to_string(), tmpMap) == false)
                return false;
            request[fd].state.isComplete = true;
        }
        else if (state.header.find("PUT") != std::string::npos || state.header.find("PATCH") != std::string::npos || state.header.find("HEAD") != std::string::npos || state.header.find("OPTIONS") != std::string::npos)
        {
            return getSpecificRespond(fd, serverConfig.getErrorPages().find(405)->second, methodNotAllowedResponse), true;
        }
        state.buffer.clear();
    }
    state.bytesReceived = backup;
    return true;
}
