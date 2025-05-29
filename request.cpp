
#include "server.hpp"
#include "request.hpp"

void Build::requestBuilder(RequstBuilder &requstBuilder)
{
    requstBuilder.buildConnection();
    requstBuilder.buildMethod();
    requstBuilder.buildFileTransfers();
    requstBuilder.buildTransferEncoding();
    requstBuilder.buildContentLength();
    requstBuilder.buildContentType();
    requstBuilder.buildAccept();
    requstBuilder.buildHost();
}

std::pair<bool, int> Build::chainOfResponsibility(RequstBuilder &requstBuilder)
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

bool FileTransferValidator::validate(RequstBuilder &builder)
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

bool TransferEncodingValidator::validate(RequstBuilder &builder)
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

bool ContentTypeValidator::validate(RequstBuilder &builder)
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

bool ContentLengthValidator::validate(RequstBuilder &builder)
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

bool ConnectionValidator::validate(RequstBuilder &builder)
{
    bool check = builder.getRequest().getConnection() == "close" || builder.getRequest().getConnection() == "keep-alive" || builder.getRequest().getConnection() == "undefined" || !builder.getRequest().getConnection().empty();

    return check;
}
