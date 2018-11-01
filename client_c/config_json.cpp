#include "config_json.h"
#include "comm/strparse.h"

int ConfJson::update( const Value* data )
{
    ERRLOG_IF1RET_N(NULL==data || !data->IsObject(), -100,
            "CONFUPDATE| msg=data invalid| data=%s", Rjson::ToString(data).c_str());
    
    RJSON_GETINT_D(mtime, data);
    const Value* contents = NULL;
    Rjson::GetValue(&contents, "contents", data);
    ERRLOG_IF1RET_N(NULL==contents || 0 == mtime, -105,
            "CONFUPDATE| msg=data invalid| data=%s", Rjson::ToString(data).c_str());
    
    m_mtime = mtime;
    RWLOCK_WRITE(m_rwLock);
    m_doc.SetObject();
    m_doc.CopyFrom(*contents, m_doc.GetAllocator());
    return 0;
}

const Value* ConfJson::_findNode( const string& qkey )
{
    static const char seperator_ch = '/';
    int ret = 0;
    const Value* nodeRet = NULL;

    do
    {
        vector<string> vecPattern;
        ret = StrParse::SpliteStr(vecPattern, qkey, seperator_ch);
        ERRLOG_IF1BRK(ret || vecPattern.empty(), -101, 
            "CONFJSON| msg=qkey invalid| key=%s| filep=%s", 
            qkey.c_str(), m_fname.c_str());

        const Value* pval = &m_doc;
        const Value* ptmp = NULL;
        vector<string>::const_iterator vitr = vecPattern.begin();
        for (; vitr != vecPattern.end(); ++vitr)
        {
            const string& token = *vitr;
            if (token.empty() || "/" == token || " " == token) continue;
            ret = Rjson::GetValue(&ptmp, token.c_str(), pval);
            if (ret && StrParse::IsNumberic(token)) // Object不存在时尝试访问数组
            {
                ret = Rjson::GetValue(&ptmp, atoi(token.c_str()), pval);
            }
            if (ret) // 获取不到时
            {
                LOGWARN("HOCFGQUERY| msg=no key found in json|  key=%s| filep=%s", 
                    qkey.c_str(), m_fname.c_str());
                ret = -102;
                pval = NULL;
                break;
            }

            pval = ptmp;
            ptmp = NULL;
        }

        if (0 == ret)
        {
            nodeRet = pval;
        }
    }
    while(0);

    return nodeRet;
}

int ConfJson::parseVal( int& oval, const Value* node )
{
    IFRETURN_N(NULL == node, -1);
    int ret = -2;

    if (node->IsInt())
    {
        oval = node->GetInt();
        ret = 0;
    }
    else if (node->IsString())
    {
        string chval = node->GetString();
        if (StrParse::IsNumberic(chval))
        {
            oval = atoi(chval.c_str());
            ret = 0;
        }
    }

    return ret;
}

int ConfJson::parseVal( string& oval, const Value* node )
{
    IFRETURN_N(NULL == node, -1);
    int ret = -2;
    if (node->IsString())
    {
        oval = node->GetString();
        ret = 0;
    }

    return ret;
}

int ConfJson::query(int& oval, const string& qkey)
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    return parseVal(oval, node);
}

int ConfJson::query(string& oval, const string& qkey)
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    return parseVal(oval, node);
}

template<class ValT>
int ConfJson::queryMAP( map<string, ValT>& oval, const string& qkey )
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    IFRETURN_N(NULL == node || !node->IsObject(), -2);

    Value::ConstMemberIterator itr = node->MemberBegin();
    for (; itr != node->MemberEnd(); ++itr)
    {
        ValT oval2;
        if (0 == parseVal(oval2, &itr->value))
        {
            const char* key = itr->name.GetString();
            oval[key] = oval2;
        }
    }

    return 0;
}

int ConfJson::query( map<string, string>& oval, const string& qkey )
{
    return queryMAP(oval, qkey);
}

int ConfJson::query( map<string, int>& oval, const string& qkey )
{
    return queryMAP(oval, qkey);
}

template<class ValT>
int ConfJson::queryVector( vector<ValT>& oval, const string& qkey )
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    IFRETURN_N(NULL == node || !node->IsArray(), -2);

    Value::ConstValueIterator itr = node->Begin();
    for (; itr != node->End(); ++itr)
    {
        ValT oval2;
        if (0 == parseVal(oval2, &(*itr)))
        {
            oval.push_back(oval2);
        }
    }

    return 0;
}

int ConfJson::query( vector<string>& oval, const string& qkey )
{
    return queryVector(oval, qkey);
}

int ConfJson::query( vector<int>& oval, const string& qkey )
{
    return queryVector(oval, qkey);
}

time_t ConfJson::getMtime( void )
{
    return m_mtime;
}