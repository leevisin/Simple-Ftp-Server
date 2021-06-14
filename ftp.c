#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <stdbool.h>

#define PORT1  21
#define PORT2  20
#define MAXLINE 4096
#define BUFFER_MAX 1024
#define SUCCESS 0

#define Entername "220 please enter username\r\n"
#define Enterpass "331 please enter password\r\n"
#define Wrongpass "530 Login incorrect\r\n"
#define Afterwrong "530 Please login with USER and PASS\r\n" 
#define Quitout "221 Goodbye\r\n"
#define Activeport "200 PORT command successful\r\n"
#define LSITCODE "150 Here comes the directory listing\r\n"
#define listfinish "226 Directory send OK\r\n"
#define get_open_succ "150 Openning data connection\r\n"
#define getfinish "226 Transfer complete\r\n"
#define succ "230 Login in successful\r\n"
#define sysback "215 UNIX Type: L8\r\n"

char* SERVER_IP = "127.0.0.1";
char buffer[BUFFER_MAX];
int tcp_server();

int main(int argc, char** argv){

    // 使用root权限运行
    if(getuid())
    {
        fprintf(stderr, "FtpServer must be started by root\n");
        exit(-1);
    }

    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;

    //创建一个监听fd
    server_sockfd = tcp_server();

    
    printf("Welcom to use FTP, waiting for client's requests.\n");


    socklen_t socket_len = sizeof(client_addr);
    if((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &socket_len)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            exit(-1);
    }
    printf("Accept client %s on TCP port %d\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    
    int flag_pass = 1;
			
    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, Entername, sizeof(Entername), 0);
    recv(client_sockfd, buffer, sizeof(buffer),0);
    printf("\nReceive username: %s",buffer);
    
    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, Enterpass, sizeof(Enterpass), 0);
    recv(client_sockfd,buffer,sizeof(buffer),0);
	
    printf("\nReceive password: %s", buffer);
    if(strncmp(buffer,"PASS 123456",11)!=0){
        memset(buffer,0,sizeof(buffer));
        send(client_sockfd, Wrongpass, sizeof(Wrongpass), 0);
    }

    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, succ, sizeof(succ), 0);
    int sys = recv(client_sockfd, buffer, sizeof(buffer),0);
    printf("buff: %s", buffer);
    if(strncmp(buffer,"SYST",4)==0 || sys<=0){
        send(client_sockfd, sysback , sizeof(sysback), 0);
    }

    while(flag_pass){
       
        memset(buffer,0,sizeof(buffer));
        int len = recv(client_sockfd, buffer, sizeof(buffer),0);
        printf("from client:%s\n",buffer);
		
        if(strncmp(buffer,"QUIT",4)==0 || len<=0){
			send(client_sockfd, Quitout, sizeof(Quitout), 0);
			break; 
		}
		
        // if(strncmp(buffer,"PWD",3)==0)

    }

    return 0;
}

int tcp_server(){
    int listenfd;
    struct sockaddr_in seraddr;

    //创建一个监听fd
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    seraddr.sin_family = AF_INET;
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    seraddr.sin_port = htons(PORT1);
	
    if(bind(listenfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    if(listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
    return listenfd;
}
