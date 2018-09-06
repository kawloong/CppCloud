#include <cstdio>
#include "public.h"

int main(int , char**)
{
//	log_open("myf.log", Logger::LEVEL_DEBUG, false, 200);
	LOGDEBUG("print debug message ... ");
	LOGINFO("print infomation message ..");
	LOGERROR("printf error test");


	return 0;
}


