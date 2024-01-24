#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#define SERVER_PORT 8888
#define MAX_LISTEN  128
#define BUFSIZE     128 

/* 单线程/进程 实现并发 */
int main()
{
    /* 创建套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error");
        exit(-1);
    }
    /* 将本地的IP和端口绑定 */
    struct sockaddr_in localAddress; //本地ip
    bzero((void *)&localAddress, sizeof(localAddress)); //清空脏数据
    localAddress.sin_family = AF_INET; //地址族
    localAddress.sin_port = htons(SERVER_PORT); //端口号转大端
    localAddress.sin_addr.s_addr =htonl(INADDR_ANY); //可以接受任何地址
    /* 绑定连接 */
    socklen_t localAddressLen = sizeof(localAddress);
    int ret = bind(sockfd, (struct sockaddr *)&localAddress, localAddressLen);
    if (ret == -1)
    {
        perror("bind error");
        exit(-1);
    }
    /* 监听 */
    ret = listen(sockfd, MAX_LISTEN);
    if (ret == -1)
    {
        perror("listen error");
    }

    /* 开始工作：用while和accept 需要使用到多线程/线程池 */
    /* 这里使用单线程实现 */

    fd_set readSet;
    /* 清空集合 */
    FD_ZERO(&readSet);
    /* 把监听的文件描述符sockfd添加到读函数中，让内核帮忙检测 */
    FD_SET(sockfd, &readSet);

    int maxfd = sockfd;
    fd_set tmpReadSet;
    bzero(&tmpReadSet, sizeof(tmpReadSet));
    while (1)
    {
        /* 备份读集合 */
        tmpReadSet = readSet;
        /* 检测 */
        ret = select(maxfd + 1, &tmpReadSet, NULL, NULL, NULL);
        if (ret == -1)
        {
            perror("select error");
            break;//崩了跳出去回收文件
        }
        /* 如果sockfd在readset集合中，就一定有人来跟服务器通信了 */
        /* readSet需要持续迎宾 */
        if (FD_ISSET(sockfd, &tmpReadSet))
        {
            /* 接收 */
            int acceptfd = accept(sockfd, NULL, NULL);
            if (acceptfd == -1)
            {
                perror("acceptfd error");
                break;//退出循环，回收句柄
            }
            /* 将通信的句柄放到读集合中 */
            FD_SET(acceptfd, &readSet);
            /* 更新最大值：有客户端可能会退出 */
            maxfd = maxfd < acceptfd ? acceptfd : maxfd;
        }
        
        /* 程序到这个地方说明可能有通信 */
        for (int  idx = 0; idx <= maxfd; idx++)
        {
            /* 如果不是本服务器的套接字并且还在readSet集合中，那一定有通信 */
            if (idx != sockfd && FD_ISSET(idx, &tmpReadSet))
            {
                char buf[BUFSIZE];
                bzero(buf, sizeof(buf));
                /* 接收客户端发过来的通信数据 */
                int readBytes = read(idx, buf, sizeof(buf) - 1);
                if (readBytes < 0)
                {
                    perror("readBytes error");
                    /* 将该通信句柄从监听的读集合中删除 */
                    FD_CLR(idx, &readSet);
                    close(idx);//出问题就关闭通信句柄，接着执行下一个readSet中的通信句柄(下一个已ready的fd句柄)
                    continue;
                }
                else if (readBytes == 0)
                {
                    printf("客户端断开连接....\n");
                    /* 将该通信句柄从监听的读集合中删除 */
                    FD_CLR(idx, &readSet);
                    close(idx);//执行下一个已ready的fd句柄
                    continue;
                }
                else
                {  
                    /* 打印数据 */
                    printf("recv:%s\n", buf);
                    for (int jdx = 0; jdx <= readBytes; jdx++)
                    {
                        buf[idx] = toupper(buf[jdx]);
                    }

                    /* 发回客户端 */
                    write(idx, buf, readBytes);
                    sleep(5);
                }
                
            }
        }
    }
    
    /* 关闭文件描述符 */
    close(sockfd);


    return 0;

}