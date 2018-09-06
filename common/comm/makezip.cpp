#include "makezip.h"
#include <cstdlib>

MakeZip::MakeZip(void)
{

}

MakeZip::MakeZip(const string& pwd): m_pwd(pwd)
{
}

int MakeZip::Zip( const string& zname, const string& dfile, bool rm )
{
    int ret = -1;
    if (!zname.empty() && !dfile.empty())
    {
        std::string cmd("zip -j");

        if (rm)
        {
            cmd.append("m");
        }
        if (!m_pwd.empty())
        {
            cmd.append("P ").append(m_pwd);
        }

        cmd.append(" ").append(zname).append(" ").append(dfile);
        ret = system(cmd.c_str());
    }

    return ret;
}

int MakeZip::UnZip( const string& zname )
{
    int ret = -1;
    if (!zname.empty())
    {
        std::string cmd("unzip -o");
        if (!m_pwd.empty())
        {
            cmd.append("P ").append(m_pwd);
        }

        cmd.append(" ").append(zname);
        ret = system(cmd.c_str());
    }

    return ret;
}
