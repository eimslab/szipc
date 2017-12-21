#pragma once

#include <vector>

using namespace std;

namespace szip
{

class Bytes
{
public:
    template <typename T>
    static size_t write(const T& value, vector<unsigned char>& buffer, size_t offset)
    {
        while (buffer.size() < offset + sizeof(T))
        {
            buffer.push_back((unsigned char)0);
        }

        unsigned char* p = (unsigned char*)&value;

        size_t i = 0;
        while (i < sizeof(T))
        {
            buffer[offset + sizeof(T) - i - 1] = p[i];
            i++;
        }

        return sizeof(T);
    }

    template <typename T>
    static T peek(unsigned char* buffer, size_t offset)
    {
        T t;
        unsigned char* p = (unsigned char*)&t;

        size_t i = 0;
        while (i < sizeof(T))
        {
            p[i] = buffer[offset + sizeof(T) - i - 1];
            i++;
        }

        return t;
    }
};

}
