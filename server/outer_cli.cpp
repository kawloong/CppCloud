#include "outer_cli.h"


HEPMUTICLASS_IMPL(OuterCli, OuterCli, CliBase)

OuterCli::OuterCli( void ): m_inServ(0)
{
    m_isLocal = false;
}

void OuterCli::init( int inServ )
{
    m_inServ = inServ;
}