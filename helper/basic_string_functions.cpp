#include "../server.hpp"

binary_string::binary_string(const char* str, size_t n)
{
    buffer.resize(n);
    for (size_t i = 0; i < n; i++)
        buffer[i] = str[i];
}

binary_string::binary_string()
{
}

binary_string::binary_string(size_t n)
{
    buffer.resize(n);
}

void binary_string::clear()
{
    buffer.clear();
}

bool binary_string::empty() const
{
    return buffer.empty();
}

uint8_t binary_string::operator[](size_t i) const
{
    return buffer[i];
}

uint8_t& binary_string::operator[](size_t i)
{
    return buffer[i];
}

uint8_t* binary_string::data()
{
    return buffer.data();
}

std::vector <uint8_t >::iterator binary_string::begin()
{
    return buffer.begin();
}

std::vector <uint8_t >::iterator binary_string::end()
{
    return buffer.end();
}

size_t binary_string::size() const
{
    return buffer.size();
}

void binary_string::push_back(uint8_t c)
{
    buffer.push_back(c);
}

binary_string::binary_string(const binary_string& other) : buffer(other.buffer)
{
}

binary_string::~binary_string()
{
}

size_t binary_string::find(const char* s, size_t pos) const
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

binary_string binary_string::substr(size_t pos, size_t n) const
{
    binary_string result;
    for (size_t i = pos; i < pos + n && i < buffer.size(); i++)
        result.push_back(buffer[i]);
    return result;
}

binary_string binary_string::operator+(const binary_string& other) const
{
    binary_string result;
    for (size_t i = 0; i < buffer.size(); i++)
        result.push_back(buffer[i]);
    for (size_t i = 0; i < other.buffer.size(); i++)
        result.push_back(other.buffer[i]);
    return result;
}

binary_string binary_string::operator+=(const binary_string& other)
{
    for (size_t i = 0; i < other.buffer.size(); i++)
        push_back(other.buffer[i]);
    return *this;
}

bool binary_string::operator==(const binary_string& other) const
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

bool binary_string::operator!=(const binary_string& other) const
{
    return !(*this == other);
}

std::ostream& operator<<(std::ostream& os, const binary_string& buffer)
{
    for (size_t i = 0; i < buffer.size(); i++)
    {
        if (isprint(buffer[i]))
            os << buffer[i];
        else
            os << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }
    return os;
}

std::string binary_string::to_string() const
{
    std::string result;
    for (size_t i = 0; i < buffer.size(); i++)
        result += buffer[i];
    return result;
}

const char* binary_string::c_str() const
{
    return reinterpret_cast<const char*>(buffer.data());
}

size_t binary_string::find(const std::string& s, size_t pos) const
{
    return find(s.c_str(), pos);
}

binary_string& binary_string::append(const char* str, size_t subpos, size_t sublen)
{
    for (size_t i = subpos; i < subpos + sublen; i++)
        push_back(str[i]);
    return *this;
}

binary_string& binary_string::append(const std::string& str, size_t subpos, size_t sublen)
{
    return append(str.c_str(), subpos, sublen);
}

binary_string& binary_string::append(const binary_string& str, size_t subpos, size_t sublen)
{
    for (size_t i = subpos; i < subpos + sublen; i++)
        push_back(str[i]);
    return *this;
}
