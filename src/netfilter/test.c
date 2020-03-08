#include "nf_userspace_queue.h"
#include <stdio.h>

void test_callback(struct callback_data *data){
    printf("收到数据包了，不处理直接发送\n");
    nfuq_send_verdict(nfuq_read_queue_num(data), nfuq_read_id(data), nfuq_read_data_len(data), nfuq_read_data(data));
}


int main(int argc, char *argv[])
{
    /* largest possible packet payload, plus netlink data overhead: */
    unsigned int queue_num;

    if (argc != 2) {
        printf("Usage: %s [queue_num]\n", argv[0]);
        return 1;
    }
    queue_num = atoi(argv[1]);
    printf("queue_num : %d \n", queue_num);

    nfuq_register(&test_callback);
    nfuq_run(queue_num);
}
