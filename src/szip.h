#pragma once

#include <vector>
#include <string>

using namespace std;

#define PUT_DIR_T   1
#define PUT_FILE_T  2

class Szip
{
public:
    static int compressBytes  (unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static int uncompressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output);
    static void zip           (const string& sourceDirOrFileName, const string& outputFilename);
    static void unzip         (const string& szipFilename, const string& outputPath);

private:

    static void readFile(const string& dir, const string& rootDir, vector<unsigned char>& buffer);
    static void put(int type, const string& name, vector<unsigned char>& buffer);
};
