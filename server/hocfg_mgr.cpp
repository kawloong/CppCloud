#include "hocfg_mgr.h"
#include "cloud/homacro.h"
#include "cloud/exception.h"
#include "iohand.h"
#include "broadcastcli.h"
#include "climanage.h"
#include "comm/hep_base.h"
#include "comm/file.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include "route_exchange.h"
#include "rapidjson/filewritestream.h"
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <cerrno>
#include <cstdio>


HEPCLASS_IMPL_FUNCX_BEG(HocfgMgr)
HEPCLASS_IMPL_FUNCX_MORE(HocfgMgr, OnSetConfigHandle)
HEPCLASS_IMPL_FUNCX_MORE(HocfgMgr, OnGetAllCfgName)
HEPCLASS_IMPL_FUNCX_MORE(HocfgMgr, OnCMD_HOCFGNEW_REQ)
HEPCLASS_IMPL_FUNCX_MORE(HocfgMgr, OnCMD_BOOKCFGCHANGE_REQ)
HEPCLASS_IMPL_FUNCX_END(HocfgMgr)

HocfgMgr* HocfgMgr::This = NULL;

HocfgMgr::HocfgMgr( void ): m_seqid(0)
{
    This = this;
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
bool HocfgMgr::getBaseConfigName( string& baseCfg, const string& curCfgName ) const
{
    int ret = -1;
    map<string,AppConfig*>::const_iterator itr = m_Allconfig.find(HOCFG_METAFILE);
    if (m_Allconfig.end() != itr)
    {
        AppConfig* papp = itr->second;
        ret = Rjson::GetStr(baseCfg, HOCFG_META_CONF_KEY, curCfgName.c_str(), &papp->doc);
    }

    return (0 == ret && !baseCfg.empty());
}

// 解析json文档进内存map
int HocfgMgr::parseConffile( const string& filename, const string& contents, time_t mtime )
{
    Document fdoc;

    ERRLOG_IF1RET_N(fdoc.Parse(contents.c_str()).GetParseError(), -44, "CONFIGLOADS| msg=json read fail| file=%s", filename.c_str());
    AppConfig *papp = NULL;
    const size_t pathlen = m_cfgpath.length();
    size_t pos1 = filename.find(m_cfgpath);
    //size_t pos2 = filename.find(".json");
    string key = (0 == pos1 /* && pos2 > pathlen*/) ? filename.substr(pathlen /*, pos2-pathlen*/) : filename;
    
    map<string, AppConfig *>::iterator itr = m_Allconfig.find(key);
    if (m_Allconfig.end() == itr)
    {
        papp = new AppConfig;
        
        m_Allconfig[key] = papp;
        papp->mtime = mtime;
        papp->doc.Parse(contents.c_str());
    }
    else
    {
        papp = itr->second;
        if (papp->mtime <= mtime)
        {
            papp->doc.SetObject();
            papp->doc.Parse(contents.c_str());
            papp->mtime = mtime;
            papp->isDel = false;
        }
    }

    return 0;
}

// json合并, 将node1的内存合并进node0
int HocfgMgr::mergeJsonFile( Value* node0, const Value* node1, MemoryPoolAllocator<>& allc ) const
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

AppConfig* HocfgMgr::getConfigByName( const string& curCfgName ) const
{
    map<string, AppConfig*>::const_iterator itr = m_Allconfig.find(curCfgName);
    AppConfig* papp = (m_Allconfig.end() == itr)? NULL: itr->second;
    return papp;
}

/**
 * @return: 返回文件时间截，如果是继承的(incBase=1)，则是最大文件的时间截；无文件返回0
 **/
int HocfgMgr::getCfgMtime( const string& file_pattern, bool incBase ) const
{
    int ret = 0;
    string baseStr;
 
    AppConfig *pconf = getConfigByName(file_pattern);
    IFRETURN_N(NULL == pconf, 0);
    ret = pconf->mtime;
    if (incBase && getBaseConfigName(baseStr, file_pattern))
    {
        vector<string> vecBase;
        ret = StrParse::SpliteStr(vecBase, baseStr, ' ');
        ERRLOG_IF1(ret, "HOCFGQUERY| msg=invalid basestr setting| "
                        "baseStr=%s| filep=%s", baseStr.c_str(), file_pattern.c_str());
        vector<string>::const_iterator vitr = vecBase.begin();
        for (; vitr != vecBase.end(); ++vitr)
        {
            AppConfig *pnod1 = getConfigByName(*vitr);
            if (pnod1 && pnod1->mtime > ret)
            {
                ret = pnod1->mtime;
            }
        }
    }

    return ret;
}

// 分布式配置查询, file_pattern每个token以-分隔, key_pattern每个token以/分隔
// 返回的字符串可能是 {object} [array] "string" integer null
int HocfgMgr::query( string& result, const string& file_pattern, const string& key_pattern, bool incBase ) const
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

int HocfgMgr::queryByKeyPattern( string& result, const Value* jdoc, const string& file_pattern, const string& key_pattern ) const
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

/** 
 * request: by CMD_SETCONFIG_REQ CMD_SETCONFIG2_REQ CMD_SETCONFIG3_REQ
 * remart: 如果没有contents,则是删除
 * format: { filename: "", mtime: 123456, contents: {..} }
 **/
int HocfgMgr::OnSetConfigHandle( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false)
    RJSON_GETSTR_D(callby, &doc); // cfg_newer:来自某一Serv请求某文件; null:来自web-cli到达的修改; setall:广播设备所有
    RJSON_GETSTR_D(filename, &doc);
    RJSON_GETINT_D(mtime, &doc);
    const Value* contents = NULL;
    Rjson::GetValue(&contents, "contents", &doc);
    NormalExceptionOn_IFTRUE(filename.empty(), 400, cmdid, seqid, "leak of filename param");

    int ret = 0;
    string desc;
    if (0 == mtime) mtime = time(NULL);
    

    if (NULL == contents || contents->IsNull()) // 删除操作
    {
        This->remove(filename, mtime); // xx清除内存xx, 同时unlink磁盘文件
        desc = _F("remove %s success", filename.c_str());
    }
    else
    {
        string fullfilename = This->m_cfgpath + filename; // 文件名要加上本地路径来存储
        ret = This->parseConffile(fullfilename, Rjson::ToString(contents), mtime);
        desc = (0==ret)? "success": "fail";
        if (0 == ret)
        {
            ret = This->save2File(fullfilename, contents);
        }
    }

    This->notifyChange(filename, mtime);
    if (CMD_SETCONFIG_REQ == cmdid)
    {
        string resp = _F("{\"code\": %d, \"desc\": \"%s\"}", ret, desc.c_str());
        iohand->sendData(CMD_SETCONFIG_RSP, seqid, resp.c_str(), resp.length(), true);

        //int fromcli = iohand->getIntProperty(CONNTERID_KEY);
        //ret = Rjson::SetObjMember(BROARDCAST_KEY_FROM, fromcli, &doc);
        ret = Rjson::SetObjMember("callby", "setall", &doc);
        ERRLOG_IF1(ret, "HOCFGSET| msg=set callby fail| cmdid=0x%X| mi=%s", cmdid, iohand->m_idProfile.c_str());
        ret = BroadCastCli::Instance()->toWorld(doc, CMD_SETCONFIG3_REQ, seqid, false);
        LOGINFO("HOCFGSET| msg=modify hocfg by app(%s)| filename=%s| ret=%d", iohand->m_idProfile.c_str(), filename.c_str(), ret);
    }
    else
    {
        string from;
        RJSON_VGETSTR(from, BROARDCAST_KEY_FROM, &doc);
        LOGINFO("HOCFGSET| msg=modify hocfg by Serv(%s) %s| filename=%s| ret=%d", 
            from.c_str(), callby.c_str(), filename.c_str(), ret);
    }

    return ret;
}

// 当某配置发生变化时，通知到已订阅的客户端
void HocfgMgr::notifyChange( const string& filename, int mtime ) const
{
    string aliasPrefix = _F("%s_%s@", BOOK_HOCFG_ALIAS_PREFIX, filename.c_str());
    CliMgr::AliasCursor alcr(aliasPrefix); 
    CliBase *cli = NULL;
    vector<CliBase*> vecCli;

    string msg("{");
    StrParse::PutOneJson(msg, "notify", "cfg_change", true);
    StrParse::PutOneJson(msg, "filename", filename, true);
    StrParse::PutOneJson(msg, "mtime", mtime, false);
    msg += "}";

    while ((cli = alcr.pop()))
    {
        IOHand* iohand = dynamic_cast<IOHand*>(cli);
        if (NULL == iohand)
        {
            LOGWARN("NOTIFYCHANGE| msg=book from ocli| cli=%s| file=%s",
                       cli->m_idProfile.c_str(), filename.c_str());
            continue;
        }
        iohand->sendData(CMD_EVNOTIFY_REQ, ++m_seqid, msg.c_str(), msg.size(), true);
    }
}

/**
 * remart: 外部查询所有配置的名字
 * return format: { file1: [1,mtime1], file2: [0,mtime2] }
 **/
int HocfgMgr::OnGetAllCfgName( void* ptr, unsigned cmdid, void* param )
{
    IOHand* iohand = (IOHand*)ptr;
    IOBuffItem* iBufItem = (IOBuffItem*)param; 
    unsigned seqid = iBufItem->head()->seqid;
    
    string resp = This->getAllCfgNameJson();
    iohand->sendData(CMD_GETCFGNAME_RSP, seqid, resp.c_str(), resp.length(), true);
    return 0;
}

// param: 0仅返回删除的; 1仅返回存在的; 2全返回(default)
string HocfgMgr::getAllCfgNameJson( int filter_flag /*=2*/ ) const
{
    string jresult("{");

    map<string, AppConfig*>::const_iterator itr = m_Allconfig.begin();
    for (int i=0; itr != m_Allconfig.end(); ++itr)
    {
        AppConfig* pcfg = itr->second;
        int flag0 = pcfg->isDel ? 0 : 1;

        if (2 == filter_flag || flag0 == filter_flag)
        {
            if (i > 0) jresult += ",";
            StrParse::AppendFormat(jresult, "\"%s\":[%d,%d]", itr->first.c_str(), flag0, pcfg->mtime);
            ++i;
        }
    }

    jresult += "}";
    return jresult;
}

void HocfgMgr::remove( const string& cfgname, time_t mtime )
{
    map<string,AppConfig*>::iterator it = m_Allconfig.find(cfgname);
    if (it != m_Allconfig.end())
    {
        if (0 == mtime)
        {
            IFDELETE(it->second);
            m_Allconfig.erase(it);
            string local_file = m_cfgpath + cfgname;
            unlink(local_file.c_str());
        }
        else if (mtime >= it->second->mtime) // web上请求删除时,不移除内存,只作unlink,因为如删内存了,同步多机有问题
        {
            string local_file = m_cfgpath + cfgname;
            unlink(local_file.c_str());
            it->second->mtime = mtime;
            it->second->isDel = true;
            it->second->doc.SetObject();
        }
    }
}

int HocfgMgr::save2File( const string& filename, const Value* doc ) const
{
    FILE* fp = NULL;
    char buf[64];
    fp = fopen(filename.c_str(), "w");
    ERRLOG_IF1RET_N(fp<0, -49, "HOCFGSAVE| msg=fopen fail %d| filename=%s", errno, filename.c_str());
    FileWriteStream osm(fp, buf, sizeof(buf));
    PrettyWriter<FileWriteStream> writer(osm);
    bool bret = doc->Accept(writer);
    fclose(fp);
    ERRLOG_IF1RET_N(!bret, -48, "HOCFGSAVE| msg=doc accept fail | filename=%s", filename.c_str());

    return 0;
}

// 对比来自其他节点上的配置, 若本地过旧,则请求更新
// 触发自CMD_BROADCAST_REQ
int HocfgMgr::compareServHoCfg( int fromSvrid, const Value* jdoc )
{
    ERRLOG_IF1RET_N(!jdoc->IsObject(), -50, "HOCFGCMP| msg=cfgera isnot jobject| ");

    int ret = 0;
    string reqmsg("{\"data\":[");
    int count = 0;
    Value::ConstMemberIterator itr = jdoc->MemberBegin();
    for (; itr != jdoc->MemberEnd(); ++itr)
    {
        const char* key = itr->name.GetString();
        ERRLOG_IF0BRK(itr->value.IsArray() && 2 == itr->value.Size(), -51, 
            "HOCFGCMP| msg=item not array| fromSvrid=%d| jdoc=%s", fromSvrid, Rjson::ToString(jdoc).c_str());
        int existFlag = 0;
        int omtime = 0;
        Rjson::GetInt(existFlag, 0, &itr->value);
        Rjson::GetInt(omtime, 1, &itr->value);

        AppConfig* appcfg = getConfigByName(key);
        if (NULL == appcfg || appcfg->mtime < omtime)
        {
            if (0 == existFlag) // 要删的
            {
                remove(key, omtime);
            }
            else
            {
                if (count > 0) reqmsg += ",";
                reqmsg += string("\"") + key + "\"";
                ++count;
            }
        }
    }
    reqmsg += "]}";
    ret = count>0 ? RouteExchage::PostToCli(reqmsg, CMD_HOCFGNEW_REQ, ++m_seqid, fromSvrid) : 0;
    
    return ret;
}

int HocfgMgr::OnCMD_HOCFGNEW_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false)
    int from = 0;
    const Value* arrdoc = NULL;

    int ret = Rjson::GetArray(&arrdoc, "data", &doc);
    Rjson::GetInt(from, ROUTE_MSG_KEY_FROM, &doc);
    ERRLOG_IF1RET_N(ret, -51, "HOCFGNEWREQ| msg=json[data] invalid| from=%d", from);

    int size = arrdoc->Size();
    int okcount = 0;
    string fs;
    for (int i=0; i < size; ++i)
    {
        string fname;
        if (0 == Rjson::GetStr(fname, i, arrdoc))
        {
            AppConfig* pcfg = This->getConfigByName(fname);
            if (NULL == pcfg) continue;

            // 响应对应于 HocfgMgr::OnSetConfigHandle 消费
            string msgrsp("{");
            StrParse::PutOneJson(msgrsp, "callby", "cfg_newer", true);
            StrParse::PutOneJson(msgrsp, "filename", fname, true);
            StrParse::PutOneJson(msgrsp, "mtime", pcfg->mtime, true);
            msgrsp += string("\"contents\":") + Rjson::ToString(&pcfg->doc);
            msgrsp += "}";

            fs += fname + " ";
            ret = RouteExchage::PostToCli(msgrsp, CMD_SETCONFIG2_REQ, seqid, from);
            ERRLOG_IF1(ret, "HOCFGNEWREQ| msg=post cfgout fail %d| filename=%s", ret, fname.c_str());
            ++okcount;
        }
    }
    
    RouteExchage::PostToCli(
        _F("{\"desc\": \"send %d cfg( %s) out\", \"code\":0}", okcount, fs.c_str()), 
        CMD_HOCFGNEW_RSP, seqid, from);
    return ret;
}

// 请求订阅配置改变通知
//  { "file_pattern": "xxx", "incbase": 1}
int HocfgMgr::OnCMD_BOOKCFGCHANGE_REQ( void* ptr, unsigned cmdid, void* param )
{
    MSGHANDLE_PARSEHEAD(false)
    RJSON_VGETINT_D(incbase, HOCFG_INCLUDEBASE_KEY, &doc);
    RJSON_VGETSTR_D(file_pattern, HOCFG_FILENAME_KEY, &doc);

    NormalExceptionOn_IFTRUE(file_pattern.empty()||NULL==This->getConfigByName(file_pattern), 
            420, CMD_BOOKCFGCHANGE_RSP, seqid, string("no file=" + file_pattern));
        
    string baseStr;
    int ret = 0;
    vector<string> vecBase;

    if (1 == incbase && This->getBaseConfigName(baseStr, file_pattern))
    {
        ret = StrParse::SpliteStr(vecBase, baseStr, ' ');
    }
    
    string linkFiles;
    vecBase.push_back(file_pattern);
    vector<string>::const_iterator vitr = vecBase.begin();
    for (; vitr != vecBase.end(); ++vitr)
    {
        const string& vitem = *vitr;
        static const string filename_special_char("/-._"); // 文件名可以包含的特殊字符
        if (StrParse::IsCharacter(vitem, filename_special_char, true))
        {
            string aliasName = _F("%s_%s@%s", BOOK_HOCFG_ALIAS_PREFIX, 
                vitem.c_str(), iohand->getProperty(CONNTERID_KEY).c_str());
            ret = CliMgr::Instance()->addAlias2Child(aliasName, iohand);
            ERRLOG_IF1(ret, "HOCFGBOOK| msg=add alias fail| aliasName=%s| iohand=%p", aliasName.c_str(), iohand);
            if (0 == ret)
            {
                linkFiles += vitem + " ";
            }
        }
    }

    string resp = 0 == ret? _F("{\"code\": 0, \"desc\": \"monitor %s success\"}", linkFiles.c_str()) :
        _F("{\"code\": 400, \"desc\": \"fail %d\"", ret);
    iohand->sendData(CMD_BOOKCFGCHANGE_RSP, seqid, resp.c_str(), resp.length(), true);
    return ret;
}

/**
 * @summery: 通过后端配置，完善客户应用属性和定制属性
 */
int HocfgMgr::setupPropByServConfig( IOHand* iohand ) const
{
    int ret = 0;

    map<string,AppConfig*>::const_iterator itr = m_Allconfig.find(HOCFG_METAFILE);
    IFRETURN_N(m_Allconfig.end() == itr, 0);
    const Value* customnode = NULL;
    Rjson::GetObject(&customnode, HOCFG_META_HOST_CUSTUM_KEY, &itr->second->doc);
    IFRETURN_N(NULL == customnode, 0);
    

    const Value* tagnode = NULL; // cli指定tag属性，优先级最高
    const Value* hostnode = NULL; // cli所在host，优先级其次
    const Value* defnode = NULL; // 缺省配置，优先级最低
    string cliip = iohand->getProperty("_ip");
    string tag = iohand->getProperty("tag");
    string svrnam = iohand->getProperty(SVRNAME_KEY);
    
    Rjson::GetObject(&defnode, "default", customnode);
    Rjson::GetObject(&hostnode, cliip.c_str(), customnode);
    Rjson::GetObject(&tagnode, tag.c_str(), customnode);

    int ntmp = 0;
    string strtmp;
    bool bexist = true;
#define SET_PROP_BY_PRIORITY(Type, cfgkey, val, propKey)              \
    bexist = true;                                                    \
    if (0 != Rjson::Get##Type(val, cfgkey, tagnode)) {                \
        if (0 != Rjson::Get##Type(val, cfgkey, hostnode)) {           \
            if (0 != Rjson::Get##Type(val, cfgkey, defnode)) {        \
                bexist = false;                                       \
            } } }                                                     \
        if (bexist) { iohand->setProperty(propKey, val); }
    
    SET_PROP_BY_PRIORITY(Int, "_idc", ntmp, "idc");
    SET_PROP_BY_PRIORITY(Int, "_rack", ntmp, "rack");
    SET_PROP_BY_PRIORITY(Str, svrnam.c_str(), strtmp, HOCFG_CLI_MAINCONF_KEY);

    return ret;
}


