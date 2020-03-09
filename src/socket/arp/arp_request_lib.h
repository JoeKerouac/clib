#ifndef ARP_REQUEST_LIB
#define ARP_REQUEST_LIB

// 默认广播mac地址
extern char broadcast_mac[];

struct arppacket;

/**
 * @brief 发送指定arp请求，当接收方mac为空或者为{ 0xff,0xff,0xff,0xff,0xff,0xff }时将会认为是发送arp查询请求
 * @param socket_fd create_arp_socket打开的socket
 * @param src_mac 发送方mac，例如{ 0xff,0xff,0xff,0xff,0xff,0xff }，必须是长度为6的数组
 * @param src_ip 发送方ip，例如 "192.168.1.1"
 * @param dest_mac 接收方mac，例如{ 0xff,0xff,0xff,0xff,0xff,0xff }，必须是长度为6的数组，可以传null或者{ 0xff,0xff,0xff,0xff,0xff,0xff }，此时认为是发送arp查询请求；
 * @param dest_ip 接收方ip，例如 "192.168.1.1"
 * @return 返回小于0表示失败
 */
int send_arp(int socket_fd, char src_mac[], char src_ip[], char dest_mac[], char dest_ip[]);

/**
 * @brief 接受一个发往本地的arp报文
 * @param socket_fd create_arp_socket打开的socket
 * @param msg arp报文，用于接受arp数据，当返回大于0的时候该地址会被填充数据
 * @return 返回小于等于0表示失败
 */
int receive_arp(int socket_fd, struct arppacket *msg);

/**
 * @brief 打开arp发送socket
 * @return socket，小于0表示失败
 */
int create_arp_socket();

/**
 * @brief 关闭create_arp_socket打开的socket
 * @param socket_fd create_arp_socket打开的socket
 */
void close_arp_socket(int socket_fd);

char * arl_get_dest_mac(struct arppacket *data);

char * arl_get_dest_ip(struct arppacket *data);

char * arl_get_src_mac(struct arppacket *data);

char * arl_get_src_ip(struct arppacket *data);

#endif