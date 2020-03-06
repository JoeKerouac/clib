#include <stdio.h>
#include "arp_request_lib.h"

static void print_mac(char *mac) {
    char start = 0;
    for(; start < 6; start++){
        unsigned char num = *mac;
        mac++;
        printf("0x%x", num);
        if(start != 5){
            printf(":");
        }
    }
}

static void print_ip(char *ip){
    char start = 0;
    for(; start < 4; start++){
      unsigned char num = *ip;
      printf("%d", num);
      ip++;
      if(start != 3){
        printf(".");
      }
    }
}

static void print_msg(struct arppacket *msg){
    printf("发送方mac是：");
    print_mac(msg->ar_sha);
    printf("\n");

    printf("发送方ip是：");
    print_ip(msg->ar_sip);
    printf("\n");


    printf("接收方mac是：");
    print_mac(msg->ar_tha);
    printf("\n");

    printf("接收方ip是：");
    print_ip(msg->ar_tip);
    printf("\n");
}

int main(){
    unsigned char local_mac[] = { 0x08,0x00,0x27,0xf6,0xf5,0x96 };
    struct arppacket msg;

    int fd = create_arp_socket();

    if(fd < 0) {
        printf("socket打开失败\n");
        return -1;
    }



    while(1){
        if(receive_arp(fd, local_mac, &msg) <= 0){
            printf("rcv error\n");
        } else {
            print_msg(&msg);
            printf("\n\n");
        }
    }

    close_arp_socket(fd);

    return 0;
}