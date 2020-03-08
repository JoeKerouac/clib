gcc -c nf_userspace_queue.c
gcc test.c nf_userspace_queue.o -l mnl -l netfilter_queue -o main
