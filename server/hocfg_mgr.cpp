#include "hocfg_mgr.h"
#include "comm/file.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <cerrno>

HocfgMgr::HocfgMgr( void )
{

}

HocfgMgr::~HocfgMgr( void )
{
    uninit();
}

int HocfgMgr::init( const string& conf_root )
{
    ERRLOG_IF1RET_N(!File::Isdir(conf_root.c_str()), -40, "CONFIGINIT| msg=path invalid| conf_root=%s", conf_root.c_str());
    m_cfgpath = conf_root;
    File::AdjustPath(m_cfgpath, true, '/');
    return loads(m_cfgpath);
}

void HocfgMgr::uninit( void )
{
    map<string, AppConfig*>::iterator itr = m_Allconfig.begin();
    for (; itr != m_Allconfig.end(); ++itr)
    {
        IFDELETE(itr->second);
    }
    m_Allconfig.clear();
}

int HocfgMgr::loads( const string& dirpath )
{
    int ret = 0;
    DIR *proot;
    struct dirent *pfile;

    ERRLOG_IF1RET_N(NULL == (proot = opendir(dirpath.c_str())), -41, 
        "CONFIGLOADS| msg=Can't open dir[%s] error:%s" , dirpath.c_str(), strerror(errno) );
    
    while((pfile = readdir(proot)) != NULL)
    {
        if( 0 == strcmp(pfile->d_name, ".") || 
            0 == strcmp(pfile->d_name, "..") )
        {
            continue;
        }

        string item = dirpath;
        StrParse::AdjustPath(item, true, '/');
        item += pfile->d_name;
        
        if (File::Isfile(item.c_str()))
        {
            if (strstr(pfile->d_name, ".json") > 0)
            {
                stringstream ss;
                ifstream ifs(item);
                if(ifs.bad())
                {
                    LOGERROR("CONFIGLOADS| msg=read %s fail %s", item.c_str(), strerror(errno));
                    continue;
                }

                ss << ifs.rdbuf();
                parseConffile(item, ss.str(), File::mtime(item.c_str()));
                ifs.close();
            }
        }
    }

    closedir(proot);
    return ret;
}

// 获取父级继承关系, 多个父级以空格分隔, 继承顺序由高至低(即祖父 父 子)
bool HocfgMgr::getBaseConfigName( string& baseCfg, const string& curCfgName )
{
    int ret = -1;
    map<string,AppConfig*>::iterator itr = m_Allconfig.find(HOCFG_METAFILE);
    if (m_Allconfig.end() != itr)
    {
        AppConfig* papp = itr->second;
        ret = Rjson::GetStr(baseCfg, curCfgName.c_str(), &papp->doc);
    }

    return (0 == ret && !baseCfg.empty());
}

// 解析json文档进内存map
int HocfgMgr::parseConffile( const string& filename, const string& contant, time_t mtime )
{
    Document fdoc;

    ERRLOG_IF1RET_N(fdoc.Parse(contant.c_str()).GetParseError(), -44, "CONFIGLOADS| msg=json read fail| file=%s", filename.c_str());
    AppConfig* papp = NULL;
    map<string, AppConfig*>::iterator itr = m_Allconfig.find(filename);
    if (m_Allconfig.end() == itr)
    {
        const size_t pathlen = m_cfgpath.length();
        size_t pos1 = filename.find(m_cfgpath);
        size_t pos2 = filename.find(".json");
        string key = (0 == pos1 && pos2 > pathlen)? filename.substr(pathlen, pos2-pathlen): filename;

        papp = new AppConfig;
        m_Allconfig[key] = papp;
        papp->mtime = mtime;
        papp->doc.Parse(contant.c_str());
    }
    else
    {
        papp = itr->second;
        if (papp->mtime < mtime)
        {
            papp->doc.SetObject();
            papp->doc.Parse(contant.c_str());
        }
    }

    return 0;
}

// json合并, 将node1的内存合并进node0
int HocfgMgr::mergeJsonFile( Value* node0, const Value* node1, MemoryPoolAllocator<>& allc )
{
    ERRLOG_IF1RET_N(!node1->IsObject(), -45, "MERGEJSON| msg=node1 not object| node1=%s", Rjson::ToString(node1).c_str());


    Value::ConstMemberIterator itr = node1->MemberBegin();
    for (; itr != node1->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();
        if (node0->HasMember(key)) // 后面可以考虑Object类型的合并?
        {
            node0->RemoveMember(key);
        }

        Value jkey, jval;
        jkey.CopyFrom(itr->name, allc);
        jval.CopyFrom(itr->value, allc);

        node0->AddMember(jkey, jval, allc);
    }

    return 0;
}

AppConfig* HocfgMgr::getConfigByName( const string& curCfgName )
{
    map<string, AppConfig*>::iterator itr = m_Allconfig.find(curCfgName);
    AppConfig* papp = (m_Allconfig.end() == itr)? NULL: itr->second;
    return papp;
}

// 分布式配置查询, file_pattern每个token以-分隔, key_pattern每个token以/分隔
// 返回的字符串可能是 {object} [array] "string" integer null
int HocfgMgr::query( string& result, const string& file_pattern, const string& key_pattern, bool incBase )
{
    int ret = 0;

    do
    {
        string baseStr;
        
        if (incBase && getBaseConfigName(baseStr, file_pattern))
        {
            Document doc;
            vector<string> vecBase;
            ret = StrParse::SpliteStr(vecBase, baseStr, ' ');
            ERRLOG_IF1BRK(ret, -48, "HOCFGQUERY| msg=invalid basestr setting| "
                "baseStr=%s| filep=%s", baseStr.c_str(), file_pattern.c_str());
            
            doc.SetObject();
            vecBase.push_back(file_pattern);
            vector<string>::const_iterator vitr = vecBase.begin();
            for (; vitr != vecBase.end(); ++vitr)
            {
                AppConfig* pnod1 = getConfigByName(*vitr);
                if (pnod1)
                {
                    ret = mergeJsonFile(&doc, &pnod1->doc, doc.GetAllocator());
                    WARNLOG_IF1(ret, "HOCFGQUERY| msg=merge %s into %s fail", (*vitr).c_str(), file_pattern.c_str());
                }
            }

            if (key_pattern.empty() || "/" == key_pattern) // 返回整个文件
            {
                result = Rjson::ToString(&doc);
                break;
            }

            ret = queryByKeyPattern(result, &doc, file_pattern, key_pattern);
        }
        else // 仅当前文件查找
        {
            AppConfig *pconf = getConfigByName(file_pattern);
            if (NULL == pconf)
            {
                result = "null";
                break;
            }

            if (key_pattern.empty() || "/" == key_pattern) // 返回整个文件
            {
                result = Rjson::ToString(&pconf->doc);
                break;
            }

            ret = queryByKeyPattern(result, &pconf->doc, file_pattern, key_pattern);
        }

    }
    while(0);
    return ret;
}

int HocfgMgr::queryByKeyPattern( string& result, const Value* jdoc, const string& file_pattern, const string& key_pattern )
{
    int ret = 0;

    do
    {
        vector<string> vecPattern;
        ret = StrParse::SpliteStr(vecPattern, key_pattern, '/');
        ERRLOG_IF1BRK(ret || vecPattern.empty(), -46, 
            "HOCFGQUERY| msg=key_pattern invalid| key=%s| filep=%s", 
            key_pattern.c_str(), file_pattern.c_str());

        const Value* pval = jdoc;
        const Value* ptmp = NULL;
        vector<string>::const_iterator vitr = vecPattern.begin();
        for (; vitr != vecPattern.end(); ++vitr)
        {
            const string& token = *vitr;
            if (token.empty() || "/" == token || " " == token) continue;
            ret = Rjson::GetValue(&ptmp, token.c_str(), pval);
            if (ret && StrParse::IsNumberic(token)) // 尝试访问数组
            {
                ret = Rjson::GetValue(&ptmp, atoi(token.c_str()), pval);
            }
            if (ret) // 获取不到时
            {
                LOGWARN("HOCFGQUERY| msg=no key found in json|  key=%s| filep=%s", 
                    key_pattern.c_str(), file_pattern.c_str());
                ret = -47;
                pval = NULL;
                break;
            }

            pval = ptmp;
            ptmp = NULL;
        }

        result = pval? Rjson::ToString(pval): "null";
    }
    while(0);
    return ret;
}
