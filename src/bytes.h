#pragma once

#include <vector>
#include <cassert>
#include <istream>

using namespace std;

namespace szip {

class Bytes {
public:
    template <typename T>
    static unsigned int write(const T& value, vector<unsigned char>& buffer, int offset) {
        if (offset < 0) {
            offset = buffer.size();
        }
        while (buffer.size() < offset + sizeof(T)) {
            buffer.push_back((unsigned char)0);
        }

        unsigned char* p = (unsigned char*)&value;

        uint i = 0;
        while (i < sizeof(T)) {
            buffer[offset + sizeof(T) - i - 1] = p[i];
            i++;
        }

        return sizeof(T);
    }

    template <typename T>
    static T peek(unsigned char* buffer, int offset) {
        if (offset < 0) {
            offset = 0;
        }

        T t;
        unsigned char* p = (unsigned char*)&t;

        uint i = 0;
        while (i < sizeof(T)) {
            p[i] = buffer[offset + sizeof(T) - i - 1];
            i++;
        }

        return t;
    }
};

}
