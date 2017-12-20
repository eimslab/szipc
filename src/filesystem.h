#pragma once

#include <vector>
#include <string>

using namespace std;

bool                fileExists(const string& filename);
unsigned long long  fileLength(const string& filename);
int                 createDirectory(const string& path);
int                 createDirectories(const string& path);
int                 removeDirectory(const string& path);
void                copyFile(const string& src, const string& dst);
void                copyDirectory(const string& src, const string& dst);
string              buildPath(const string& root, const string& subPath);
string              baseName(const string& path);
string              dirName(const string& path);
bool                isDir(const string& name);
bool                isFile(const string& name);
void                getFiles(const string& path, vector<string>& files);
void                getDirs(const string& path, vector<string>& dirs);
string              thisExePath();
#ifdef _WIN32
bool                isUTF8(const void* pBuffer, long size);
string              ansi2utf8(const string& ansi);
string              utf82ansi(const string& utf8);
#endif
