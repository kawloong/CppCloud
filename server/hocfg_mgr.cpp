#include "hocfg_mgr.h"
#include "comm/file.h"
#include "cloud/const.h"
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
    ERRLOG_IF1RET_N(File::Isdir(conf_root.c_str()), -40, "CONFIGINIT| msg=path invalid| conf_root=%s", conf_root.c_str());
    m_cfgpath = conf_root;
    File::AdjustPath(m_cfgpath, true, '/');
    return 0;
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

        string item = dirpath + "/" + pfile->d_name;
        
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

// 获取父级继承关系, 多个父级以空格分隔
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
        papp = new AppConfig;
        m_Allconfig[filename] = papp;
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
    Document doc;

    Value::ConstMemberIterator itr = node1->MemberBegin();
    for (; itr != node1->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();
        if (node0->HasMember(key))
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

// 分布式配置查询, file_pattern每个token以-分隔, key_pattern每个token以/分隔

int HocfgMgr::query( string& result, const string& file_pattern, const string& key_pattern )
{

}