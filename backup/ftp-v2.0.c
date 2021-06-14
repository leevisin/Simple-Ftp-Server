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
#define COMMAND_MAX 1024
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
void str_trim_crlf(char *str);
void str_split(const char *str , char *left, char *right, char c);
void str_upper(char *str);
void handle_USER(int, char *str);
void handle_PASS(int, char *str);
void handle_SYST(int);
void handle_PWD(int);
void handle_PORT(int, char *str);
void handle_CWD(int, char *str);
void handle_LIST(int);
void handle_MKD(int, char *str);
void handle_RMD(int, char *str);
void handle_RNFR(int, char *str);
void handle_RNTO(int, char *str);
void handle_QUIT(int);
void handle_RETR(int, char *str);
void handle_STOR(int, char *str);
void handle_TYPE(int);
void handle_PASV(int);



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
    
    // int flag_pass = 1;
			
    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, Entername, sizeof(Entername), 0);
    recv(client_sockfd, buffer, sizeof(buffer),0);
    printf("Receive username: %s\n",buffer);
    
    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, Enterpass, sizeof(Enterpass), 0);
    recv(client_sockfd,buffer,sizeof(buffer),0);
	
    printf("Receive password: %s\n", buffer);
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

    while(1){
        char com[COMMAND_MAX];//FTP指令
        char args[COMMAND_MAX];//FTP指令的参数
        memset(buffer,0,sizeof(buffer));
        memset(com, 0, sizeof (com));
        memset(args, 0, sizeof (args));

        int len = recv(client_sockfd, buffer, sizeof(buffer),0);
        printf("from client:%s\n",buffer);

        str_trim_crlf(buffer);
        str_split(buffer, com, args, ' ');
        str_upper(com);
        // Need to display user IP Action speed&file when transmitting
        printf("Action=%s, Args=%s\n", com, args);

        // if(strncmp(buffer,"QUIT",4)==0 || len<=0){
		// 	send(client_sockfd, Quitout, sizeof(Quitout), 0);
		// 	break; 
		// }

		if (strcmp("USER", com) == 0) {
			// handle_USER(client_sockfd, args);
		} else if (strcmp("PASS", com) == 0) {
			// handle_PASS(client_sockfd, args);
		} else if (strcmp("SYST", com) == 0) {
			// handle_SYST(client_sockfd);
		} else if (strcmp("PWD", com) == 0) {
			// handle_PWD(client_sockfd);
		} else if (strcmp("PORT", com) == 0) {
			// handle_PORT(client_sockfd, args);
		} else if (strcmp("CWD", com) == 0) {
			// handle_CWD(client_sockfd, args);
		} else if (strcmp("LIST", com) == 0) {
			// handle_LIST(client_sockfd);
		} else if (strcmp("MKD", com) == 0) {
			// handle_MKD(client_sockfd, args);
		} else if (strcmp("RMD", com) == 0) {
	 		// handle_RMD(client_sockfd, args);
		} else if (strcmp("RNFR", com) == 0) {
	 		// handle_RNFR(client_sockfd, args);
		} else if (strcmp("RNTO", com) == 0) {
	 		// handle_RNTO(client_sockfd, args);
		} else if (strcmp("QUIT", com) == 0) {
			// handle_QUIT(client_sockfd);
			break;
		} else if (strcmp("RETR", com) == 0) {
			// handle_RETR(client_sockfd, args);
		} else if (strcmp("STOR", com) == 0) {
			// handle_STOR(client_sockfd, args);
		} else if (strcmp("TYPE", com) == 0) {
			// handle_TYPE(client_sockfd);
		} else if (strcmp("PASV", com) == 0) {
			// handle_PASV(client_sockfd);
		} else {
			char buf[100];
			strcpy(buf, "500 ");
			strcat(buf, com);
			strcat(buf, "cannot be recognized by server\r\n");
			send(client_sockfd, buf, sizeof(buf), 0);
		}

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

// 下面三个函数是处理Request命令的，分出com和args通过空格
void str_trim_crlf(char *str)
{
    char *p = &str[strlen(str)-1];
    while(*p == '\r' || *p == '\n')
        *p-- = '\0';
}

void str_split(const char *str , char *left, char *right, char c)
{
    //strchr返回字符串str中第一次出现字符c的位置
    char *p = strchr(str, c);
    if (p == NULL)
        strcpy(left, str);
    else
    {
        //strncpy最多拷贝p-str个字符到字符串left中
        strncpy(left, str, p-str);
        //strcpy从p+1指向的位置拷贝字符串到right指向的开始位置
        strcpy(right, p+1);
    }
}

void str_upper(char *str)
{
    while (*str)
    {
        *str = toupper(*str);
        ++str;
    }
}