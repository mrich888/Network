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
#include <pthread.h>


#define SERVER_PORT 6666
#define MAX_LISTEN 128
#define LOCAL_IPADDRESS " 127.0.0.1"
#define BUF_SIZE 128

void * threadHandle(void *arg)
{
    /* 线程分离 */
    pthread_detach(pthread_self());
    /* 设置通信句柄 */
    int acceptfd = *(int *)arg;
    
    /* 通信 */
    char recvBuf[BUF_SIZE];//接收缓冲区
    memset(recvBuf, 0, sizeof(recvBuf));

    char sendBuf[BUF_SIZE];//发送缓冲区
    memset(sendBuf, 0, sizeof(sendBuf));

    /* 接收读取到的字节数 */
    int readBytes = 0;
    while (1)
    {
        readBytes = read(acceptfd, (void *)&recvBuf, sizeof(recvBuf));
        if (readBytes <= 0)
        {
            perror("read error");
            close(acceptfd);
            break;
        }
        else
        {
           /* 读到的字符串 */
            printf("buffer:%s\n", recvBuf);
            if (strncmp(recvBuf, "123456", strlen("123456")) == 0)
            {
                strncpy(sendBuf, "一起加油123456", sizeof(sendBuf) - 1);
                sleep(5);
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
            else if (strncmp(recvBuf, "778899", strlen("778899")) == 0)
            {
                strncpy(sendBuf, "一起加油778899", sizeof(sendBuf) - 1);
                sleep(5);
                write(acceptfd, sendBuf, sizeof(sendBuf));
            }
        }
    }
    /* 线程退出 */
    pthread_exit(NULL);

}


// void sigHander(int sigNum)
// {

   
// }

int main()
{

    // /*  信号注册 */
    // signal(SIGINT, sigHander);
    // signal(SIGQUIT, sigHander);
    // signal(SIGTSTP, sigHander);
    /* 创建套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socker error");
        exit(-1);
    }
    /* 端口复用 */
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
    /* 客户信息 */
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));
    int acceptfd = 0;
    while (1)
    {
        socklen_t clientAddressLen = 0;
        acceptfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (acceptfd == -1)
        {
            perror("accept error");
            exit(-1);
        }
        /* 每个线程服务一个客户 */
        pthread_t pid;
        ret = pthread_create(&pid, NULL, threadHandle, (void *)&acceptfd);
        if (ret != 0)
        {
            perror("thread error");
            exit(-1);
        }
    }
    

    close(sockfd);

    return 0;
}