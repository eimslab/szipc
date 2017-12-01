#pragma once

#include <vector>
// The zlib library must be installed, for example(for macos): brew install zlib
// link flag: -lz
#include <zlib.h>

using namespace std;

#define PUT_DIR_T   1
#define PUT_FILE_T  2

class Szip {
public:
    static bool fileExists(const string& filename);
    static uint fileLength(const string& filename);
    static int createDirectories(const string& path);
    static string buildPath(const string& root, const string& subPath);
    static bool isDir(const string& name);
    static bool isFile(const string& name);
    static void getFiles(const string& path, vector<string>& files);
    static void getDirs(const string& path, vector<string>& dirs);

    static int compressBytes  (unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static int uncompressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static void zip           (const string& sourceDirOrFileName, const string& outputFilename);
    static void unzip         (const string& szipFilename, const string& outputPath);

private:

    static void readFile(const string& dir, const string& rootDir, vector<unsigned char>& buffer);
    static void put(int type, const string& name, vector<unsigned char>& buffer);
};
