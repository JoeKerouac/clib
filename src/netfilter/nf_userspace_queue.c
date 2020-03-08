#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <libmnl/libmnl.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nfnetlink.h>

#include <linux/types.h>
#include <linux/netfilter/nfnetlink_queue.h>

#include <libnetfilter_queue/libnetfilter_queue.h>

/* only for NFQA_CT, not needed otherwise: */
#include <linux/netfilter/nfnetlink_conntrack.h>

static struct mnl_socket *nl;

// 回调数据
struct callback_data{
    // 回调数据
    char *data;
    // 数据长度
    unsigned short data_len;
    // 队列号
    int queue_num;
    // 内核hook点
    int hook_num;
    // 数据包id
    unsigned int id;
};


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

// 定义回调函数类型
static void (*_callback)(struct callback_data *);

/**
 * @brief 构建nlmsghdr并放入指定内存
 * @param buf 指定数据内存指针
 * @param type 消息类型
 * @param queue_num 队列号
 * @return 构建的nlmsghdr指针，该指针指向的数据内存区域就是在buf的内存区域
 */
static struct nlmsghdr * nfuq_hdr_put(char *buf, int type, uint32_t queue_num);

/**
 * @brief 收到内核消息的回调，当内核收到报文然后放入队列后会发出消息，回调到这里
 * @param nlh 消息头
 * @param data 回传数据，mnl_cb_run函数传进来的指针，可以用于返回数据等，这里传的是null，不需要回传数据
 * @return 返回大于等于1表示成功，小于等于-1表示失败，0表示要停止回调
 */
static int queue_cb(const struct nlmsghdr *nlh, void *data);

/**
 * @brief 注册回调函数
 * @param callback 函数指针
 */
void nfuq_register(void (*callback)(struct callback_data *)) {
    _callback = callback;
}

// 启动
int nfuq_run(unsigned int queue_num)
{
    char *buf;
    /* largest possible packet payload, plus netlink data overhead: */
    size_t sizeof_buf = 0xffff + (MNL_SOCKET_BUFFER_SIZE/2);
    struct nlmsghdr *nlh;
    int ret;
    unsigned int portid;

    nl = mnl_socket_open(NETLINK_NETFILTER);

    // socket打开失败
    if (nl == NULL) {
        return -1;
    }

    // 绑定失败
    if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
        return -2;
    }

    // portid
    portid = mnl_socket_get_portid(nl);

    buf = malloc(sizeof_buf);
    // 内存申请失败
    if (!buf) {
        return -3;
    }

    nlh = nfuq_hdr_put(buf, NFQNL_MSG_CONFIG, queue_num);
    nfq_nlmsg_cfg_put_cmd(nlh, AF_INET, NFQNL_CFG_CMD_BIND);

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        return -4;
    }

    nlh = nfuq_hdr_put(buf, NFQNL_MSG_CONFIG, queue_num);
    nfq_nlmsg_cfg_put_params(nlh, NFQNL_COPY_PACKET, 0xffff);

    mnl_attr_put_u32(nlh, NFQA_CFG_FLAGS, htonl(NFQA_CFG_F_GSO));
    mnl_attr_put_u32(nlh, NFQA_CFG_MASK, htonl(NFQA_CFG_F_GSO));

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        return -5;
    }

    /* ENOBUFS is signalled to userspace when packets were lost
     * on kernel side.  In most cases, userspace isn't interested
     * in this information, so turn it off.
     */
    ret = 1;
    mnl_socket_setsockopt(nl, NETLINK_NO_ENOBUFS, &ret, sizeof(int));

    for (;;) {
        ret = mnl_socket_recvfrom(nl, buf, sizeof_buf);
        if (ret == -1) {
            return -6;
        }

        // 被终止
        ret = mnl_cb_run(buf, ret, 0, portid, queue_cb, NULL);
        if (ret < 0){
            return -7;
        }
    }

    mnl_socket_close(nl);

    return 0;
}

/**
 * @brief 构建nlmsghdr并放入指定内存
 * @param buf 指定数据内存指针
 * @param type 消息类型
 * @param queue_num 队列号
 * @return 构建的nlmsghdr指针，该指针指向的数据内存区域就是在buf的内存区域
 */
static struct nlmsghdr *
nfuq_hdr_put(char *buf, int type, uint32_t queue_num)
{
    struct nlmsghdr *nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type    = (NFNL_SUBSYS_QUEUE << 8) | type;
    nlh->nlmsg_flags = NLM_F_REQUEST;

    struct nfgenmsg *nfg = mnl_nlmsg_put_extra_header(nlh, sizeof(*nfg));
    nfg->nfgen_family = AF_UNSPEC;
    nfg->version = NFNETLINK_V0;
    nfg->res_id = htons(queue_num);

    return nlh;
}

/**
 * @brief 发送决策
 * @param queue_num 队列号
 * @param id id
 * @param plen 数据长度
 * @param sendData ip数据报文，要符合ip报文规范
 */
void nfuq_send_verdict(int queue_num, unsigned int id, unsigned short plen, void *sendData) {
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh;
    struct nlattr *nest, *data;

    nlh = nfuq_hdr_put(buf, NFQNL_MSG_VERDICT, queue_num);
    nfq_nlmsg_verdict_put(nlh, id, NF_ACCEPT);

    /* example to set the connmark. First, start NFQA_CT section: */
    nest = mnl_attr_nest_start(nlh, NFQA_CT);
    /* then, add the connmark attribute: */
    mnl_attr_put_u32(nlh, CTA_MARK, htonl(42));
    /* more conntrack attributes, e.g. CTA_LABELS could be set here */
    /* end conntrack section */
    mnl_attr_nest_end(nlh, nest);

    // 放入数据
    mnl_attr_put(nlh, NFQA_PAYLOAD, plen, sendData);

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        perror("mnl_socket_send");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 收到内核消息的回调，当内核收到报文然后放入队列后会发出消息，回调到这里
 * @param nlh 消息头
 * @param data 回传数据，mnl_cb_run函数传进来的指针，可以用于返回数据等，这里传的是null，不需要回传数据
 * @return 返回大于等于1表示成功，小于等于-1表示失败，0表示要停止回调
 */
static int queue_cb(const struct nlmsghdr *nlh, void *data)
{
    struct nfqnl_msg_packet_hdr *ph = NULL;
    struct nlattr *attr[NFQA_MAX+1] = {};
    uint32_t id = 0;
    struct nfgenmsg *nfg;
    uint16_t plen;
    int queue_num, hook;

    // 解析参数列表
    if (nfq_nlmsg_parse(nlh, attr) < 0) {
        perror("problems parsing");
        return MNL_CB_ERROR;
    }

    // 获取nfgenmsg
    nfg = mnl_nlmsg_get_payload(nlh);
    // 队列号
    queue_num = ntohs(nfg->res_id);

    if (attr[NFQA_PACKET_HDR] == NULL) {
        fputs("metaheader not set\n", stderr);
        return MNL_CB_ERROR;
    }

    ph = mnl_attr_get_payload(attr[NFQA_PACKET_HDR]);
    // 对应的内核hook点，有可能内核多个hook点都加入到了该队列，这时可以用这个区分
    hook = ph->hook;

    // ip报文长度
    plen = mnl_attr_get_payload_len(attr[NFQA_PAYLOAD]);
    // 标准的ip报文
    unsigned char *payload = mnl_attr_get_payload(attr[NFQA_PAYLOAD]);

    id = ntohl(ph->packet_id);

    if (_callback){
        // 注册的有回调函数，准备回调数据
        struct callback_data cd = {
            .data = payload,
            .data_len = plen,
            .queue_num = queue_num,
            .hook_num = hook,
            .id = id
        };
        // 回调
        _callback(&cd);
    } else {
        // 没有回调函数直接发送通过
        nfuq_send_verdict(queue_num, id, plen, payload);
    }

    return MNL_CB_OK;
}

void nfuq_set_data(struct callback_data *callback_data, char *data){
    callback_data->data = data;
}

void nfuq_set_data_len(struct callback_data *data, unsigned short data_len){
    data->data_len = data_len;
}

void nfuq_set_queue_num(struct callback_data *data, int queue_num){
    data->queue_num = queue_num;
}

void nfuq_set_hook_num(struct callback_data *data, int hook_num){
    data->hook_num = hook_num;
}

void nfuq_set_id(struct callback_data *data, unsigned int id){
    data->id = id;
}

char* nfuq_read_data(struct callback_data *data){
    return data->data;
}

unsigned short nfuq_read_data_len(struct callback_data *data){
    return data->data_len;
}

int nfuq_read_queue_num(struct callback_data *data){
    return data->queue_num;
}

int nfuq_read_hook_num(struct callback_data *data){
    return data->hook_num;
}

unsigned int nfuq_read_id(struct callback_data *data){
    return data->id;
}
