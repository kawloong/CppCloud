#include "broadcastcli.h"
#include "cloud/const.h"
#include "comm/strparse.h"
#include "iohand.h"
#include "switchhand.h"
#include "climanage.h"

HEPCLASS_IMPL_FUNC(BroadCastCli, OnBroadCMD)

BroadCastCli::BroadCastCli()
{

}

void BroadCastCli::init( void )
{
    m_seqid = 0;
    SwitchHand::Instance()->appendQTask(this, BROADCASTCLI_INTERVAL_SEC*1000);
}

int BroadCastCli::run(int p1, long p2)
{
    return -1;
}

int BroadCastCli::qrun( int flag, long p2 )
{
 
    
    return 0;
}

