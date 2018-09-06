
#include <cstring>
#include <cstdio>
#include "config.h"

#define MAX_BUF 1024

Config::Config(void)
{
    m_pPath = NULL;

    //pthread_mutex_init(&this->m_lstMutex, NULL);
}

Config::~Config(void)
{
    if (m_pPath)
    {
        delete []m_pPath;
    }

    //pthread_mutex_destroy(&this->m_lstMutex);
}

int Config::load(const char *pPath)
{
    char szBuf[MAX_BUF] = {0};
    FILE *fp;
    char *p1 = NULL;
    char *p2 = NULL;
    string strItem;
    string strKey;
    string strValue;
    string strTemp;

    if (NULL == pPath)
    {
        return -1;
    }

    fp = fopen(pPath, "r");
    if (NULL == fp)
    {
        return -2;
    }

    //pthread_mutex_lock(&this->m_lstMutex);

    m_lstConfig.clear();

    while (fgets(szBuf, MAX_BUF - 1, fp) != NULL)
    {
        if ('#' == *szBuf || '\0' == *szBuf)
        {
            continue;
        }

        p1 = strchr(szBuf, '[');
        if (p1 != NULL)
        {
            p2 = strrchr(szBuf, ']');
            if (p2 != NULL && p2 > p1)
            {
//                 strItem.clear();
//                 copy(p1 + 1, p2, back_inserter(strItem));
                strItem.assign(p1+1, p2-p1-1);
            }
            else
            {
                fclose(fp);
                //pthread_mutex_unlock(&this->m_lstMutex);
                return -3;
            }
        }
        else
        {
            p1 = strchr(szBuf, '=');
            if (p1 != NULL)
            {
                size_t sztmp;
//                 strKey.clear();
//                 copy(szBuf, p1, back_inserter(strKey));
                strKey.assign(szBuf, p1-szBuf);

                if (strchr(strKey.c_str(), '#') != NULL)
                {
                    continue;
                }

                strKey.erase(0, strKey.find_first_not_of(" \t\r\n"));
                strKey.erase(strKey.find_last_not_of(" \t") + 1);

                strValue.assign(p1+1);
                sztmp = strValue.find_first_not_of(" \t\r\n");
                strValue.erase(0, sztmp);

                if (!strValue.empty())
                {
                    sztmp = strValue.find_first_of(" \t\r\n#");
                    strValue.erase(sztmp);
                }

                m_lstConfig.push_back(make_pair(strItem, make_pair(strKey, strValue)));
            }
        }
    }

    //pthread_mutex_unlock(&this->m_lstMutex);

    fclose(fp);

    if (m_pPath)
    {
        if (strcmp(pPath, m_pPath) == 0)
        {
            return 0;
        }

        delete [] m_pPath;
    }

    m_pPath = new char[strlen(pPath) + 1];
    strcpy(m_pPath, pPath);

    return 0;
}

int Config::reload(void)
{
    return this->load(m_pPath);
}

int Config::unload(void)
{
    m_lstConfig.clear();
    if (m_pPath)
    {
        delete [] m_pPath;
        m_pPath = NULL;
    }

    return 0;
}

int Config::read(const string &strItem, const string &strLvalue, string &strValue)
{
    List_Config_Item::iterator iter;

    //pthread_mutex_lock(&this->m_lstMutex);

    for (iter = m_lstConfig.begin(); iter != m_lstConfig.end(); ++iter)
    {
        if (strItem == iter->first)
        {
            if (strLvalue == iter->second.first)
            {
                strValue = iter->second.second;
                //pthread_mutex_unlock(&this->m_lstMutex);
                return 0;
            }
        }
    }

    //pthread_mutex_unlock(&this->m_lstMutex);
    strValue = "";

    return -1;
}

