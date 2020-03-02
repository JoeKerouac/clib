#include <stdio.h>
#include "arp_request_lib.h"

int main(){
    unsigned char src_mac[] = { 0x08,0x00,0x27,0xf6,0xf5,0x96 };
    unsigned char dest_mac[] = { 0x08,0x00,0x27,0xfc,0x07,0x9c };
    char src_ip[] = "192.168.199.1";
    char dest_ip[] = "192.168.199.110";

    int fd = create_arp_socket();

    if(fd < 0) {
        printf("socket打开失败\n");
        return -1;
    }

    while(1){
        if(send_arp(fd, src_mac, src_ip, dest_mac, dest_ip) < 0){
            printf("send error\n");
        } else {
            printf("send success\n");
        }
        sleep(1);
    }

    close_arp_socket(fd);

    return 0;
}