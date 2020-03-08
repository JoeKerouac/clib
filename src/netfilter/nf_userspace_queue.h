#ifndef NF_UQ_HD
#define NF_UQ_HD


// 回调数据结构体
struct callback_data;

/**
 * @brief 注册回调函数
 * @param callback 函数指针
 */
void nfuq_register(void (*callback)(struct callback_data *)) ;

/**
 * @brief 发送决策，回调函数接收到数据包处理后需要调用该函数发送响应
 * @param queue_num 队列号
 * @param id id
 * @param plen 数据长度
 * @param sendData ip数据报文，要符合ip报文规范
 */
void nfuq_send_verdict(int queue_num, unsigned int id, unsigned short plen, void *sendData);

/**
 * @brief 开始启动接受内核消息
 * @param queue_num 队列号
 * @return 返回0表示正常退出，返回其他表示异常退出
 */
int nfuq_run(unsigned int queue_num);


void nfuq_set_data(struct callback_data *callback_data, char *data);
void nfuq_set_data_len(struct callback_data *data, unsigned short data_len);
void nfuq_set_queue_num(struct callback_data *data, int queue_num);
void nfuq_set_hook_num(struct callback_data *data, int hook_num);
void nfuq_set_id(struct callback_data *data, unsigned int id);

char* nfuq_read_data(struct callback_data *data);
unsigned short nfuq_read_data_len(struct callback_data *data);
int nfuq_read_queue_num(struct callback_data *data);
int nfuq_read_hook_num(struct callback_data *data);
unsigned int nfuq_read_id(struct callback_data *data);

#endif