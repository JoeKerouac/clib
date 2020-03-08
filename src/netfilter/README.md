## 目录说明

netfilter框架学习使用，内核态增加hook点，将数据包由用户空间决策是否发出，然后用户态再读包决策是否发出，是否改包；

## 文件说明
- nf_userspace_queue.h: `nf_userspace_queue.c`的头文件
- nf_userspace_queue.c: 用户空间接受队列，内核态决策为`NF_QUEUE`状态的数据包都会到这里处理；
- nf_kernel_custom_hook.c: 内核空间对数据包；
- test.c: 调用`nf_userspace_queue.c`的示例；
- compile.sh: 编译`test.c`文件使用的脚本；
- Makefile: 编译nf_kernel_custom_hook.c文件需要使用到的Makefile文件，使用`make`命令编译，`make clean`命令清除编译结果；

## 编译说明
- nf_userspace_queue.c：该文件编译依赖于mnl和netfilter_queue两个库，安装方法：
  - mnl:使用`yum search libmnl`来查找相关类库，找到结果中带devel的安装包安装；
  - netfilter_queue:同mnl一样，使用`yum search libnetfilter_queue`来查找相关类库，然后找到结果中带devel的安装包安装；
- nf_kernel_custom_hook.c：如果`/lib/modules/$(shell uname -r)/build/`这个文件找不到，那么手动替换为Linux源码的实际路径，例
如`/usr/src/kernels/3.10.0-1062.9.1.el7.x86_64/`，如果本机没有那么需要安装Linux源码；


## 使用说明
1、test.c文件使用compile.sh编译后生成nf_queue可执行文件，执行该文件的时候需要传入参数，参数为队列编号，因为内核部分没有指定队列
编号，所以队列编号默认是0，传入0即可，命令：`./nf_queue 0`；
2、使用make命令编译nf_kernel_custom_hook.c后，使用`insmod nf_kernel_custom_hook.ko`安装内核（注意，机器重启后内核会自动卸载），不使用的时候可以
用`rmmod nf_kernel_custom_hook`命令来卸载内核（注意这里没有后缀）；
3、注意：一定要先运行用户空间的程序，再安装内核，否则在运行用户空间程序前网络将会断开；

## 一些常量说明
### hook点定义
定义在netfilter_ipv4.h文件中，有以下值：
- NF_IP_PRE_ROUTING:0:After promisc drops, checksum checks
- NF_IP_LOCAL_IN:1:If the packet is destined for this box
- NF_IP_FORWARD:2:If the packet is destined for another interface
- NF_IP_LOCAL_OUT:3:Packets coming from a local process
- NF_IP_POST_ROUTING:4:Packets about to hit the wire
- NF_IP_NUMHOOKS:5:表示最大的hook值，不能用于实际返回，仅用于判断使用，判断hook值是否超出范围；

hook点顺序:
外部数据包->NF_IP_PRE_ROUTING->NF_IP_LOCAL_IN->NF_IP_LOCAL_OUT->NF_IP_POST_ROUTING->数据包发送出去


NF_IP_FORWARD说明：
```
对于目的地不是本机需要转发的数据包，链路上如下：
NF_IP_PRE_ROUTING->NF_IP_FORWARD->NF_IP_POST_ROUTING

注意，只有主机开启了ip_forward才会有该情况发生
```

### 数据包决策返回值说明
- NF_DROP:0:直接删除该包
- NF_ACCEPT:1:接受该包，继续往后处理；
- NF_STOLEN:2:忘记该包，与NF_DROP的区别是NF_DROP会释放sk_buff资源，而NF_STOLEN不会释放sk_buff资源，需要函数自己释放；
- NF_QUEUE:3:将包加入队列，然后等待用户空间决策；怎么处理；
- NF_REPEAT:4:将数据包返回上个节点处理；
- NF_STOP:5:与NF_ACCEPT类似，不同的是后边的HOOK拦截器不会被执行了；
- NF_MAX_VERDICT:5:不是直接用来返回的，而是用来判断返回值是否超出限制的；


## netfilter框架相关
1、netfilter不能抓混杂模式的包，只能接收本机的数据？这个待确定；
2、netfilter内核空间的队列长度有限制，如果内核空间队列满了那么新的数据包将会被丢弃；

## 数据包格式
### ip首部：
4位版本、4位首部长度、8位服务类型、16位总长度、16位标识、3位标志、13位片偏移、8位生存时间（TTL）、8位协议、16位首部校验和、32位源IP
地址、32位目的IP地址、选项（可选，可以有可以没有）、数据
