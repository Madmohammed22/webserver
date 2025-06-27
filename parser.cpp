#include "server.hpp"
#include "Cgi.hpp"
#include "request.hpp"
#include "helper/utils.hpp"

std::string concatenate(std::vector<std::string> &vec)
{
  std::string result;
  std::vector<std::string>::iterator it;

  for (it = vec.begin(); it != vec.end(); it++)
  {
    result += '/' + *it;
  }
  return result;
}

std::string resolveUrl(std::string &url)
{
  std::vector<std::string> splitBySlash;
  std::vector<std::string> currentUrl;

  splitBySlash = split(url, '/');
  std::vector<std::string>::iterator it;

  for (it = splitBySlash.begin(); it != splitBySlash.end(); it++)
  {
    if (*it == ".." && !currentUrl.empty())
      currentUrl.pop_back();
    else if (*it == ".." && currentUrl.empty())
      return ("");
    else if (*it != ".")
      currentUrl.push_back(*it);
  }
  if (currentUrl.empty())
    return ("/");
  return (concatenate(currentUrl));
}

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
    backup = state.bytesReceived;
    fileName_backup = state.fileName;
    backup = backup - request[fd].bodyStart;
    // [soukaina] here after the request is checked i will store the serverConfig in the request
    serverConfig = getConfigForRequest(multiServers[clientToServer[fd]], request[fd].getHost());
    request[fd].serverConfig = serverConfig;
    state.headerFlag = false;
    tmpMap = key_value_pair(ft_parseRequest_T(fd, this, state.header, serverConfig).first);

    if (!(state.header.find("POST") != std::string::npos))
      request[fd].state.file->close();

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
        std::cout << "I was here\n";
        return false;
      }
      request[fd].state.last_activity_time = time(NULL);

    }
    else if (state.header.find("DELETE") != std::string::npos)
    {
      DELETE delete_;

      if (header_parser(delete_, request[fd], state.header, tmpMap) == false)
        return false;
        
      request[fd].state.isComplete = true;
    }

    if (request[fd].getContentLength() == "0"){
      request[fd].state.last_activity_time = time(NULL);
      request[fd].state.isComplete = true;
      return true;
    }
    if (request[fd].bodyStart)
    {
      Binary_String body = holder.substr(request[fd].bodyStart, backup);

      state.file->write(body.c_str(), backup);
      if (static_cast<int>(atoi(request[fd].contentLength.c_str())) <= (int)backup)
      {
        state.isComplete = true;
        state.file->close();
      }
    }
    // request[fd].state.url = "/path/../path/pathkhk/..";
    // std::cout << "this is my resolved URL " << resolveUrl(request[fd].state.url)<< std::endl;
    // exit(0);
    // [soukaina] this must be changed to right function that extract the correct location
    request[fd].location = serverConfig.getLocations().front();
    // request[fd].location = getExactLocationBasedOnUrl(state.url, request[fd].serverConfig);

    if (request[fd].getMethod() != "POST")
    {
      request[fd].state.file->close();
    }
    // std::cout << request[fd].location.root << std::endl;
    if (request[fd].state.url.find("/cgi-bin") != std::string::npos)
    {

      request[fd].cgi.parseCgi(request[fd]);
      if (request[fd].code != 200)
      {
        std::cout << "i have been here\n";
        return (false);
      }
      request[fd].cgi.setEnv(request[fd]);
    }
    state.buffer.clear();
  }
  state.fileName = fileName_backup;
  state.bytesReceived = backup;
  return true;
}
