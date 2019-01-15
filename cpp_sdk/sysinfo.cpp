
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
// mostly need to read the linux config files to get system info
 
// ---- get os info ---- //
void getOsInfo()
{
    FILE *fp = fopen("/proc/version", "r");
    if(NULL == fp)
        printf("failed to open version\n");
    char szTest[1000] = {0};
    while(!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n
        printf("%s", szTest);
    }
    fclose(fp);
}
 
// ---- get cpu info ---- //
void getCpuInfo()
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(NULL == fp)
        printf("failed to open cpuinfo\n");
    char szTest[1000] = {0};
    // read file line by line
    while(!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp); // leave out \n
        printf("%s", szTest);
    }
    fclose(fp);
}
 
 
// ---- get memory info ---- //
void getMemoryInfo()
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if(NULL == fp)
        printf("failed to open meminfo\n");
    char szTest[1000] = {0};
    while(!feof(fp))
    {
        memset(szTest, 0, sizeof(szTest));
        fgets(szTest, sizeof(szTest) - 1, fp);
        printf("%s", szTest);
    }
    fclose(fp);
}
 
// ---- get harddisk info ---- //
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
 
void getHardDiskInfo()
{
    // use cmd, this is the only way
    static struct hd_driveid hd;
    int fd;
 
    if ((fd = open("/dev/sda1", O_RDONLY | O_NONBLOCK)) < 0)
    {
        printf("ERROR opening /dev/sda\n");
        return;
    }
 
    if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
    {
        printf("model ", hd.model);
        printf("HardDisk Serial Number: %.20s\n", hd.serial_no);
    }
    else
        printf("no available harddisk info");
}
 
// ---- get network info ---- //
#include <ifaddrs.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <linux/if.h>
void getNetworkInfo()
{
    // get ip, works for linux and mac os, best method
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
 
    getifaddrs(&ifAddrStruct);
 
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
 
        // check if IP4
        if (ifa->ifa_addr->sa_family == AF_INET)
        {
            void *tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char local_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, local_ip, INET_ADDRSTRLEN);
 
            // actually only need external ip
            printf("%s IP: %s\n", ifa->ifa_name, local_ip);
        }
    }
 
    if (ifAddrStruct)
        freeifaddrs(ifAddrStruct);
 
 
    // get mac, need to create socket first, may not work for mac os
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
 
    char local_mac[128] = { 0 };
 
    strcpy(ifr.ifr_name, "eth0"); // only need ethernet card
    if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
    {
        char temp_str[10] = { 0 };
        memcpy(temp_str, ifr.ifr_hwaddr.sa_data, 6);
        sprintf(local_mac, "%02x-%02x-%02x-%02x-%02x-%02x", temp_str[0]&0xff, temp_str[1]&0xff, temp_str[2]&0xff, temp_str[3]&0xff, temp_str[4]&0xff, temp_str[5]&0xff);
    }
 
    printf("Local Mac: %s\n", local_mac);
 
}
 
int main(int argc, char **argv)
{
    printf("=== os information ===\n");
    getOsInfo();
 
    printf("=== cpu infomation ===\n");
    getCpuInfo();
 
    printf("=== memory information ===\n");
    getMemoryInfo();
 
    printf("=== harddisk information ===\n");
    getHardDiskInfo();
 
    printf("=== network information ===\n");
    getNetworkInfo();
 
    return 0;
}