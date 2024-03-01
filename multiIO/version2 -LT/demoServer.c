#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <error.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <fcntl.h>

#define SERVER_PORT 8080
#define MAX_LISTEN  128
#define LOCAL_IPADDRESS "127.0.0.1"
#define BUFFER_SIZE 128

#define READ_BUFFER_SIZE 5


int main()
{
    /* 创建socket套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket error");
        exit(-1);
    }

    /* 设置端口复用 */
    int enableOpt = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&enableOpt, sizeof(enableOpt));
    if (ret == -1)
    {
        perror("setsockopt error");
        exit(-1);
    }

    /* 绑定 */
#if 0
    /* 这个结构体不好用 */
    struct sockaddr localAddress;
#else
    struct sockaddr_in localAddress;
#endif
    /* 清除脏数据 */
    memset(&localAddress, 0, sizeof(localAddress));

    /* 地址族 */
    localAddress.sin_family = AF_INET;
    /* 端口需要转成大端 */
    localAddress.sin_port = htons(SERVER_PORT);
    /* ip地址需要转成大端 */

    /* Address to accept any incoming messages.  */
    /* INADDR_ANY = 0x00000000 */
    localAddress.sin_addr.s_addr = htonl(INADDR_ANY); 

    
    int localAddressLen = sizeof(localAddress);
    ret = bind(sockfd, (struct sockaddr *)&localAddress, localAddressLen);
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
        exit(-1);
    }   

    /* 创建epoll 红黑树 实例 */
    int epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll create error");
        exit(-1);
    }

    /* 将sockfd 添加到红黑树实例里面 */
    struct epoll_event event;
    /* 清除脏数据 */
    memset(&event, 0, sizeof(event));
    event.data.fd = sockfd;
    event.events = EPOLLIN;     // 读事件
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
    if (ret == -1)
    {
        perror("epoll ctl error");
        exit(-1);
    }

    /* 监听的数量 */
    int nums = 0;

    /* 读到字节数 */
    int readBytes = 0;
    while (1)
    {
        struct epoll_event events[BUFFER_SIZE];
        /* 清除脏数据 */
        memset(events, 0, sizeof(events));

        nums = epoll_wait(epfd, events, BUFFER_SIZE, -1);
        if (nums == -1)
        {
            perror("epoll wait error");
            exit(-1);
        }  

        printf("nums = %d\n", nums);


        /* 
            程序执行到这个地方有两种情况:
                1. 超时
                2. 有监听数据来了
        */
        for (int idx = 0; idx < nums; idx++)
        {
            int fd = events[idx].data.fd;
            if (fd == sockfd)
            {
                /* 有连接 */
                int connfd = accept(sockfd, NULL, NULL);
                if (connfd == -1)
                {
                    perror("perror error");
                    exit(-1);
                }

                /* 将通信句柄fd 设置成非阻塞模式 */
                int flag = fcntl(connfd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(connfd, F_SETFL, flag);


                struct epoll_event conn_event;
                memset(&conn_event, 0, sizeof(conn_event));
                conn_event.data.fd = connfd;
                conn_event.events = EPOLLIN;

                /* 将通信的句柄 添加到树结点中 */
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &conn_event);
                if (ret == -1)
                {
                    perror("epoll_ctl error");
                    continue;
                }
            }
            else
            {
                char buffer[READ_BUFFER_SIZE] = { 0 };
                /* 有数据通信 */
                readBytes = read(fd, buffer, sizeof(buffer) - 1);
                if (readBytes < 0)
                {
                    perror("read error");
                    /* 将该文件句柄 从红黑树上 删除掉 */
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    /* 关闭文件句柄 */
                    close(fd);
                }
                else if (readBytes == 0)
                {
                    printf("客户端下线......\n");

                    /* 将该文件句柄 从红黑树上 删除掉 */
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    /* 关闭文件句柄 */
                    close(fd);
                }
                else
                {
                    printf("recv:%s\n", buffer);

                    for (int jdx = 0; jdx < readBytes; jdx++)
                    {
                        buffer[jdx] = toupper(buffer[jdx]);
                    }

                    /* 发回客户端 */
                    write(fd, buffer, readBytes);

                    usleep(300);
                }
            }
        }
    }

    /* 关闭文件描述符 */
    close(sockfd);

    return 0;
}