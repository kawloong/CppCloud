
#ifndef _MAKE_ZIP_H
#define _MAKE_ZIP_H
#include <string>

using std::string;

class MakeZip
{
public:
    MakeZip(void);
    MakeZip(const string& pwd);

    int Zip(const string& zname, const string& dfile, bool rm);

    int UnZip(const string& zname);

private:
    string m_pwd;
};

#endif