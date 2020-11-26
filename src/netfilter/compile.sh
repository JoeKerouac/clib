# 安装nf_userspace_queue库到本机
cp nf_userspace_queue.h /usr/include/nf_userspace_queue.h
gcc -fPIC -shared nf_userspace_queue.c -lmnl -lnetfilter_queue -o /usr/lib/libnf_userspace_queue.so
# 编译本目录下的测试类
gcc -c nf_userspace_queue.c
gcc test.c nf_userspace_queue.o -lmnl -lnetfilter_queue -o main
