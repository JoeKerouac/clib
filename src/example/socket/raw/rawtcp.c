//---cat rawtcp.c---
// Run as root or SUID 0, just datagram no data/payload
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

// Packet length
#define PCKT_LEN 8192


/* Structure of a TCP header */
struct tcpheader {

 unsigned short int tcph_srcport;

 unsigned short int tcph_destport;

 unsigned int       tcph_seqnum;

 unsigned int       tcph_acknum;

 unsigned char      tcph_reserved:4, tcph_offset:4;

 // unsigned char tcph_flags;

  unsigned int

       tcp_res1:4,      /*little-endian*/

       tcph_hlen:4,     /*length of tcp header in 32-bit words*/

       tcph_fin:1,      /*Finish flag "fin"*/

       tcph_syn:1,       /*Synchronize sequence numbers to start a connection*/

       tcph_rst:1,      /*Reset flag */

       tcph_psh:1,      /*Push, sends data to the application*/

       tcph_ack:1,      /*acknowledge*/

       tcph_urg:1,      /*urgent pointer*/

       tcph_res2:2;

 unsigned short int tcph_win;

 unsigned short int tcph_chksum;

 unsigned short int tcph_urgptr;

};



// Simple checksum function, may use others such as Cyclic Redundancy Check, CRC

unsigned short csum(unsigned short *buf, int len) {

        unsigned long sum;

        for(sum=0; len>0; len--)

                sum += *buf++;

        sum = (sum >> 16) + (sum &0xffff);

        sum += (sum >> 16);

        return (unsigned short)(~sum);

}

int main(int argc, char *argv[]) {
    int sd;

    // No data, just datagram
    char buffer[PCKT_LEN];
    // The size of the headers
    struct iphdr *ip = (struct iphdr *) buffer;
    struct tcpheader *tcp = (struct tcpheader *) (buffer + sizeof(struct iphdr));


    struct sockaddr_in sin;

    memset(buffer, 0, PCKT_LEN);

    if(argc != 5) {
        printf("- Invalid parameters!!!\n");
        printf("- Usage: %s <source hostname/IP> <source port> <target hostname/IP> <target port>\n", argv[0]);
        exit(-1);
    }


    // IPPROTO_TCP允许自定义header，其中checksum字段和totalLength字段无论我们是否指定内核层都会填充， 而sourceAddress和packetId如果
    // 我们指定了内核就不会填充，否则内核仍将会填充，注意，IPPROTO_RAW协议虽然会默认允许IP_HDRINCL选项，但是这样的话socket就只能发送数据不
    // 能接收数据了;
    // IPPROTO_TCP定义在https://elixir.bootlin.com/linux/v4.7/source/include/uapi/linux/in.h中
    sd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);

    if(sd < 0) {
       perror("socket() error");
       exit(-1);
    }

    printf("socket()-SOCK_RAW and tcp protocol is OK.\n");

    int one = 1;
    const int *val = &one;

    // 指示内核不要填充ip_header，我们自己填充，raw socket文档：https://man7.org/linux/man-pages/man7/raw.7.html
    // 注意，设置了这个选项后底层将不会对数据包进行分片，需要我们自己限制数据包大小不能超过MTU，不然发送失败；
    if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("setsockopt() error");
        exit(-1);
    } else {
       printf("setsockopt() is OK\n");
    }


    // The source is redundant, may be used later if needed
    // Address family
    sin.sin_family = AF_INET;

    // Source port, can be any, modify as needed
    // 对于Linux2.2及以后，这个应该固定设置为0
    sin.sin_port = 0;

    // Source IP, can be any, modify as needed
    sin.sin_addr.s_addr = inet_addr(argv[1]);

    // IP structure
    // header长度，单位4byte
    ip->ihl = 5;
    // 版本号
    ip->version = 4;
    // 服务类型
    ip->tos = 16;
    // 片偏移，分片的时候会用
    ip->frag_off = 0;
    // 数据包生存时间
    ip->ttl = 64;
    // ICMP是1，tcp是6，UDP是17
    // 定义在https://elixir.bootlin.com/linux/v4.7/source/include/uapi/linux/in.h中
    ip->protocol = IPPROTO_TCP;
    // 源ip，可以不填充，不填充的话内核层会填充，这里我们选择填充；
    // inet_addr函数可以将点分十进制ip转换为长证书u_long类型
    ip->saddr = inet_addr(argv[1]);
    // 目标IP
    ip->daddr = inet_addr(argv[3]);

    // The TCP structure. The source port, spoofed, we accept through the command line
    tcp->tcph_srcport = htons(atoi(argv[2]));

    // The destination port, we accept through command line
    tcp->tcph_destport = htons(atoi(argv[4]));
    tcp->tcph_seqnum = htonl(1);
    tcp->tcph_acknum = 0;
    tcp->tcph_offset = 5;
    tcp->tcph_syn = 1;
    tcp->tcph_ack = 0;
    tcp->tcph_win = htons(32767);
    tcp->tcph_chksum = 0; // Done by kernel
    tcp->tcph_urgptr = 0;


    printf("Using:::::Source IP: %s port: %u, Target IP: %s port: %u.\n", argv[1], atoi(argv[2]), argv[3], atoi(argv[4]));


    // sendto() loop, send every 2 second for 50 counts

    unsigned int count;

    unsigned int totalLen;
    totalLen = sizeof(struct iphdr) + sizeof(struct tcphdr) + 0;
    for(count = 0; count < 20; count++) {
        if(sendto(sd, buffer, totalLen, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            perror("sendto() error");
            exit(-1);
        } else {
            printf("Count #%u - sendto() is OK\n", count);
        }
        sleep(2);
    }

    close(sd);

    return 0;

}
