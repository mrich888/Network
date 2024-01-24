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
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == -1)
    {
        perror("socker error");
    }
    /* 存储的要连接的服务器的 */
    struct sockaddr_in server_address;
    int server_address_len = sizeof(sockaddr);
    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    int ret = inet_pton(AF_INET, SERVER_IPADDRESS, (void *)&server_address.sin_addr.s_addr);//本地转大端
    if (ret != 1)
    {
        perror("inet_pton error");
    }
    
    char recvBuf[BUF_SIZE];
    memset(recvBuf, 0, sizeof(recvBuf));
    char bufW[BUF_SIZE];
    memset(bufW, 0, sizeof(bufW));

    
    strncpy(recvBuf, "ABcDe", strlen(recvBuf) - 1);
    /* 先发送 */
    sendto(client_sock, bufW, sizeof(bufW), 0, (struct sockaddr *)&server_address, server_address_len);
        
    recvfrom(client_sock, recvBuf, sizeof( recvBuf), 0, (struct sockaddr *)&server_address, &server_address_len);
    

    close(client_sock);
  

    return 0;
}