

// 服务间通信使用tcp方式，消息头定义如下
#pragma pack(1)
struct sc_msg_head_t
{
    unsigned char ver;
    unsigned char head_len;
    unsigned int  body_len;
    unsigned short cmdid;
    unsigned short seqid;
};
#pragma pack()

enum command_id_t
{
    CMD_WHOAMI_REQ = 0x0001, // 普通app(cli)启动上报自己身份
    CMD_GETCLI_REQ = 0x0002,
    CMD_HUNGUP_REQ = 0X0003,
    CMD_GETLOGR_REQ= 0X0004,
    CMD_EXCHANG_REQ= 0X0005,
    CMD_SETARGS_REQ= 0X0006,
    CMD_GETWARN_REQ= 0X0007,
    CMD_IAMSERV_REQ = 0x0008, // 分布式中心端启动时报上自身身份
    CMD_KEEPALIVE_REQ = 0x0009,
    CMD_BROADCAST_REQ = 0x000A,
    CMD_CLIERA_REQ = 0x000B,
    CMD_TESTING_REQ = 0x000C,
    CMD_UPDATEERA_REQ = 0x000D,
    CMD_GETCONFIG_REQ = 0x000E, // 查询分布式配置
    CMD_SETCONFIG_REQ = 0x000F,
    CMD_GETCFGNAME_REQ = 0x0010,
    CMD_HOCFGNEW_REQ = 0x0011, // 需要新的配置
    CMD_SETCONFIG2_REQ = 0x0012, // 单播
    CMD_SETCONFIG3_REQ = 0x0013, // 广播
    CMD_BOOKCFGCHANGE_REQ = 0x0014, // 订阅配置改变,有改变时通过CMD_EVNOTIFY_REQ通知
    CMD_EVNOTIFY_REQ = 0x0015, // 通知app事件
    CMD_SVRREGISTER_REQ = 0x0016, // 服务注册
    CMD_SVRSEARCH_REQ = 0x0017, // 服务发现

    CMDID_MID = 0x1000,

    CMD_WHOAMI_RSP = 0x1001,
    CMD_GETCLI_RSP = 0x1002,
    CMD_HUNGUP_RSP = 0X1003,
    CMD_GETLOGR_RSP= 0X1004,
    CMD_EXCHANG_RSP= 0X1005,
    CMD_SETARGS_RSP= 0X1006,
    CMD_GETWARN_RSP= 0X1007,
    CMD_IAMSERV_RSP = 0x1008,
    CMD_KEEPALIVE_RSP = 0x1009,
    CMD_BROADCAST_RSP = 0x100A,
    CMD_CLIERA_RSP = 0x100B,
    CMD_TESTING_RSP = 0x100C,
    CMD_UPDATEERA_RSP = 0x100D,
    CMD_GETCONFIG_RSP = 0x100E,
    CMD_SETCONFIG_RSP = 0x100F,
    CMD_GETCFGNAME_RSP = 0x1010,
    CMD_HOCFGNEW_RSP = 0x1011, 
    CMD_SETCONFIG2_RSP = 0x1012,
    CMD_SETCONFIG3_RSP = 0x1013,
    CMD_BOOKCFGCHANGE_RSP = 0x1014, 
    CMD_EVNOTIFY_RSP = 0x1015,
    CMD_SVRREGISTER_RSP = 0x1016,
    CMD_SVRSEARCH_RSP = 0x1017,
};

static const unsigned char g_msg_ver = 1;
static const unsigned int g_maxpkg_len = (200*1024*1024); // 200M Limit