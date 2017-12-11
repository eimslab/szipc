#pragma once

#include <vector>
#include <string>

using namespace std;

bool            fileExists(const string& filename);
unsigned int    fileLength(const string& filename);
int             createDirectories(const string& path);
string          buildPath(const string& root, const string& subPath);
string          baseName(const string& path);
string          dirName(const string& path);
bool            isDir(const string& name);
bool            isFile(const string& name);
void            getFiles(const string& path, vector<string>& files);
void            getDirs(const string& path, vector<string>& dirs);
string          thisExePath();
#ifdef _WIN32
string          ansi2utf8(const string& ansi);
string          utf82ansi(const string& utf8);
#endif
