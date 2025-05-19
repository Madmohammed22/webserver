#include "server.hpp"
#include "request.hpp"

template <typename T>

bool header_parser(T method, Request &request, std::string header, std::map<std::string, std::string> tmpMap)
{
    Build build;
    std::ofstream *file = request.state.file;
    Request methaData(header, tmpMap);
    method = T(methaData);
    build.buildRequest(method);
    if (!build.buildRequest_valid(method).first)
    {
        return false;
    }
    methaData = method.getRequest();
    request.method = methaData.getMethod();
    request.connection = methaData.getConnection();
    request.transferEncoding = methaData.getTransferEncoding();
    request.contentLength = methaData.getContentLength();
    request.ContentType = methaData.getContentType();
    request.accept = methaData.getAccept();
    request.host = methaData.getHost();
    request.header = header;
    request.state = methaData.getFileTransfers();
    request.state.file = file;
    return true;
}

bool Server::validateHeader(int fd, FileTransferState &state, ConfigData serverConfig)
{
    std::map<std::string, std::string> tmpMap;
    size_t header_end = state.buffer.find("\r\n\r\n");
    if (header_end != std::string::npos)
    {
        state.header = state.buffer.substr(0, header_end + 4);
        state.headerFlag = true;
        size_t body_start = header_end + 4;
        if (body_start < state.buffer.length())
        {

            Binary_String body_part = state.buffer.substr(body_start, state.buffer.length());
            state.bytesReceived += body_part.length();
            state.file->write(body_part.c_str(), body_part.length());
            state.bytesReceived += body_part.length();
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
    return true;
}