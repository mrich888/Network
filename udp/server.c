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

int main()
{
    /* 服务端套接字 */
    int server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    /* 判断 */
    if (server_sock == -1)
    {
        perror("socker error");
        exit(-1);
    }
    /* 绑定本地地址 */
    struct sockaddr_in local_address;
    memset(&local_address, 0, sizeof(local_address));

    local_address.sin_family = AF_INET;
    local_address.sin_port = htons(SERVER_PORT);
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 绑定链接 */
    int local_address_len = 0;
    int ret = bind(server_sock, (struct sockaddr*)&local_address, local_address_len);
    if (ret == -1)
    {
        perror("bind error");
    }

    /* 客户端 */
    struct sockaddr_in client_address;
    int client_address_len = 0;
    memset(&client_address, 0, sizeof(client_address));

    /* 存数据 */
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    /* 先接收客户端的消息 */
    int readbytes = recvfrom(server_sock, buf, sizeof(buf), 0, (struct sockaddr *)&client_address, &client_address_len);
    if (readbytes > 0)
    {
        for (int idx = 0; idx < readbytes; idx++)
        {
            if (buf[idx] >= 'a' && buf[idx] <= 'z')
            {
                buf[idx] -= 32;
            }
        }
    }
    
    /* 再给客户端发回去 */
    sendto(server_sock, buf, sizeof(buf), 0, (struct sockaddr *)&client_address, client_address_len);
    
    close(server_sock);

    return 0;
}