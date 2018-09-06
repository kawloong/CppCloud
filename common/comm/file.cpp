#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include "file.h"


bool File::Exists(const char* name)
{
    struct stat st;
    return stat(name?name:"", &st) == 0;
}

bool File::Isdir(const char* dirname)
{
    struct stat st;
    if(stat(dirname, &st) == -1)
    {
        return false;
    }
    return (bool)S_ISDIR(st.st_mode);
}

bool File::Isfile(const char* filename)
{
    struct stat st;
    if(stat(filename, &st) == -1){
        return false;
    }
    return (bool)S_ISREG(st.st_mode);
}

bool File::CreatDir_r( const char* path )
{
    if (NULL == path) return false;

    int nHead, nTail;   
    std::string strDir(path);
    if ('/' != strDir[0] && '.' != strDir[0])
    {
        strDir = "./" + strDir;
    }
    if ('/' == strDir[strDir.size()-1])
    {
        strDir.erase(strDir.size()-1, 1);
    }

    DIR* pDir = opendir(path);

    if (NULL == pDir)
    {
        nHead = strDir.find("/");
        nTail = strDir.rfind("/");
        if (nHead != nTail)
        {
            if (false == CreatDir_r(strDir.substr(0, nTail).c_str()))
            {
                return false;
            }
        }

        if (-1 == mkdir(strDir.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH))
        {      
            return false;
        }
    }
    else
    {
        closedir(pDir);
    } 

    return true;
}

bool File::GetPath(const char* fullfile, string& path, bool nosep)
{
    struct stat buf;
    path = fullfile;

    unsigned length = path.length();
    int result = stat(fullfile, &buf);
    if (0 == result && S_ISDIR(buf.st_mode))
    {
        return true;
    }

    while (--length > 0)
    {
        if (path[length] == '/' || path[length] == '\\')
        {
            if (nosep)
            {
                path[length] = 0;
            }
            else
            {
                ++length;
            }
            break;
        }
        else
        {
            path[length] = 0;
        }
    }

    path.resize(length);

    return true;
}

bool File::GetFilename(const char* fullfile, string& name)
{
    int len = strlen(fullfile);
    while (--len >= 0)
    {
        if ('/' == fullfile[len] || '\\' == fullfile[len])
        {
            name = fullfile+len+1;
            return true;
        }
    }
    name = fullfile;
    return true;
}

bool File::Move( const char* srcfile, const char* dstfile )
{
    int result;

    result = rename(srcfile, dstfile);
    if (-1 == result && ENOENT == errno) // 目标目录缺少时先创建
    {
        string dstpath;
        GetPath(dstfile, dstpath, true);
        if (!dstpath.empty())
        {
            if (CreatDir_r(dstpath.c_str()))
            {
                result = rename(srcfile, dstfile);
            }
        }
    }

    return (0 == result);
}

//recursively delete all the file in the directory.
bool File::RemoveDir(const char* dir_full_path)
{
    struct dirent *dir;
    struct stat st;
    DIR* dirp = opendir(dir_full_path);  

    if(!dirp)
    {
        return false;
    }

    while((dir = readdir(dirp)) != NULL)
    {
        if(strcmp(dir->d_name,".") == 0
            || strcmp(dir->d_name,"..") == 0)
        {
            continue;
        }

        std::string sub_path = string(dir_full_path) + '/' + dir->d_name;
        if(lstat(sub_path.c_str(),&st) == -1)
        {
            continue;
        }

        if(S_ISDIR(st.st_mode))
        {
            if(!RemoveDir(sub_path.c_str())) // 如果是目录文件，递归删除
            {
                closedir(dirp);
                return false;
            }

            rmdir(sub_path.c_str());
        }
        else if(S_ISREG(st.st_mode))
        {
            unlink(sub_path.c_str());     // 如果是普通文件，则unlink
        }
        else
        {
            continue;
        }
    }

    if(rmdir(dir_full_path) == -1)//delete dir itself.
    {
        closedir(dirp);
        return false;
    }

    closedir(dirp);
    return true;
}

void File::AdjustPath( string& path, bool useend, char dv /*= '/'*/ )
{
    if (!path.empty())
    {
        size_t len = path.length();
        if (useend)
        {
            if (dv != path.at(len-1))
            {
                path.append(1, dv);
            }
        }
        else
        {
            while (dv == path.at(len-1))
            {
                path.erase(len-1);
                len -= 1;
            }
        }
    }
}
