#ifndef BINARY_STRING_HPP
#define BINARY_STRING_HPP

#include "globalInclude.hpp"

class Binary_String
{
    private:
        std::vector <uint8_t > buffer;
        static const size_t npos = -1;
    public:
        Binary_String(const char* str, size_t n);
        Binary_String();
        Binary_String(const Binary_String& other);
        Binary_String(size_t n);
        ~Binary_String();
        size_t find(const char* s, size_t pos = 0) const;
        size_t find(const std::string& s, size_t pos = 0) const;
        Binary_String substr(size_t pos, size_t n) const;
        Binary_String& append(const char* str, size_t subpos, size_t sublen);
        Binary_String& append(const std::string& str, size_t subpos, size_t sublen);
        Binary_String& append(const Binary_String& str, size_t subpos, size_t sublen);
        void clear();
        std::string to_string() const;
        const char* c_str() const;
        size_t length() const;
        uint8_t operator[](size_t i) const;
        uint8_t& operator[](size_t i);
        uint8_t* data();
        bool empty() const;
        std::vector <uint8_t >::iterator begin();
        std::vector <uint8_t >::iterator end();
        void push_back(uint8_t c);
        Binary_String operator+(const Binary_String& other) const;
        Binary_String operator+=(const Binary_String& other);
        bool operator==(const Binary_String& other) const;
        bool operator!=(const Binary_String& other) const;

};

std::ostream& operator<<(std::ostream& os, const Binary_String& buffer);


#endif