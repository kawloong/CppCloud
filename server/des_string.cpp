#include "crypt.h"
#include <cstdio>

struct ParamConf
{
    string str_input; // 输入数据
    string str_key;
    bool onlyb64;
    bool encrpt; // 0加密; 1解密;
}g_param; // 保存命令行相关参数

// 获取命令行参数
static int parse_cmdline(int argc, char** argv)
{
    int ret = 0;
    int c;
    g_param.onlyb64 = true;
    g_param.encrpt = true;


    while ((c = getopt(argc, argv, "c:d:k:b")) != -1)
    {
        switch (c)
        {
        case 'c' :
            g_param.str_input = optarg;
            g_param.encrpt = true;
            break;
        case 'd' :
            g_param.str_input = optarg;
            g_param.encrpt = false;
            break;
        case 'k':
            g_param.str_key = optarg;
            g_param.onlyb64 = false;
            break;
        case 'b': // only base64
            g_param.onlyb64 = true;
            g_param.str_key = "";
            break;

        case 'h':
        default :
            ret = 1;
            break;
        }
    }

    return ret;
}

static int tip_help(int r)
{
    printf("command parameter use: \n"
        "\t-b no use des encrypt or decrypt\n"
        "\t-c <xxxxxx> encrypt xxxxxx to des+base64\n"
        "\t-d <xxxxxx> decrypt xxxxxx to origin string\n"
        "\t-k <key> passord set\n");
    return r;
}

int main(int argc, char** argv)
{
    int ret;
    ret = parse_cmdline(argc, argv);
    if (ret) return tip_help(1);
    if (g_param.str_input.empty()) return tip_help(2);

    const char* begstr = "";
    const char* keystr = "";
    Crypt* cpt = NULL;
    if (!g_param.str_key.empty())
    {
        cpt = new Crypt(g_param.str_key);
        keystr = g_param.str_key.c_str();
    }
    else
    {
        cpt = new Crypt(g_param.onlyb64);
        keystr = g_param.onlyb64? "only-base64": "def-key";
    }

    string strout;
    if (g_param.encrpt) // 加密模式
    {
        ret = cpt->encode(strout, g_param.str_input);
        begstr = "encrypt";
    }
    else
    {
        ret = cpt->decode(strout, g_param.str_input);
        begstr = "decrypt";
    }

    printf("%s [%s] result:\n%s\n", begstr, keystr, strout.c_str());

    delete cpt;
    return 0;
}