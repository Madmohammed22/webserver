#include "../server.hpp"


Binary_String::Binary_String(const char* str, size_t n)
{
    buffer.resize(n);
    for (size_t i = 0; i < n; i++)
        buffer[i] = str[i];
}

Binary_String::Binary_String()
{
}

Binary_String::Binary_String(size_t n)
{
    buffer.resize(n);
}

void Binary_String::clear()
{
    buffer.clear();
}

bool Binary_String::empty() const
{
    return buffer.empty();
}

uint8_t Binary_String::operator[](size_t i) const
{
    return buffer[i];
}

uint8_t& Binary_String::operator[](size_t i)
{
    return buffer[i];
}

uint8_t* Binary_String::data()
{
    return buffer.data();
}

std::vector <uint8_t >::iterator Binary_String::begin()
{
    return buffer.begin();
}

std::vector <uint8_t >::iterator Binary_String::end()
{
    return buffer.end();
}

size_t Binary_String::length() const
{
    return buffer.size();
}

void Binary_String::push_back(uint8_t c)
{
    buffer.push_back(c);
}

Binary_String::Binary_String(const Binary_String& other) : buffer(other.buffer)
{
}

Binary_String::~Binary_String()
{
}

size_t Binary_String::find(const char* s, size_t pos) const
{
    size_t n = strlen(s);
    if (n == 0)
        return 0;
    for (size_t i = pos; i < buffer.size(); i++)
    {
        if (buffer[i] == s[0])
        {
            bool found = true;
            for (size_t j = 1; j < n; j++)
            {
                if (i + j >= buffer.size() || buffer[i + j] != s[j])
                {
                    found = false;
                    break;
                }
            }
            if (found)
                return i;
        }
    }
    return std::string::npos;
}

Binary_String Binary_String::substr(size_t pos, size_t n) const
{
    Binary_String result;
    for (size_t i = pos; i < pos + n && i < buffer.size(); i++)
        result.push_back(buffer[i]);
    return result;
}

Binary_String Binary_String::operator+(const Binary_String& other) const
{
    Binary_String result;
    for (size_t i = 0; i < buffer.size(); i++)
        result.push_back(buffer[i]);
    for (size_t i = 0; i < other.buffer.size(); i++)
        result.push_back(other.buffer[i]);
    return result;
}

Binary_String Binary_String::operator+=(const Binary_String& other)
{
    for (size_t i = 0; i < other.buffer.size(); i++)
        push_back(other.buffer[i]);
    return *this;
}

bool Binary_String::operator==(const Binary_String& other) const
{
    if (buffer.size() != other.buffer.size())
        return false;
    for (size_t i = 0; i < buffer.size(); i++)
    {
        if (buffer[i] != other.buffer[i])
            return false;
    }
    return true;
}

bool Binary_String::operator!=(const Binary_String& other) const
{
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const Binary_String& buffer)
{
    for (size_t i = 0; i < buffer.length(); i++)
    {
        if (isprint(buffer[i]))
            os << buffer[i];
        else
            os << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    return os;
}

std::string Binary_String::to_string() const
{
    std::string result;
    for (size_t i = 0; i < buffer.size(); i++)
        result += buffer[i];
    return result;
}

const char* Binary_String::c_str() const
{
    return reinterpret_cast<const char*>(buffer.data());
}

size_t Binary_String::find(const std::string& s, size_t pos) const
{
    return find(s.c_str(), pos);
}

Binary_String& Binary_String::append(const char* str, size_t subpos, size_t sublen)
{
    for (size_t i = subpos; i < subpos + sublen; i++)
        push_back(str[i]);
    return *this;
}

Binary_String& Binary_String::append(const std::string& str, size_t subpos, size_t sublen)
{
    return append(str.c_str(), subpos, sublen);
}

Binary_String& Binary_String::append(const Binary_String& str, size_t subpos, size_t sublen)
{
    for (size_t i = subpos; i < subpos + sublen; i++)
        push_back(str[i]);
    return *this;
}


// void check_timeout(Server *server)
// {
//     (void)server;
//     // std::map<int, FileTransferState>::iterator it = server->fileTransfers.begin();
//     // (void)it;
//     // time_t current_time = time(NULL);
//     // while (it != server->fileTransfers.end())
//     // {
//     //     if (current_time - it->second.last_activity_time > TIMEOUT)
//     //     {
//     //         // std::cerr << "Client " << it->first << " timed out." << std::endl;
//     //         // close(it->first);
//     //         // std::map<int, FileTransferState>::iterator tmp = it;
//     //         // returnTimeoutRequest(tmp->second.fd, server);
//     //         // ++it;
//     //         // close(it->first);
//     //         // server->fileTransfers.erase(tmp);
//     //     }
//     //     else
//     //         ++it;
//     // }
// }

// int returnTimeoutRequest(int fd, Server *server)
// {
//     std::string path1 = PATHE;
//     std::string path2 = "408.html";
//     std::string new_path = path1 + path2;
//     std::string content = server->readFile(new_path);
//     std::string httpResponse = server->createTimeoutResponse(server->getContentType(new_path), content.length());
//     if (!httpResponse.empty())
//     {
//         if (send(fd, httpResponse.c_str(), httpResponse.length(), MSG_NOSIGNAL) == -1)
//         {
//             return std::cerr << "Failed to send error response header" << std::endl, server->fileTransfers.erase(fd), close(fd), 0;
//         }

//         if (send(fd, content.c_str(), content.length(), MSG_NOSIGNAL) == -1)
//             return std::cerr << "Failed to send error content" << std::endl, server->fileTransfers.erase(fd), close(fd), 0;

//         if (send(fd, "\r\n\r\n", 2, MSG_NOSIGNAL) == -1)
//             return server->fileTransfers.erase(fd), close(fd), 0;
//     }

//     return 0;
// }