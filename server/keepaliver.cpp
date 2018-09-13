#include "keepalive.h"
#include "cloud/const.h"

void KeepAliver::init( void )
{
    SwitchHand::Instance()->appendQTask(this, CLI_KEEPALIVE_TIME_SEC*1000);
}

int KeepAliver::qrun( int flag, long p2 )
{
    CliBase* cli = NULL;
    long now = time(NULL);
    long atime_kaliv = now - CLI_KEEPALIVE_TIME_SEC;
    long atime_dead = now - CLI_DEAD_TIME_SEC;

    // 最小过期时间键, 小于此值的cli要进行发送keepalive包
    string expire_kaliv_key = StrParse::Format("%s%ld", CLI_PREFIX_KEY_TIMEOUT, atime_kaliv);
    string expire_dead_key = StrParse::Format("%s%ld", CLI_PREFIX_KEY_TIMEOUT, atime_dead);

    CliMgr::AliasCursor alcur(CLI_PREFIX_KEY_TIMEOUT);
    while ( (cli = alcur.pop()) )
    {
        if (expire_dead_key.compare(alcur.retKey) > 0) // 需清理
        {

        }
        else if (expire_kaliv_key.compare(expire_kaliv_key) > 0) // 需keepalive
        {

        }
        else
        {
            break;
        }
    }

    if (HEFG_PEXIT != flag)
    {
        SwitchHand::Instance()->appendQTask(this, CLI_KEEPALIVE_TIME_SEC*1000);
    }
    
    return 0;
}