#include "svrprop.h"
#include "comm/strparse.h"

SvrProp::SvrProp( void ): svrid(0), prvdid(0), okcount(0), ngcount(0), tmpnum(0),
	 protocol(0),version(0), weight(0), idc(0), rack(0), islocal(false), enable(false)
{

}

bool SvrProp::valid( void ) const
{
	return enable && protocol > 0 && svrid > 0 && weight > 0 && !url.empty();
}

string SvrProp::jsonStr( void ) const
{
	string strjson;
	strjson.append("{");
	StrParse::PutOneJson(strjson, "regname", regname, true);
	StrParse::PutOneJson(strjson, "url", url, true);
	if(!desc.empty()>0) StrParse::PutOneJson(strjson, "desc", desc, true);
	if(svrid>0) StrParse::PutOneJson(strjson, "svrid", svrid, true);
	if(okcount>0) StrParse::PutOneJson(strjson, "okcount", okcount, true);
	if(ngcount>0) StrParse::PutOneJson(strjson, "ngcount", ngcount, true);
	StrParse::PutOneJson(strjson, "protocol", protocol, true);
	StrParse::PutOneJson(strjson, "version", version, true);
	// 权重，当服务消费应用调用时，weight=匹配分数值
	if(weight>0) StrParse::PutOneJson(strjson, "weight", weight, true);
	if(idc>0) StrParse::PutOneJson(strjson, "idc", idc, true);
	if(rack>0) StrParse::PutOneJson(strjson, "rack", rack, true);
 	StrParse::PutOneJson(strjson, "enable", enable, false);
	strjson.append("}");

	return strjson;
}
