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


#define SERVER_PORT 6666
#define MAX_LISTEN 128
#define LOCAL_IPADDRESS " 127.0.0.1"
#define BUF_SIZE 128

void sigHander(int sigNum)
{

   
}

int main()
{

    /*  信号注册 */
    signal(SIGINT, sigHander);
    signal(SIGQUIT, sigHander);
    signal(SIGTSTP, sigHander);
    /* 创建套接字 */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socker error");
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
        exit(-1);
    }
    /* 客户信息 */
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));

    socklen_t clientAddressLen = 0;
    int acceptfd = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen);
    if (acceptfd == -1)
    {
        perror("accept error");
    }

    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    char replyBuf[BUF_SIZE];
    memset(replyBuf, 0, sizeof(replyBuf));
    int readBytes = 0;
    while (1)
    {
        readBytes = read(acceptfd, buf, sizeof(buf));
        if (readBytes <= 0)
        {
            perror("read error");
            close(acceptfd);
            break;
        }
        else
        {
            /*读到的字节数 */
            printf("buf:%s\n", buf);
            sleep(3);
            strncpy(replyBuf, "一起加油", sizeof(replyBuf) - 1);
            write(acceptfd, replyBuf, sizeof(replyBuf));
        }
    }

    close(sockfd);

    return 0;
}