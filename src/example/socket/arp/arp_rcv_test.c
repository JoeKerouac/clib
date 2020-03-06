#include <stdio.h>
#include "arp_request_lib.h"

static void print_mac(char *mac) {
    char i = 0;
    for(; i < 6; i++){
        unsigned char num = *mac;
        mac++;
        printf("0x%x", num);
        if(i != 5){
            printf(":");
        }
    }
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
            printf("接到的发送方mac是：");
            print_mac(msg.ar_sha);
            printf("\n");
            printf("接到的接受方mac是：");
            print_mac(msg.ar_tha);

            printf("\n\n\n");
        }
    }

    close_arp_socket(fd);

    return 0;
}