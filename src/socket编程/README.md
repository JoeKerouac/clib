## 文件说明
- arp_request_lib.c:发送查询arp请求的库代码；
- arp_request_lib.h:发送查询arp请求的库的头文件；
- test.c:测试类，使用`arp_request_lib.c`中的函数

## 相关命令
编译：
```
# 编译arp_request_lib.c:
gcc -c arp_request_lib.c

# 编译test.c，其中arp_request_lib.o是上边编译结果
gcc test.c arp_request_lib.o 
```


清空arp缓存:
```
ip ne flush all
```

查询arp缓存：

```
arp -a
```