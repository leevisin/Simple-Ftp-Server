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
char *username; //用户名
char *rnfr_name;

int tcp_server();
void str_trim_crlf(char *str);
void str_split(const char *str , char *left, char *right, char c);
void str_upper(char *str);
void handle_USER(int, char *str);
void handle_PASS(int, char *str);
void handle_SYST(int);
void handle_PWD(int);
void handle_DELE(int, char *str);
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
    // recv(client_sockfd, buffer, sizeof(buffer),0);
    // printf("Receive username: %s",buffer);
    
    // memset(buffer,0,sizeof(buffer));
    // send(client_sockfd, Enterpass, sizeof(Enterpass), 0);
    // recv(client_sockfd,buffer,sizeof(buffer),0);
	
    // printf("Receive password: %s", buffer);
    // if(strncmp(buffer,"PASS 123456",11)!=0){
    //     memset(buffer,0,sizeof(buffer));
    //     send(client_sockfd, Wrongpass, sizeof(Wrongpass), 0);
    // }

    // // 切换到home/student目录下
    // if(chdir("/home/student") == -1)
    //     exit(-1);

    // memset(buffer,0,sizeof(buffer));
    // send(client_sockfd, succ, sizeof(succ), 0);
    // int sys = recv(client_sockfd, buffer, sizeof(buffer),0);
    // if(strncmp(buffer,"SYST",4)==0 || sys<=0){
    //     send(client_sockfd, sysback , sizeof(sysback), 0);
    // }

    while(1){
        char com[COMMAND_MAX];//FTP指令
        char args[COMMAND_MAX];//FTP指令的参数
        memset(buffer,0,sizeof(buffer));
        memset(com, 0, sizeof (com));
        memset(args, 0, sizeof (args));

        int len = recv(client_sockfd, buffer, sizeof(buffer),0);

        str_trim_crlf(buffer);
        str_split(buffer, com, args, ' ');
        str_upper(com);
        // Need to display user IP Action speed&file when transmitting
        printf("Action=%s, Args=%s\n", com, args);

		if (strcmp("USER", com) == 0) {
			handle_USER(client_sockfd, args);
		} else if (strcmp("PASS", com) == 0) {
			handle_PASS(client_sockfd, args);
		} else if (strcmp("SYST", com) == 0) {
			handle_SYST(client_sockfd);
		} else if (strcmp("PWD", com) == 0) {
			handle_PWD(client_sockfd);
		} else if (strcmp("PORT", com) == 0) {
			handle_PORT(client_sockfd, args);
		} else if (strcmp("CWD", com) == 0) {
			handle_CWD(client_sockfd, args);
		} else if (strcmp("DELE", com) == 0) {
			handle_DELE(client_sockfd, args);
		} else if (strcmp("LIST", com) == 0) {
			// handle_LIST(client_sockfd);
		} else if (strcmp("MKD", com) == 0) {
			handle_MKD(client_sockfd, args);
		} else if (strcmp("RNFR", com) == 0) {
	 		handle_RNFR(client_sockfd, args);
		} else if (strcmp("RNTO", com) == 0) {
	 		handle_RNTO(client_sockfd, args);
		} else if (strcmp("QUIT", com) == 0) {
			handle_QUIT(client_sockfd);
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
			char buf[] = "Unimplement command.\r\n";
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

void handle_USER(int client_sockfd, char *args){
    struct passwd *pw;
    char buf[] = "331 Please specify the password.\r\n";
    // if((pw = getpwnam(args)) == NULL)
    // {
    //     sprintf(buf, "530 Login incorrect.\r\n");
    //     send(client_sockfd, buf, sizeof(buf), 0);
    //     return;
    // }

    if(strncmp(args,"student",7)!=0){
        sprintf(buf, "530 Login incorrect.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
    }
    username = args;
    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_PASS(int client_sockfd, char *args){

    char buf[] = "230 Login successful! Welcome!\r\n";

    if(strncmp(buffer,"PASS 123456",11)!=0){
        sprintf(buf, "530 Login incorrect.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
    }

    sprintf(buf, "230 Login successful! Welcome!\r\n ");

    //home---切换到主目录
    if(chdir("/home/student") == -1)
        exit(-1);

    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_SYST(int client_sockfd){
    char buf[] = "215 UNIX Type: L8\r\n";
    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_PWD(int client_sockfd){
    char tmp[1024] = {0};
    if(getcwd(tmp, sizeof tmp) == NULL)
    {
        //return值为-1/0，函数进入系统内核，
        char buf[] = "504 get cwd error\r\n";
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }
    char text[1024] = {0};
    snprintf(text, sizeof text, "257 \"%s\"\r\n", tmp);
    send(client_sockfd, text, sizeof(text), 0);
}

void handle_CWD(int client_sockfd, char *args){
    char buf[] = "250 Directory successfully changed.\r\n";
    if(chdir(args) == -1)
    {
        sprintf(buf, "550 Failed to change directory.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }

    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_PORT(int client_sockfd, char *args){
    //设置主动工作模式
    unsigned int v[6] = {0};
    sscanf(args, "%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);

    struct sockaddr_in *p_addr;
    p_addr = (struct sockaddr_in *)malloc(sizeof (struct sockaddr_in));
    memset(p_addr, 0, sizeof(struct sockaddr_in));
    p_addr->sin_family = AF_INET;

    char *p = (char*)&p_addr->sin_port;
    p[0] = v[4];
    p[1] = v[5];

    p = (char*)&p_addr->sin_addr.s_addr;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];
    p[3] = v[3];

    char buf[] = "200 PORT command successful. Consider using PASV.\r\n";
    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_LIST(int client_sockfd){
    
}

void handle_MKD(int client_sockfd, char *args){
    if(mkdir(args, 0777) == -1)
    {
        char buf[] = "550 Create directory operation failed.\r\n";
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }

    char text[1024] = {0};
    if(args[0] == '/') //绝对路径
        snprintf(text, sizeof text, "257 %s created.\r\n", args);
    else
    {
        //char *getcwd(char *buf, size_t size);
        char tmp[1024] = {0};
        if(getcwd(tmp, sizeof tmp) == NULL)
        {
            exit(-1);
        }
        snprintf(text, sizeof text, "257 %s/%s created.\r\n", tmp, args);
    }
    send(client_sockfd, text, sizeof(text), 0);
}

void handle_DELE(int client_sockfd, char *args){
    char buf[] = "250 Delete operation successful.\r\n";
    if(unlink(args) == -1)
    {
        //550 Delete operation failed.
        sprintf(buf, "550 Delete operation failed.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }
    //250 Delete operation successful.
    send(client_sockfd, buf, sizeof(buf), 0);

}

void handle_RNFR(int client_sockfd, char *args){
    if(rnfr_name) //防止内存泄露
    {
        free(rnfr_name);
        rnfr_name = NULL;
    }
    rnfr_name = (char*)malloc(strlen(args)+1);
    strcpy(rnfr_name, args);
    //350 Ready for RNTO.
    char buf[] = "350 Ready for RNTO.\r\n";
    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_RNTO(int client_sockfd, char *args){
    char buf[] = "250 Rename successful.\r\n";
    if(rnfr_name == NULL)
    {
        //503 RNFR required first.
        sprintf(buf, "503 RNFR required first.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }

    if(rename(rnfr_name, args) == -1)
    {
        sprintf(buf, "550 RRename failed.\r\n");
        send(client_sockfd, buf, sizeof(buf), 0);
        return;
    }
    free(rnfr_name);
    rnfr_name = NULL;

    //250 Rename successful.
    send(client_sockfd, buf, sizeof(buf), 0);
}

void handle_QUIT(int client_sockfd){
    char buf[] = "221 Goodbye\r\n";
    send(client_sockfd, buf, sizeof(buf), 0);
}