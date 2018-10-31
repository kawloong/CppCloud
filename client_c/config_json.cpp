#include "config_json.h"

int ConfJson::update( const Value* data )
{
    ERRLOG_IF1RET_N(NULL==data || !data->IsObject(), -100,
            "CONFUPDATE| msg=data invalid| data=%s", Rjson::ToString(data));
    
    m_doc.SetObject();
    m_doc.CopyFrom(*data, m_doc.GetAllocator());
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
            oval = _N(chval);
            ret = 0;
        }
    }

    return ret;
}

int ConfJson::parseVal( string& oval, const Value* node )
{
    
}

int ConfJson::query(int& oval, const string& qkey)
{
    const Value* node = _findNode(qkey);
    IFRETURN_N(NULL == node, -2);
}

int ConfJson::query(string& oval, const string& qkey)
{
}