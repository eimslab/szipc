#include <cassert>
#include <iostream>
#include <fstream>
// The zlib library must be installed, for example(for macos): brew install zlib
// link flag: -lz
#include <zlib.h>

#include "bytes.h"
#include "filesystem.h"

#include "szip.h"

#ifdef _MSC_VER
    #pragma comment(lib, "zlib.lib")
#endif

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#define DLL_EXPORT __declspec(dllexport) __stdcall
#ifdef __cplusplus
extern "C"
{
#endif
void DLL_EXPORT zip(char* sourceDirOrFileName, char* outputFilename);
void DLL_EXPORT unzip(char* szipFilename, char* outputPath);
#ifdef __cplusplus
}
#endif

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}

#else

extern "C" void zip(char* sourceDirOrFileName, char* outputFilename);
extern "C" void unzip(char* szipFilename, char* outputPath);

#endif

void zip(char* sourceDirOrFileName, char* outputFilename)
{
    Szip::zip(sourceDirOrFileName, outputFilename);
}

void unzip(char* szipFilename, char* outputPath)
{
    Szip::unzip(szipFilename, outputPath);
}

int Szip::compressBytes(unsigned char* input, size_t len, vector<unsigned char>& output)
{
    unsigned long output_len = compressBound(len);
    unsigned char* buffer = new unsigned char[output_len];

    int result = compress(buffer, &output_len, input, len);
    if (result != Z_OK)
    {
        delete[] buffer;

        return result;
    }

    output.reserve(output_len);
    for (size_t i = 0; i < output_len; i ++)
    {
        output.push_back(buffer[i]);
    }
    delete[] buffer;

    return Z_OK;
}

int Szip::uncompressBytes(unsigned char* input, size_t len, vector<unsigned char>& output)
{
    if (len <= 0)
    {
        return 0;
    }

    unsigned long output_len = len * 10;
    unsigned char* buffer = new unsigned char[output_len];

    int result = uncompress(buffer, &output_len, input, len);

    if (result == Z_DATA_ERROR)
    {
        delete[] buffer;

        return result;
    }

    while (result == Z_BUF_ERROR)
    {
        output_len *= 10;
        delete[] buffer;
        buffer = new unsigned char[output_len];

        result = uncompress(buffer, &output_len, input, len);
    }

    if (result != Z_OK)
    {
        delete[] buffer;

        return result;
    }

    output.reserve(output_len);
    for (size_t i = 0; i < output_len; i ++)
    {
        output.push_back(buffer[i]);
    }
    delete[] buffer;

    return Z_OK;
}

void Szip::zip(const string& sourceDirOrFileName, const string& outputFilename)
{
    assert(fileExists(sourceDirOrFileName));
    assert(outputFilename != "");

    unsigned char const magic[] = { 12, 29 };
    vector<unsigned char> buffer;

    if (isFile(sourceDirOrFileName))
    {
        put(PUT_FILE_T, sourceDirOrFileName, buffer);
    }
    else
    {
        readFile(sourceDirOrFileName, "", buffer);
    }

    vector<unsigned char> compressed;
    int len = compressBytes(buffer.data(), (unsigned long)buffer.size(), compressed);
    if (len != Z_OK)
    {
        assert(false);
    }

    remove(outputFilename.c_str());
    ofstream os;
    os.open(outputFilename, ios::out | ios::binary | ios::app);
    os.write((char*)magic, 2);
    os.write((char*)compressed.data(), compressed.size());
    os.close();
}

void Szip::unzip(const string& szipFilename, const string& outputPath)
{
    assert(fileExists(szipFilename));
    size_t len = fileLength(szipFilename);
    assert(len > 2);

    unsigned char const magic[] = { 12, 29 };
    unsigned char* data = new unsigned char[len];
    ifstream fin(szipFilename, ios::binary);
    fin.read((char *)data, len);

    assert(data[0] == magic[0] && data[1] == magic[1]);

    vector<unsigned char> buffer;
    int ret = uncompressBytes(data + 2, len - 2, buffer);
    if (ret != Z_OK)
    {
        assert(false);
    }

    if (!fileExists(outputPath))
    {
        createDirectories(outputPath);
    }

    if (buffer.size() == 0)
    {
        return;
    }

    string dir = outputPath;
    size_t pos = 0;
    while (pos < buffer.size())
    {
        unsigned char type = buffer[pos++];
        unsigned short len = szip::Bytes::peek<unsigned short>(buffer.data(), pos);
        pos += 2;
#ifdef _WIN32
        string name = utf82ansi(string((char*)buffer.data() + pos, 0, len));
#else
        string name((char*)buffer.data() + pos, 0, len);
#endif

        pos += len;

        if (type == 0x01)
        {
            dir = buildPath(outputPath, name);
            if (!fileExists(dir))  createDirectories(dir);
        }
        else
        {
            string filename = buildPath(dir, name);
            size_t file_len = szip::Bytes::peek<unsigned int>(buffer.data(), pos);
            pos += 4;

            ofstream fout;
            fout.open(filename, ios::binary);
            for (size_t i = 0; i < file_len; i++)
            {
                fout << buffer[i + pos];
            }
            fout << flush;
            fout.close();
            pos += file_len;
        }
    }
}

// private:

void Szip::readFile(const string& dir, const string& rootDir, vector<unsigned char>& buffer)
{
    vector<string> files;
    getFiles(dir, files);
    for (size_t i = 0; i < files.size(); i++)
    {
    	    put(PUT_FILE_T, files[i], buffer);
    }

    vector<string> dirs;
    getDirs(dir, dirs);
    for (size_t i = 0; i < dirs.size(); i++)
    {
        string t = buildPath(rootDir, baseName(dirs[i]));
        put(PUT_DIR_T, t, buffer);
        readFile(dirs[i], t, buffer);
    }
}

void Szip::put(int type, const string& name, vector<unsigned char>& buffer)
{
    assert(type == PUT_DIR_T || type == PUT_FILE_T);

    buffer.push_back((unsigned char)type);
#ifdef _WIN32
    string t = ansi2utf8((type == PUT_FILE_T) ? baseName(name) : name);
#else
    string t = (type == PUT_FILE_T) ? baseName(name) : name;
#endif
    szip::Bytes::write<unsigned short>((unsigned short)t.length(), buffer, buffer.size());

    for (size_t i = 0; i < t.length(); i++)
    {
        buffer.push_back(t[i]);
    }

    if (type == PUT_FILE_T)
    {
        ifstream is;
        is.open(name, ios::binary);
        is.seekg(0, ios::end);
        int len = (int)is.tellg();
        is.seekg(0, ios::beg);
        char* content = new char[len];
        is.read(content, len);
        is.close();
        szip::Bytes::write<unsigned int>((unsigned int)len, buffer, buffer.size());
        for (int i = 0; i < len; i++)
        {
            buffer.push_back(content[i]);
        }
        delete[] content;
    }
}
