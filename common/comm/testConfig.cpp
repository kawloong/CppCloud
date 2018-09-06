#include "log.h"
#include "cppcloud_config.h"
#include  <stdio.h>

using namespace CloudConf;

int main(int argc, char* argv[])
{
	std::string strt;
	Config conf;
	int ret = conf.load("a.ini");
	printf("load ret=%d\n", ret);
	ret = conf.read("comm", "serv", strt);
	printf("read serv=%s ret=%d\n", strt.c_str(), ret);
	
	ret = conf.read("comm3", "port", strt);
	printf("read com3 ret=%d\n", ret);

	//////////////////////////////////////
	ret = log_open("filet.log");
	printf("open logfile = %d\n", ret);
	log_debug("debug message %d", ret);
	log_warn("warn message %d", ret);

	ret = CloudConf::Init("./b.ini");
	strt = GETLOGPATH();
	printf("logpath=%s\n", strt.c_str());

	ret = TESTINI();
	printf("testini=%d\n", ret);

	return 0;
}

	
	
