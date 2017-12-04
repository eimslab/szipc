#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <libgen.h>
#include <cstring>
#include <sys/stat.h>
// The zlib library must be installed, for example(for macos): brew install zlib
// link flag: -lz
#include <zlib.h>
#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
	#include <windows.h>
#else
    #include <sys/types.h>
    #include <dirent.h>
#endif

#include "szip.h"
#include "bytes.h"


#ifdef _WIN32

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

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
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

void zip(char* sourceDirOrFileName, char* outputFilename) {
    Szip::zip(sourceDirOrFileName, outputFilename);
}

void unzip(char* szipFilename, char* outputPath) {
    Szip::unzip(szipFilename, outputPath);
}


bool Szip::fileExists(const string& filename) {
    return (access(filename.c_str(), F_OK) != -1);
}

unsigned int Szip::fileLength(const string& filename) {
    ifstream is;
    is.open(filename, ios::binary);
    is.seekg(0, ios::end);
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
    if (root.empty() || subPath.empty()) {
        return root + subPath;
    }

    if (root[root.length() - 1] == '\\' || root[root.length() - 1] == '/' || subPath[0] == '\\' || subPath[0] == '/') {
        return root + subPath;
    }

    return root + "/" + subPath;
}

string Szip::baseName(const string& path) {
#ifdef _WIN32
	if (path.length() == 0 || path == "\\" || path == "/") {
		return "";
	}

	char* t = strdup(path.c_str());
	if (t[strlen(t) - 1] == '\\' || t[strlen(t) - 1] == '/') {
		t[strlen(t) - 1] = '\0';
	}

	int i = strlen(t);
	while (--i >= 0) {
		if (t[i] == '\\' || t[i] == '/') {
			break;
		}
	}

	string ret(t + i + 1);
	delete t;
	return ret;
#else
	return basename((char*)path.c_str());
#endif
}

bool Szip::isDir(const string& name) {
    struct stat buf = { 0 };
    stat(name.c_str(), &buf);
    return buf.st_mode & S_IFDIR;
}

bool Szip::isFile(const string& name) {
    struct stat buf = { 0 };
    stat(name.c_str(), &buf);
    return buf.st_mode & S_IFREG;
}

void Szip::getFiles(const string& path, vector<string>& files) {
#ifdef _WIN32
    intptr_t handle;
    _finddata_t fileinfo;

    string dir = path;
    if (dir[dir.length() - 1] != '\\' || dir[dir.length() - 1] != '/') {
        dir += "/";
    }
    dir += "*.*";

    handle = _findfirst(dir.c_str(), &fileinfo);
    if (handle == -1) {
        return;
    }

    do {
        if (!(fileinfo.attrib & _A_SUBDIR)) {
            files.push_back(buildPath(path, fileinfo.name));
        }
    } while (_findnext(handle, &fileinfo) == 0);

    _findclose(handle);
#else
    DIR* dir;
    if (!(dir = opendir(path.c_str()))) {
        assert(false);
    }

    struct dirent* d_ent;
    string temppath = path;
    if (temppath[temppath.length() - 1] != '\\' || temppath[temppath.length() - 1] != '/') {
        temppath += "/";
    }

    while ((d_ent = readdir(dir)) != NULL) {
        struct stat file_stat;
        if (strncmp(d_ent->d_name, ".", 1) == 0 || strncmp(d_ent->d_name, "..", 2) == 0) {
            continue;
        }

        string name = temppath + d_ent->d_name;
        if (lstat(name.c_str(), &file_stat) < 0) {
            assert(false);
        }

        if (!S_ISDIR(file_stat.st_mode)) {
            files.push_back(name);
        }
    }

    closedir(dir);
#endif
}

void Szip::getDirs(const string& path, vector<string>& dirs) {
#ifdef _WIN32
    intptr_t handle;
    _finddata_t fileinfo;

    string dir = path;
    if (dir[dir.length() - 1] != '\\' || dir[dir.length() - 1] != '/') {
        dir += "/";
    }
    dir += "*.*";

    handle = _findfirst(dir.c_str(), &fileinfo);
    if (handle == -1) {
        return;
    }

    do {
        if (fileinfo.attrib & _A_SUBDIR) {
            if (strcmp(fileinfo.name, ".") && strcmp(fileinfo.name, "..")) {
            	dirs.push_back(buildPath(path, fileinfo.name));
            }
        }
    } while (_findnext(handle, &fileinfo) == 0);

    _findclose(handle);
#else
    DIR* dir;
    if (!(dir = opendir(path.c_str()))) {
        assert(false);
    }

    struct dirent* d_ent;
    string temppath = path;
    if (temppath[temppath.length() - 1] != '\\' || temppath[temppath.length() - 1] != '/') {
        temppath += "/";
    }

    while ((d_ent = readdir(dir)) != NULL) {
        struct stat file_stat;
        if (strncmp(d_ent->d_name, ".", 1) == 0 || strncmp(d_ent->d_name, "..", 2) == 0) {
        	continue;
        }

        string name = temppath + d_ent->d_name;
        if (lstat(name.c_str(), &file_stat) < 0) {
            assert(false);
        }

        if (S_ISDIR(file_stat.st_mode)) {
            dirs.push_back(name);
        }
    }

    closedir(dir);
#endif
}

#ifdef _WIN32
string Szip::ansi2utf8(const string& ansi) {
	int len = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), ansi.length(), NULL, 0);

	WCHAR* lpszW = new WCHAR[len];
	assert(MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), ansi.length(), lpszW, len) == len);

	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, lpszW, len, NULL, 0, NULL, NULL);
	assert(utf8_len > 0);

	char* utf8_string = new char[utf8_len];
	assert(WideCharToMultiByte(CP_UTF8, 0, lpszW, len, utf8_string, utf8_len, NULL, NULL) == utf8_len);

	string ret(utf8_string);

	delete[] lpszW;
	delete[] utf8_string;

	return ret;
}

string Szip::utf82ansi(const string& utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)utf8.c_str(), utf8.length(), NULL, 0);

    WCHAR* lpszW = new WCHAR[len];
    assert(MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR)utf8.c_str(), utf8.length(), lpszW, len) == len);

    int ansi_len = WideCharToMultiByte(CP_ACP, 0, lpszW, len, NULL, 0, NULL, NULL);
    assert(ansi_len > 0);

    char* ansi_string = new char[ansi_len];
    assert(WideCharToMultiByte(CP_ACP, 0, lpszW, len, ansi_string, ansi_len, NULL, NULL) == ansi_len);

    string ret(ansi_string);

    delete[] lpszW;
    delete[] ansi_string;

    return ret;
}
#endif

int Szip::compressBytes(unsigned char* input, unsigned long len, vector<unsigned char>& output) {
    unsigned long output_len = compressBound(len);
    unsigned char* buffer = new unsigned char[output_len];

    int result = compress(buffer, &output_len, input, len);
    if (result != Z_OK) {
        delete[] buffer;

        return result;
    }

    output.reserve(output_len);
    for (unsigned long i = 0; i < output_len; i ++) {
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
    for (unsigned long i = 0; i < output_len; i ++) {
        output.push_back(buffer[i]);
    }
    delete[] buffer;

    return Z_OK;
}

void Szip::zip(const string& sourceDirOrFileName, const string& outputFilename) {
    assert(fileExists(sourceDirOrFileName));
    assert(outputFilename != "");

    unsigned char const magic[] = { 12, 29 };
    vector<unsigned char> buffer;

    if (isFile(sourceDirOrFileName)) {
        put(PUT_FILE_T, sourceDirOrFileName, buffer);
    } else {
        readFile(sourceDirOrFileName, "", buffer);
    }

    vector<unsigned char> compressed;
    int len = compressBytes(buffer.data(), buffer.size(), compressed);
    if (len != Z_OK) {
        assert(false);
    }

    remove(outputFilename.c_str());
    ofstream os;
    os.open(outputFilename, ios::out | ios::binary | ios::app);
    os.write((char*)magic, 2);
    os.write((char*)compressed.data(), compressed.size());
    os.close();
}

void Szip::unzip(const string& szipFilename, const string& outputPath) {
    assert(fileExists(szipFilename));
    int len = fileLength(szipFilename);
    assert(len > 2);

    unsigned char const magic[] = { 12, 29 };
    unsigned char* data = new unsigned char[len];
    ifstream fin(szipFilename, ios::binary);
    fin.read((char *)data, len);

    assert(data[0] == magic[0] && data[1] == magic[1]);

    vector<unsigned char> buffer;
    len = uncompressBytes(data + 2, len - 2, buffer);
    if (len != Z_OK) {
        assert(false);
    }

    if (!fileExists(outputPath))   createDirectories(outputPath);

    if (buffer.size() == 0) {
        return;
    }

    string dir = outputPath;
    size_t pos = 0;
    while (pos < buffer.size()) {
        unsigned char type = buffer[pos++];
        unsigned short len = szip::Bytes::peek<unsigned short>(buffer.data(), pos);
        pos += 2;
#ifdef _WIN32
        string name = utf82ansi(string((char*)buffer.data() + pos, 0, len));
#else
        string name((char*)buffer.data() + pos, 0, len);
#endif

        pos += len;

        if (type == 0x01) {
            dir = buildPath(outputPath, name);
            if (!fileExists(dir))  createDirectories(dir);
        } else {
            string filename = buildPath(dir, name);
            unsigned int file_len = szip::Bytes::peek<unsigned int>(buffer.data(), pos);
            pos += 4;

            ofstream fout;
            fout.open(filename, ios::binary);
            for (unsigned int i = 0; i < file_len; i++) {
                fout << buffer[i + pos];
            }
            fout << flush;
            fout.close();
            pos += file_len;
        }
    }
}

// private:

void Szip::readFile(const string& dir, const string& rootDir, vector<unsigned char>& buffer) {
    vector<string> files;
    getFiles(dir, files);
    for (size_t i = 0; i < files.size(); i++) {
    	put(PUT_FILE_T, files[i], buffer);
    }

    vector<string> dirs;
    getDirs(dir, dirs);
    for (size_t i = 0; i < dirs.size(); i++) {
        string t = buildPath(rootDir, baseName(dirs[i]));
        put(PUT_DIR_T, t, buffer);
        readFile(dirs[i], t, buffer);
    }
}

void Szip::put(int type, const string& name, vector<unsigned char>& buffer) {
    assert(type == PUT_DIR_T || type == PUT_FILE_T);

    buffer.push_back((unsigned char)type);
#ifdef _WIN32
    string t = ansi2utf8((type == PUT_FILE_T) ? baseName(name) : name);
#else
    string t = (type == PUT_FILE_T) ? baseName(name) : name;
#endif
    szip::Bytes::write<unsigned short>((unsigned short)t.length(), buffer, -1);

    for (size_t i = 0; i < t.length(); i++) {
        buffer.push_back(t[i]);
    }

    if (type == PUT_FILE_T) {
        ifstream is;
        is.open(name, ios::binary);
        is.seekg(0, ios::end);
        int len = is.tellg();
        is.seekg(0, ios::beg);
        char* content = new char[len];
        is.read(content, len);
        is.close();
        szip::Bytes::write<unsigned int>((unsigned int)len, buffer, -1);
        for (int i = 0; i < len; i++) {
            buffer.push_back(content[i]);
        }
        delete[] content;
    }
}


// test:

//#include "szip.h"
//#include <string>
//
//int main()
//{
//    Szip::zip("/Users/shove/Desktop/123", "/Users/shove/Desktop/123.szip");
//    Szip::unzip("/Users/shove/Desktop/123.szip", "/Users/shove/Desktop/124");
//}
