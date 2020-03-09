cp -f arp_request_lib.h /usr/include/arp_request_lib.h
gcc -fPIC -shared arp_request_lib.c -o /usr/lib/libarp_request_lib.so