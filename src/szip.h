#pragma once

#include <vector>
// The zlib library must be installed, for example(for macos): brew install zlib
// link flag: -lz
#include <zlib.h>

using namespace std;

class Szip {
public:
    static bool fileExists(const string& filename);
    static uint fileLength(const string& filename);
    static int createDirectories(const string& path);
    static string buildPath(const string& root, const string& subPath);

    static int compressBytes  (unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static int uncompressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static void zip           (const string& sourceDirOrFileName, const string& outputFilename);
    static void unzip         (const string& szipFilename, const string& outputPath);

private:
    static const unsigned char magic[] = { 12, 29 };

};
