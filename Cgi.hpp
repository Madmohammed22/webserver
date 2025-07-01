#ifndef CGI_HPP
#define CGI_HPP


class Server;
#include "globalInclude.hpp"
#include "ConfigData.hpp"

enum cgiState
{
  CGI_NOT_STARTED,
  CGI_RUNNING,
  CGI_COMPLETE,
};

class Request;

class Cgi
{
  
private:
  std::map < std::string, std::string > _varEnv;
  int _status;
  char **_env;
  char **_argv;
  std::string _query;
  std::string _path;
  bool _isCgi;
  pid_t _pid;
  
public:

  Cgi();
  ~Cgi();

  std::string CgiBodyResponse;
  std::string fileNameOut;
  std::string fileNameIn;
  enum cgiState cgiState;
  int fdIn;
  int bytesSend;
  int readPosition;

  std::string getFileExtension(std::string& path, std::string &url);  
  std::string getPathInfo(std::string &path, std::string &ext);
  void parseCgi(Request &req);
  bool getIsCgi();
  void runCgi(Server &serv, int fd, Request &req, ConfigData &serverConfig);
  std::string getQuery();
  void setQuery(std::string query);
  void setEnv(Request &req);
  int getStatus();
  pid_t getPid();
  void setStatus(int status);


};

#endif
