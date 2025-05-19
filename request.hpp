//
#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "server.hpp"
#include "globalInclude.hpp"

struct FileTransferState
{
    std::string filePath;
    size_t offset;
    size_t fileSize;
    bool isComplete;
    bool isCompleteServingFile;
    size_t endOffset;
    std::string mime;
    int uriLength;
    int test;

    Binary_String header;
    time_t last_activity_time;
    std::string fileType;
    int fd;
    bool isValidHeader;
    int headerFlag;
    std::ofstream *file;
    bool isCompleteShortFile;

    int bytesReceived;
    Binary_String buffer;
    int flag;
    std::map<std::string, std::string> mapOnHeader;
    struct Multipart multp;
    std::set<std::string> knownPaths;

    FileTransferState() : offset(0), fileSize(0), isComplete(false), headerFlag(0), bytesReceived(0) {}
    ~FileTransferState() {}
};

class Request
{
public:
    FileTransferState state;
    std::string header;
    std::map<std::string, std::string> keys;
    int flag;

public:
    Request() {}
    Request(std::string h, std::map<std::string, std::string> k)
    {
        this->header = h;
        this->keys = k;
    }

public:
    std::string method;
    std::string host;
    std::string accept;
    std::string connection;
    std::string transferEncoding;
    std::string contentLength;
    std::string ContentType;

public:
    void setAccept(std::string accep) { this->accept = accep; }
    void setHost(std::string hosts) { this->host = hosts; }
    void setMethod(std::string meth) { this->method = meth; }
    void setContentType(std::string cont) { this->ContentType = cont; }
    void setFileTransfers(FileTransferState state) { this->state = state; }
    void setConnection(std::string c) { connection = c; }
    void setTransferEncoding(std::string te) { transferEncoding = te; }
    void setContentLength(std::string cl) { contentLength = cl; }

public:
    std::string getAccept() { return accept; }
    std::string getHost() { return host; }
    std::string getMethod() { return method; }
    std::string getContentType() { return ContentType; }
    FileTransferState getFileTransfers() { return state; }
    std::string getConnection() { return connection; }
    std::string getTransferEncoding() { return transferEncoding; }
    std::string getContentLength() { return contentLength; }

public:
    void Data()
    {
        std::cout << "---------------------------------------\n"
                  << std::endl;
        std::cout << "-------(Start : Meta data) --------" << std::endl;
        std::cout << "method: [" << method << "]" << std::endl;
        std::cout << "connection: [" << connection << "]" << std::endl;
        std::cout << "ContentType: [" << ContentType << "]" << std::endl;
        std::cout << "transferEncoding : [" << transferEncoding << "]" << std::endl;
        std::cout << "contentLength : [" << contentLength << "]" << std::endl;
        std::cout << "host : [" << host << "]" << std::endl;
        std::cout << "accept : [" << accept << "]" << std::endl;
        std::cout << "-------(End : Meta data) --------\n"
                  << std::endl;
        std::cout << "-------(Start : FileTransferState) --------" << std::endl;
        std::cout << "state.filePath: [" << state.filePath << "]" << std::endl;
        std::cout << "state.offset: [" << state.offset << "]" << std::endl;
        std::cout << "state.fileSize: [" << state.fileSize << "]" << std::endl;
        std::cout << "state.isComplete: [" << state.isComplete << "]" << std::endl;
        std::cout << "mime: [" << state.mime << "]" << std::endl;
        std::cout << "uriLength: [" << state.uriLength << "]" << std::endl;
        std::cout << "-------(End : FileTransferState) --------\n"
                  << std::endl;
        std::cout << "---------------------------------------\n"
                  << std::endl;
    }

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
    Request getRequest() const { return request; }
};

class HeaderValidator
{
public:
    virtual ~HeaderValidator() {}
    virtual bool validate(RequstBuilder &builder) = 0;
    virtual HeaderValidator *getNextValidator() = 0;
};

class FileTransferValidator : public HeaderValidator
{
public:
    FileTransferValidator() {}
    ~FileTransferValidator() {}

public:
    bool validate(RequstBuilder &builder)
    {
        std::string method = builder.getRequest().getMethod();
        if (method == "GET" || method == "POST" || method == "DELETE")
        {
            if (builder.getRequest().state.uriLength > MAXURI)
            {
                return false;
            }
            if (builder.getRequest().keys.find(method)->second.find("HTTP/1.1") == std::string::npos)
                return false;
        }
        return true;
    }
    std::map<int, int> map;
    
    HeaderValidator *getNextValidator()
    {
        return NULL;
    }
};

class TransferEncodingValidator : public HeaderValidator
{
public:
    TransferEncodingValidator() {}
    ~TransferEncodingValidator() {}

public:
    bool validate(RequstBuilder &builder)
    {
        if (builder.getRequest().getMethod() == "GET")
        {
            return builder.getRequest().getTransferEncoding() == "undefined";
        }
        else if (builder.getRequest().getMethod() == "POST")
        {
            return builder.getRequest().getTransferEncoding() == "undefined";
        }
        else if (builder.getRequest().getMethod() == "DELETE")
        {
            return builder.getRequest().getTransferEncoding() == "undefined";
        }
        return true;
    }

    HeaderValidator *getNextValidator()
    {
        return new FileTransferValidator();
    }
};

class ContentTypeValidator : public HeaderValidator
{
public:
    ContentTypeValidator() {}
    ~ContentTypeValidator() {}

public:
    bool validate(RequstBuilder &builder)
    {
        if (builder.getRequest().getMethod() == "GET")
        {
            return builder.getRequest().getContentType() == "undefined";
        }
        else if (builder.getRequest().getMethod() == "POST")
        {
            return builder.getRequest().getContentType().find("multipart/form-data") != std::string::npos;
        }
        else if (builder.getRequest().getMethod() == "DELETE")
        {
            return builder.getRequest().getContentType() == "undefined";
        }
        return true;
    }

    HeaderValidator *getNextValidator()
    {
        return new TransferEncodingValidator();
    }
};
class ContentLengthValidator : public HeaderValidator
{
public:
    ContentLengthValidator() {}
    ~ContentLengthValidator() {}

public:
    bool validate(RequstBuilder &builder)
    {
        if (builder.getRequest().getMethod() == "GET")
        {
            return builder.getRequest().getContentLength() == "undefined";
        }
        else if (builder.getRequest().getMethod() == "POST")
        {
            if (builder.getRequest().getContentLength().empty() || builder.getRequest().getContentLength() == "undefined")
            {
                return false;
            }
            bool check = true;
            std::string str = builder.getRequest().getContentLength();
            for (size_t i = 0; i < str.length(); i++)
            {
                if (!isdigit(str.at(i)))
                    check = false;
            }
            return check;
        }
        else if (builder.getRequest().getMethod() == "DELETE")
        {
            return builder.getRequest().getContentLength() == "undefined";
        }
        return false;
    }

    HeaderValidator *getNextValidator()
    {
        return new ContentTypeValidator();
    }
};

class ConnectionValidator : public HeaderValidator
{
public:
    ConnectionValidator() {}
    ~ConnectionValidator() {}

public:
    bool validate(RequstBuilder &builder)
    {
        bool check = builder.getRequest().getConnection() == "close" || builder.getRequest().getConnection() == "keep-alive" || builder.getRequest().getConnection() == "undefined" || !builder.getRequest().getConnection().empty();

        return check;
    }

    HeaderValidator *getNextValidator()
    {
        return new ContentLengthValidator();
    }
};

class Build
{

public:
    Build() {}
    ~Build() {}

public:
    void buildRequest(RequstBuilder &requstBuilder)
    {
        requstBuilder.buildConnection();
        requstBuilder.buildMethod();
        requstBuilder.buildFileTransfers();
        requstBuilder.buildTransferEncoding();
        requstBuilder.buildContentLength();
        requstBuilder.buildContentType();
        requstBuilder.buildAccept();
        requstBuilder.buildHost();
    };

public:
    std::pair<bool, int> buildRequest_valid(RequstBuilder &requstBuilder)
    {
        std::pair<bool, int> return_pair;
        return_pair.first = true;
        return_pair.second = 0;
        ContentLengthValidator contentLengthValidator;
        TransferEncodingValidator transferEncodingValidator;
        ConnectionValidator connectionValidator;
        FileTransferValidator fileTransferValidator;
        ContentTypeValidator contentTypeValidator;

        std::vector<HeaderValidator *> validators;
        validators.push_back(&fileTransferValidator);
        validators.push_back(&connectionValidator);
        validators.push_back(&transferEncodingValidator);
        validators.push_back(&contentLengthValidator);
        validators.push_back(&contentTypeValidator);

        for (std::vector<HeaderValidator *>::iterator it = validators.begin(); it != validators.end(); ++it)
        {
            if (!(*it)->validate(requstBuilder))
            {
                
                std::cout << "Request is invalid." << std::endl;
                return_pair.first = false;
                return_pair.second = -1;
                return return_pair;
            }
        }
        return return_pair;
    }
};

#endif