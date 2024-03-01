#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/epoll.h>
#include <ctype.h>


#define SERVER_PORT 8686
#define MAX_LISTEN 128
#define LOCAL_IPADDRESS " 127.0.0.1"
#define BUF_SIZE 128


int main()
{
    /* 创建套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socker error");
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
    struct sockaddr_in localAddress;
    memset(&localAddress, 0, sizeof(localAddress));
    /* 地址族 */
    localAddress.sin_family = AF_INET;
    /* 端口需要转成大端 */
    localAddress.sin_port = htons(SERVER_PORT);
    #if 1
    /* ip地址需要转成大端 */
    /* 可以接受任何来源信息 */
    localAddress.sin_addr.s_addr =  htonl(INADDR_ANY);
    #else
    inet_pton(AF_INET, "", &(localAddress.sin_addr.s_addr));
    #endif

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
        exit(1);
    }

    /* 将sockfd 添加到红黑树实例里面 */
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = sockfd;
    event.events = EPOLLIN;//读事件
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &event);
    if (ret == -1)
    {
        perror("epoll control error");
        exit(-1);
    }
    /* 循环接收 */
    int nums = 0;//监听多少个连接
    int readBytes = 0; //读到的字节数
    while (1)
    {
        struct epoll_event events[BUF_SIZE];
        memset(events, 0, BUF_SIZE);
        /* ⭐ */
        nums = epoll_wait(epfd, events, BUF_SIZE, -1);
        if (nums == -1)
        {
            perror("epoll wait error");
            exit(-1);
        }

        /* 程序运行到这有两种情况：①超时 ②有监听数据来 */
        for (int idx = 0; idx < nums; idx++)
        {
            int fd = events[idx].data.fd;
            if (fd == sockfd)
            {
                /* 有连接 */
                int connfd = accept(sockfd, NULL, NULL);\
                if (connfd == -1)
                {
                    perror("accept error");
                    exit(-1);
                }
                
                /* 将通信的句柄添加到树结点中 */
                struct epoll_event conn_event;
                memset(&event, 0, sizeof(conn_event));
                event.data.fd = connfd;
                event.events = EPOLLIN;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &conn_event);
                if (ret == -1)
                {
                    perror("epoll control error");
                    continue;
                }

            }
            else
            {
                /* 有数据通信 */
                char buffer[BUF_SIZE] = { 0 };
                readBytes = read(fd, buffer, sizeof(buffer) - 1);
                if (readBytes < 0)
                {
                    perror("read error");
                    /* 将该文件句柄从树上删除 */
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                 
                }
                else if (readBytes = 0)
                {
                    printf("客户端下线.....\n");
                    /* 将该文件句柄从树上删除 */
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
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
                    
                    usleep(500);
                }   
            }       
        }
    }
    close(sockfd);

    return 0;
}