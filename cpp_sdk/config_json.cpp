#include "config_json.h"
#include "comm/strparse.h"

ConfJson::ConfJson( const string& fname )
{
    m_fname = fname;
}

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
    //m_doc.SetObject();
    //m_doc.CopyFrom(*contents, m_doc.GetAllocator()); // 有个问题不用这样用
    m_doc.Parse(Rjson::ToString(contents).c_str());
    
    return 0;
}

const Value* ConfJson::_findNode( const string& qkey ) const
{
    static const char seperator_ch = '/';
    int ret = 0;
    const Value* nodeRet = NULL;


    do
    {
        vector<string> vecPattern;
        string strdoc = Rjson::ToString(&m_doc);

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

int ConfJson::query(int& oval, const string& qkey, bool wideVal) const
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    return _parseVal(oval, node, wideVal);
}

int ConfJson::query( string& oval, const string& qkey, bool wideVal ) const
{
    RWLOCK_READ(m_rwLock);
    const Value* node = _findNode(qkey);
    return _parseVal(oval, node, wideVal);
}

int ConfJson::query( map<string, string>& oval, const string& qkey, bool wideVal ) const
{
    RWLOCK_READ(m_rwLock);
    return queryMAP(oval, qkey, wideVal);
}

int ConfJson::query( map<string, int>& oval, const string& qkey, bool wideVal ) const
{
    RWLOCK_READ(m_rwLock);
    return queryMAP(oval, qkey, wideVal);
}

int ConfJson::query( vector<string>& oval, const string& qkey, bool wideVal ) const
{
    RWLOCK_READ(m_rwLock);
    return queryVector(oval, qkey, wideVal);
}

int ConfJson::query( vector<int>& oval, const string& qkey, bool wideVal ) const
{
    RWLOCK_READ(m_rwLock);
    return queryVector(oval, qkey, wideVal);
}

time_t ConfJson::getMtime( void ) const
{
    return m_mtime;
}

int ConfJson::_parseVal( int& oval, const Value* node, bool wideVal ) const
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

int ConfJson::_parseVal( string& oval, const Value* node, bool wideVal ) const
{
    IFRETURN_N(NULL == node, -1);
    int ret = -2;
    if (node->IsString())
    {
        oval = node->GetString();
        ret = 0;
    }
    else
    {
        if (wideVal) // 可将其他类型转化成string
        {
            if (node->IsNumber()) oval = _N(node->GetInt());
            else if (node->IsFloat()) oval = _F("%f", node->GetFloat());
            else if (node->IsBool()) oval = node->IsTrue()? "true": "false";
            else if (node->IsObject()) oval = "_Object";
            else if (node->IsArray()) oval = "_Array";
            else oval = "";
            
            ret = 0;
        }
    }

    return ret;
}

template<class ValT>
int ConfJson::queryMAP( map<string, ValT>& oval, const string& qkey, bool wideVal ) const
{
    const Value* node = _findNode(qkey);
    IFRETURN_N(NULL == node || !node->IsObject(), -2);

    Value::ConstMemberIterator itr = node->MemberBegin();
    for (; itr != node->MemberEnd(); ++itr)
    {
        ValT oval2;
        if (0 == _parseVal(oval2, &itr->value, wideVal))
        {
            const char* key = itr->name.GetString();
            oval[key] = oval2;
        }
    }

    return 0;
}

template<class ValT>
int ConfJson::queryVector( vector<ValT>& oval, const string& qkey, bool wideVal ) const
{
    const Value* node = _findNode(qkey);
    IFRETURN_N(NULL == node || !node->IsArray(), -2);

    Value::ConstValueIterator itr = node->Begin();
    for (; itr != node->End(); ++itr)
    {
        ValT oval2;
        if (0 == _parseVal(oval2, &(*itr), wideVal))
        {
            oval.push_back(oval2);
        }
    }

    return 0;
}
