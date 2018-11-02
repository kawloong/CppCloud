#include "svrconsumer.h"
#include "svrconsumer.h"


SvrConsumer::SvrConsumer( void )
{
    This = this;
}

SvrConsumer::~SvrConsumer( void )
{
    
}

int SvrConsumer::OnCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{
    return This->onCMD_SVRSEARCH_RSP(ptr, cmdid, param);
}

int SvrConsumer::OnCMD_EVNOTIFY_REQ( void* ptr ) // provider 下线通知
{
    return onCMD_EVNOTIFY_REQ(ptr);
}

int SvrConsumer::onCMD_SVRSEARCH_RSP( void* ptr, unsigned cmdid, void* param )
{

}
int SvrConsumer::onCMD_EVNOTIFY_REQ( void* ptr )
{
    
}

SvrConsumer::uninit( void )
{
    map<string, SvrItem*>::iterator it = m_allPrvds.begin();
    for (; it != m_allPrvds.end(); ++it)
    {
        delete it->second;
    }

    m_allPrvds.clear();
}

