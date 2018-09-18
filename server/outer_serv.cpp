#include "outer_serv.h"


HEPMUTICLASS_IMPL(OuterServ, OuterServ, CliBase)

OuterServ::OuterServ()
{
    m_cliType = 1;
    m_isLocal = false;
}