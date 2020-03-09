#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <string.h>
#include <net/if.h>

#define AR_TYPE         0x0806 // ARP帧类型
#define ETHER_TYPE      0x01 // 以太网类型
#define PROTOCOL        0x0800 // 上层协议类型，IP协议
#define OP_REQUEST      0x01 // ARP请求包操作码
#define OP_RESPONSE     0x02 // ARP响应包操作码
#define IP_ADD_LEN      4 // ip地址长度
#define BROADCAST_MAC   { 0xff,0xff,0xff,0xff,0xff,0xff } // 广播mac地址

// 默认广播mac地址
const char broadcast_mac[] =  BROADCAST_MAC;

// arp请求以太网数据
struct arppacket
{
        unsigned char dest_mac[ETH_ALEN];//接收方MAC
        unsigned char src_mac[ETH_ALEN];//发送方MAC
        unsigned short type;         //0x0806是ARP帧的类型值
        unsigned short ar_hrd;//硬件类型 - 以太网类型值0x1
        unsigned short ar_pro;//上层协议类型 - IP协议(0x0800)
        unsigned char  ar_hln;//MAC地址长度
        unsigned char  ar_pln;//IP地址长度
        unsigned short ar_op;//操作码 - 0x1表示ARP请求包,0x2表示应答包
        unsigned char  ar_sha[ETH_ALEN];//发送方mac
        unsigned char ar_sip[IP_ADD_LEN];//发送方ip
        unsigned char ar_tha[ETH_ALEN];//接收方mac
        unsigned char ar_tip[IP_ADD_LEN];//接收方ip
} __attribute__ ((__packed__));

/**
 * @brief 发送指定arp请求，当接收方mac为空或者为{ 0xff,0xff,0xff,0xff,0xff,0xff }时将会认为是发送arp查询请求
 * @param socket_fd create_arp_socket打开的socket
 * @param src_mac 发送方mac，例如{ 0xff,0xff,0xff,0xff,0xff,0xff }，必须是长度为6的数组
 * @param src_ip 发送方ip，例如 "192.168.1.1"
 * @param dest_mac 接收方mac，例如{ 0xff,0xff,0xff,0xff,0xff,0xff }，必须是长度为6的数组，可以传null或者{ 0xff,0xff,0xff,0xff,0xff,0xff }，此时认为是发送arp查询请求；
 * @param dest_ip 接收方ip，例如 "192.168.1.1"
 * @return 返回小于0表示失败
 */
int send_arp(int socket_fd, char src_mac[], char src_ip[], char dest_mac[], char dest_ip[]) {
    if(socket_fd < 0){
        return -1;
    }

    // 定义发送方、接收方地址
    struct in_addr s,r;
    // 接收方地址
    struct sockaddr_ll sl;

    // 构建应答包
    struct arppacket arp={
                .dest_mac = BROADCAST_MAC,
                .src_mac = {0},
                .type = htons(AR_TYPE),
                .ar_hrd = htons(ETHER_TYPE),
                .ar_pro = htons(PROTOCOL),
                .ar_hln = ETH_ALEN,
                .ar_pln = IP_ADD_LEN,
                .ar_op = htons(OP_REQUEST),
                .ar_sha = {0},
                .ar_sip = {0},
                .ar_tha = BROADCAST_MAC,
                .ar_tip = {0}
    };

    // 发送方mac赋值
    memcpy(&arp.src_mac, src_mac, ETH_ALEN);
    memcpy(&arp.ar_sha, src_mac, ETH_ALEN);

    // 如果指定了接收方mac，那么使用指定的mac
    if (dest_mac) {
        memcpy(&arp.src_mac, dest_mac, ETH_ALEN);
        memcpy(&arp.ar_tha, dest_mac, ETH_ALEN);
    }

    // 设置发送方ip
    inet_aton(src_ip, &s);
    memcpy(&arp.ar_sip, &s, sizeof(s));

    // 设置接收方ip
    inet_aton(dest_ip, &r);
    memcpy(&arp.ar_tip, &r, sizeof(r));

    // 接收方地址内存初始化
    memset(&sl, 0, sizeof(sl));
    // 接收方地址信息
    sl.sll_family = AF_PACKET;
    sl.sll_halen = ETHER_ADDR_LEN;
    sl.sll_protocol = htons(ETH_P_ARP);
    sl.sll_ifindex = IFF_BROADCAST;

    // 如果用户指定了接收方的mac地址，那么使用用户指定的mac地址，如果用户未指定，那么使用默认的地址
    if (dest_mac) {
        memcpy(sl.sll_addr, dest_mac, ETHER_ADDR_LEN);
    } else {
        memcpy(sl.sll_addr, broadcast_mac, ETHER_ADDR_LEN);
    }

    if(sendto(socket_fd, &arp, sizeof(arp), 0, (struct sockaddr*)&sl, sizeof(sl)) <= 0) {
        return -2;
    } else {
        return 0;
    }
}

/**
 * @brief 接受一个发往本地的arp报文
 * @param socket_fd create_arp_socket打开的socket
 * @param msg arp报文，用于接受arp数据，当返回大于0的时候该地址会被填充数据
 * @return 返回小于等于0表示失败
 */
int receive_arp(int socket_fd, struct arppacket *msg) {
    struct sockaddr_ll sl;
    int ret;
    // 地址长度
    socklen_t addr_length = sizeof(struct sockaddr_ll);


    // 地址初始化
    sl.sll_family = AF_PACKET;
    sl.sll_halen = ETHER_ADDR_LEN;
    sl.sll_protocol = htons(ETH_P_ARP);
    sl.sll_ifindex = IFF_BROADCAST;

    memset(msg, 0, sizeof(struct arppacket));
    // 接收数据
    ret = recvfrom(socket_fd, msg, sizeof(struct arppacket), 0, (struct sockaddr *)&sl, &addr_length);

    return ret;
}

/**
 * @brief 打开arp发送socket
 * @return socket，小于0表示失败
 */
int create_arp_socket() {
    return socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ARP));
}

/**
 * @brief 关闭create_arp_socket打开的socket
 * @param socket_fd create_arp_socket打开的socket
 */
void close_arp_socket(int socket_fd) {
    close(socket_fd);
}

char * arl_get_dest_mac(struct arppacket *data){
    return data->ar_tha;
}

char * arl_get_dest_ip(struct arppacket *data){
    return data->ar_tip;
}

char * arl_get_src_mac(struct arppacket *data){
    return data->ar_sha;
}

char * arl_get_src_ip(struct arppacket *data){
    return data->ar_sip;
}