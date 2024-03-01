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


#define SERVER_PORT 8686 /* 。。。。。  */
#define SERVER_IP   "172.31.173.216"
#define BUFFER_SIZE 128

int main()
{
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
    
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    /* 端口 */
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    /* IP地址 */
    ret = inet_pton(AF_INET, SERVER_IP, (void *)&(serverAddress.sin_addr.s_addr));
    if (ret != 1)
    {
        perror("inet_pton error");
        exit(-1);
    }

    /* ip地址 */
    ret = connect(sockfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (ret == -1)
    {
        perror("connect error");
        exit(-1);
    }

    /* 写缓冲区 */
    char writeBuffer[BUFFER_SIZE];
    memset(writeBuffer, 0, sizeof(writeBuffer));

    /* 读缓冲区 */
    char recvBuffer[BUFFER_SIZE];
    memset(recvBuffer, 0, sizeof(recvBuffer));

    int readBytes = 0;
    while (1)
    {
        printf("input:");
        scanf("%s", writeBuffer);

        /* 带上字符串的结束符'\0' */
        write(sockfd, writeBuffer, strlen(writeBuffer) + 1);

        /* 接收数据 */
        readBytes = read(sockfd, recvBuffer, sizeof(recvBuffer) - 1);
        if (readBytes < 0)
        {
            perror("read error");
            exit(-1);
        }
        else if (readBytes == 0)
        {
            printf("readBytes == 0\n");
        }
        else
        {
            printf("recv:%s\n", recvBuffer);
        }
    }
    
    
    /* 休息5S */
    sleep(5);

    
    close(sockfd);

    return 0;
}