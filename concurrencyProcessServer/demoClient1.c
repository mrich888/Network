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
#define SERVER_IPADDRESS "172.31.173.216"
#define BUF_SIZE 128

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socker error");
    }
    /* 存储的要连接的服务器的 */
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    /* 协议族 */
    serverAddress.sin_family = AF_INET;
    /* 端口 */
    serverAddress.sin_port = htons(SERVER_PORT);
    /*  */
    int ret = inet_pton(AF_INET, SERVER_IPADDRESS, (void *)&serverAddress.sin_addr.s_addr);
    if (ret != 1)
    {
        perror("inet_pton error");
    }
    
    ret = connect(sockfd,(struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (ret == -1)
    {
        perror("conner error");
        exit(-1);
    }

    char recvBuf[BUF_SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));

    while (1)
    {
        strncpy(buf, "123456", sizeof(buf) - 1);
        write(sockfd, buf, sizeof(buf));

        read(sockfd, recvBuf, sizeof(recvBuf));
        printf("recv:%s\n", recvBuf);

        sleep(5);
    
    }
    
    close(sockfd);
  

    return 0;
}