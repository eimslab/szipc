#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdint>
#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

#include "szip.h"
#include "bytes.h"

bool Szip::fileExists(const string& filename) {
    return (access(filename.c_str(), F_OK) != -1);
}

uint Szip::fileLength(const string& filename) {
    ifstream is;
    is.open(filename, ios::binary );
    is.seekg (0, ios::end);
    int len = is.tellg();
    is.close();

    return len;
}

int Szip::createDirectories(const string& path) {
    string dir = path;
    if (dir[dir.length() - 1] != '\\' || dir[dir.length() - 1] != '/')
        dir += "/";

    int len = dir.length();
    char tmp[256] = { 0 };

    for (int i = 0; i < len; ++i) {
        tmp[i] = dir[i];

        if (tmp[i] == '\\' || tmp[i] == '/') {
            if (!fileExists(tmp)) {
                #ifdef _WIN32
                    int ret = _mkdir(tmp);
                #else
                    int ret = mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                #endif
                if (ret != 0) {
                    return ret;
                }
            }
        }
    }

    return 0;
}

string Szip::buildPath(const string& root, const string& subPath) {
    if (root[root.length() - 1] == '\\' || root[root.length() - 1] == '/' || subPath[0] == '\\' || subPath[0] == '/') {
        return root + subPath;
    }

    return root + "/" + subPath;
}

int Szip::compressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output) {
    unsigned long output_len = compressBound(len);
    unsigned char* buffer = new unsigned char[output_len];

    int result = compress(buffer, &output_len, input, len);
    if (result != Z_OK) {
        delete[] buffer;

        return result;
    }

    output.reserve(output_len);
    for (int i = 0; i < output_len; i ++) {
        output.push_back(buffer[i]);
    }
    delete[] buffer;

    return Z_OK;
}

int Szip::uncompressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output) {
    if (len <= 0) {
        return 0;
    }

    unsigned long output_len = len * 100;
    unsigned char* buffer = new unsigned char[output_len];

    int result = uncompress(buffer, &output_len, input, len);

    if (result == Z_DATA_ERROR) {
        delete[] buffer;

        return result;
    }

    while (result == Z_BUF_ERROR) {
        output_len *= 10;
        delete[] buffer;
        buffer = new unsigned char[output_len];

        result = uncompress(buffer, &output_len, input, len);
    }

    if (result != Z_OK) {
        delete[] buffer;

        return result;
    }

    output.reserve(output_len);
    for (int i = 0; i < output_len; i ++) {
        output.push_back(buffer[i]);
    }
    delete[] buffer;

    return Z_OK;
}

void Szip::zip(const string& sourceDirOrFileName, const string& outputFilename) {
    assert(fileExists(sourceDirOrFileName));
    assert(outputFilename != "");

    ubyte[] buffer;

    if (std.file.isFile(sourceDirOrFileName)) {
        put!"file"(sourceDirOrFileName, buffer);
    } else {
        readFile(sourceDirOrFileName, string.init, buffer);
    }

    std.file.write(outputFilename, magic);
    std.file.append(outputFilename, compress(buffer));
}

void Szip::unzip(const string& zipFilename, const string& outputPath) {
    assert(fileExists(zipFilename));
    int len = fileLength(zipFilename);
    assert(len > 2);

    unsigned char* data = new unsigned char[len];
    ifstream fin(zipFilename, ios::binary);
    fin.read((char *)data, len);

    assert(data[0] == magic[0] && data[1] == magic[1]);

    vector<unsigned char> buffer;
    uncompressBytes(data + 2, len - 2, buffer);


    if (!fileExists(outputPath))   createDirectories(outputPath);

    if (buffer.size() == 0) {
        return;
    }

    string dir = outputPath;
    int pos = 0;
    while (pos < buffer.size()) {
        unsigned char type = buffer[pos++];
        ushort len = szip::Bytes::peek<unsigned char>(buffer.data(), pos);
        pos += 2;
        string name((char*)buffer.data() + pos, 0, len);
        pos += len;

        if (type == 0x01) {
            dir = buildPath(outputPath, name);
            if (!fileExists(dir))  createDirectories(dir);
        } else {
            string filename = buildPath(dir, name);
            uint file_len = szip::Bytes::peek<unsigned int>(buffer.data(), pos);
            pos += 4;

            ofstream fout;
            fout.open(filename, ios::binary);
            for (int i = 0; i < file_len; i++) {
                fout << buffer[i + pos];
            }
            fout << flush;
            fout.close();
            pos += file_len;
        }
    }
}

// private:

