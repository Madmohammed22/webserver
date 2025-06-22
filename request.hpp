//
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "globalInclude.hpp"
#include "Cgi.hpp"

#define MAX_URI_SIZE 2048
#define MAX_HEADER_KEY_SIZE 8192
#define MAX_HEADER_VALUE_SIZE 128

enum parsingState
{
    START,
    Method,
    FirstSpace,
    SlashURI,
    URIBody,
    StringH,
    StringHT,
    StringHTT,
    StringHTTP,
    StringHTTPSlash,
    StringHTTPSlashN,
    StringHTTPSlashNDot,
    StringHTTPSlashNDotN,
    FirstSlashR,
    FirstSlashN,
    HeaderKey,
    HeaderValue,
    ERROR,
    END
};
 
struct FileTransferState
{
    std::string url;
    size_t offset;
    size_t fileSize;
    bool isComplete;
    bool PostHeaderIsValid;
    bool isCompleteServingFile;
    size_t endOffset;
    std::string mime;
    int uriLength;
    int test;

    std::string fileName;
    std::string header;
    time_t last_activity_time;
    std::string fileType;
    int fd;
    bool isValidHeader;
    bool headerFlag;
    std::ofstream *file;
    bool isCompleteShortFile;
    std::set<std::string> logFile;
    int bytesReceived;
    Binary_String buffer;
    int flag;
    std::map<std::string, std::string> mapOnHeader;
    struct Multipart multp;
    std::set<std::string> knownPaths;

    FileTransferState() : offset(0), fileSize(0), isComplete(false), PostHeaderIsValid(false), headerFlag(true), bytesReceived(0) {}
    ~FileTransferState() {}
};

class Request
{
public:
    ConfigData serverConfig;
    FileTransferState state;
    std::string header;
    Cgi cgi;
    struct Multipart multp;
    Location location;
    std::map<std::string, std::string> keys;
    int flag;
    int code;
    int bodyStart;

public:
    Request(): cgi(), multp() , code(200), bodyStart(0), _parsingState(START), _urlLength(0), _keyLength(0), _valueLength(0){}
    Request(std::string h, std::map<std::string, std::string> k)
    : multp(){
        this->code = 200;
        this->_urlLength = 0;
        this->_keyLength = 0;
        this->_valueLength = 0;
        this->_parsingState = START;
        this->header = h;
        this->keys = k;
        this->bodyStart = 0;
    }

public:
    std::string method;
    std::string host;
    std::string accept;
    std::string connection;
    std::string transferEncoding;
    std::string contentLength;
    std::string ContentType;
    std::string Cookie;

// parsing request Syntax
private:
    parsingState _parsingState;
    int _urlLength;
    int _keyLength;
    int _valueLength;

public:
    void setAccept(std::string accep) { this->accept = accep; }
    void setHost(std::string hosts) { this->host = hosts; }
    void setMethod(std::string meth) { this->method = meth; }
    void setContentType(std::string cont) { this->ContentType = cont; }
    void setFileTransfers(FileTransferState state) { this->state = state; }
    void setConnection(std::string c) { connection = c; }
    void setTransferEncoding(std::string te) { transferEncoding = te; }
    void setContentLength(std::string cl) { contentLength = cl; }
    void setCookie(std::string cookie) { Cookie = cookie; }

public:
    std::string getAccept() { return accept; }
    parsingState getParsingState() { return _parsingState; }
    std::string getHost() { return host; }
    std::string getMethod() { return method; }
    std::string getContentType() { return ContentType; }
    FileTransferState getFileTransfers() { return state; }
    std::string getConnection() { return connection; }
    std::string getTransferEncoding() { return transferEncoding; }
    std::string getContentLength() { return contentLength; }
    std::string getCookie() { return Cookie; }

    //parsing request
    int checkHeaderSyntax(Binary_String buffer);
public:
    ~Request() {}
};

class RequstBuilder
{
public:
    virtual void buildHost() = 0;
    virtual void buildAccept() = 0;
    virtual void buildMethod() = 0;
    virtual void buildContentType() = 0;
    virtual void buildFileTransfers() = 0;
    virtual void buildConnection() = 0;
    virtual void buildTransferEncoding() = 0;
    virtual void buildContentLength() = 0;
    virtual void buildCookie() = 0;

public:
    virtual Request getRequest() const = 0;
};

class GET : public RequstBuilder
{

public:
    Request request;

public:
    GET() {};
    GET(Request req) { this->request = req; }

public:
    void includeBuild(std::string target, std::string &metaData, int pick);
    void buildHost() { includeBuild("Host:", request.host, 2); };
    void buildAccept() { includeBuild("Accept:", request.accept, 2); };
    void buildMethod() { includeBuild("GET", request.method, 1); };
    void buildContentType() { includeBuild("Content-Type:", request.ContentType, 2); };
    void buildFileTransfers();
    void buildConnection() { includeBuild("Connection:", request.connection, 2); };
    void buildTransferEncoding() { includeBuild("Transfer-Encoding:", request.transferEncoding, 2); };
    void buildContentLength() { includeBuild("Content-Length:", request.contentLength, 2); };
    void buildCookie() { includeBuild("Cookie:", request.Cookie, 2); };

    Request getRequest() const { return request; }
};

class POST : public RequstBuilder
{
public:
    Request request;

public:
    POST() {};

    POST(Request req) { this->request = req; }

public:
    void includeBuild(std::string target, std::string &metaData, int pick);
    void buildHost() { includeBuild("Host:", request.host, 2); };
    void buildAccept() { includeBuild("Accept:", request.accept, 2); };
    void buildMethod() { includeBuild("POST", request.method, 1); };
    void buildContentType() { includeBuild("Content-Type:", request.ContentType, 2); };
    void buildFileTransfers();
    void buildConnection() { includeBuild("Connection:", request.connection, 2); };
    void buildTransferEncoding() { includeBuild("Transfer-Encoding:", request.transferEncoding, 2); };
    void buildContentLength() { includeBuild("Content-Length:", request.contentLength, 2); };
    void buildCookie() { includeBuild("Cookie:", request.Cookie, 2); };
    Request getRequest() const { return request; }
};

//
class DELETE : public RequstBuilder
{
public:
    Request request;

public:
    DELETE() {};
    DELETE(Request req) { this->request = req; }

public:
    void includeBuild(std::string target, std::string &metaData, int pick);
    void buildHost() { includeBuild("Host:", request.host, 2); };
    void buildAccept() { includeBuild("Accept:", request.accept, 2); };
    void buildMethod() { includeBuild("DELETE", request.method, 1); };
    void buildContentType() { includeBuild("Content-Type:", request.ContentType, 2); };
    void buildFileTransfers();
    void buildConnection() { includeBuild("Connection:", request.connection, 2); };
    void buildTransferEncoding() { includeBuild("Transfer-Encoding:", request.transferEncoding, 2); };
    void buildContentLength() { includeBuild("Content-Length:", request.contentLength, 2); };
    void buildCookie() { includeBuild("Cookie:", request.Cookie, 2); };
    Request getRequest() const { return request; }
};

class HeaderValidator
{
public:
    virtual ~HeaderValidator() {}
    virtual bool validate(RequstBuilder &builder) = 0;
    virtual HeaderValidator *getNextValidator() = 0;
};

class CookieValidator : public HeaderValidator
{
public:
    CookieValidator() {}
    ~CookieValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return NULL; }
};

class FileTransferValidator : public HeaderValidator
{
public:
    FileTransferValidator() {}
    ~FileTransferValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return new CookieValidator(); }
};

class TransferEncodingValidator : public HeaderValidator
{
public:
    TransferEncodingValidator() {}
    ~TransferEncodingValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return new FileTransferValidator(); }
};

class ContentTypeValidator : public HeaderValidator
{
public:
    ContentTypeValidator() {}
    ~ContentTypeValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return new TransferEncodingValidator(); }
};
class ContentLengthValidator : public HeaderValidator
{
public:
    ContentLengthValidator() {}
    ~ContentLengthValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return new ContentTypeValidator(); }
};

class ConnectionValidator : public HeaderValidator
{
public:
    ConnectionValidator() {}
    ~ConnectionValidator() {}
    bool validate(RequstBuilder &builder);
    HeaderValidator *getNextValidator() { return new ContentLengthValidator(); }
};

class Build
{

public:
    Build() {}
    ~Build() {}

public:
    void requestBuilder(RequstBuilder &requstBuilder);
    std::pair<bool, int> chainOfResponsibility(RequstBuilder &requstBuilder);
};

#endif
