#include "keepaliver.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include "iohand.h"
#include "switchhand.h"
#include "climanage.h"


KeepAliver::KeepAliver()
{

}

void KeepAliver::init( void )
{
    m_seqid = 0;
    SwitchHand::Instance()->appendQTask(this, CLI_KEEPALIVE_TIME_SEC*1000);
}

int KeepAliver::run(int p1, long p2)
{
    return -1;
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
        if (expire_dead_key.compare(alcur.iter_range.retKey) > 0) // 需清理
        {
            closeCli(cli);
        }
        else if (expire_kaliv_key.compare(alcur.iter_range.retKey) > 0) // 需keepalive
        {
            sendReq(cli);
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

int KeepAliver::sendReq( CliBase* cli )
{
    ERRLOG_IF1RET_N(NULL==cli||!cli->isLocal(), 33, "KEEPALIVEREQ| msg=invalid cli %p", cli);
    IOHand* ioh = dynamic_cast<IOHand*>(cli);
    ERRLOG_IF1RET_N(NULL==ioh, 34, "KEEPALIVEREQ| msg=cli isnot IOHand %p| mi=%s", cli, cli->m_idProfile.c_str());
    
    int ret;
    ret = ioh->sendData(CMD_KEEPALIVE_REQ, ++m_seqid, "", 0, true);
    LOGOPT_EI(ret, "KEEPALIVEREQ| msg=send msg result %d| mi=%s", ret, cli->m_idProfile.c_str());
    return ret;
}

void KeepAliver::closeCli( CliBase* cli )
{
    ERRLOG_IF1RET(NULL==cli||!cli->isLocal(), "KEEPALIVEREQ| msg=invalid cli %p", cli);
    IOHand* ioh = dynamic_cast<IOHand*>(cli);
    string mi = ioh->m_idProfile;
    int atime = ioh->getIntProperty("atime");

    int ret = ioh->driveClose();
    LOGINFO("KEEPALIVECLOSE| msg=close zombie connect| ret=%d| cli=%s| dt=%ds", ret, mi.c_str(), (int)time(NULL)-atime);
}