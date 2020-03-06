#define __KERNEL__
#define MODULE

#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter.h>


static struct nf_hook_ops nfho;

// 实际的hook函数
unsigned int hook_func(unsigned int hooknum,struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
//    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
//    struct udphdr *udp_header;
//    struct tcphdr *tcp_header;
//    struct list_head *p;
//    unsigned int src_ip = (unsigned int)ip_header->saddr;
//    unsigned int dest_ip = (unsigned int)ip_header->daddr;
//    unsigned int src_port = 0;
//    unsigned int dest_port = 0;
//
//    if (ip_header->protocol==17) {
//      udp_header = (struct udphdr *)skb_transport_header(skb);
//      src_port = (unsigned int)ntohs(udp_header->source);
//    } else if (ip_header->protocol == 6) {
//      tcp_header = (struct tcphdr *)skb_transport_header(skb);
//      src_port = (unsigned int)ntohs(tcp_header->source);
//      dest_port = (unsigned int)ntohs(tcp_header->dest);
//    }
//
//    /* 这里日志可以不用打印，不然可能会影响性能 */
//    printk(KERN_INFO "OUT packet info: src ip: %u, src port: %u; dest ip: %u, dest port: %u; proto:\n", src_ip, src_port, dest_ip, dest_port);
//    printk(KERN_DEBUG "IP addres = %pI4  DEST = %pI4\n", &src_ip, &dest_ip);
    // 所有数据包直接队列缓存起来
    return NF_QUEUE;
}

int init(void){
    printk(KERN_INFO "init_module nf_kernel_custom_hook\n");
    nfho.hook = hook_func;
    /* 该常量定义在netfilter_ipv4.h文件中，2表示NF_IP_FORWARD，详见README */
    nfho.hooknum = 2;
    nfho.pf = PF_INET;
    nfho.priority = NF_IP_PRI_FIRST;
    nf_register_hook(&nfho);
    return 0;
}

void cleanup(void){
    printk(KERN_INFO "cleanup_module nf_kernel_custom_hook\n");
    nf_unregister_hook(&nfho);
}

module_init(init);
module_exit(cleanup);
