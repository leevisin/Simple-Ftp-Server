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

// FTP服务进程向nobody进程请求的命令
#define PRIV_SOCK_GET_DATA_SOCK 1
#define PRIV_SOCK_PASV_ACTIVE 2
#define PRIV_SOCK_PASV_LISTEN 3
#define PRIV_SOCK_PASV_ACCEPT 4

// nobody进程对FTP服务进程的应答
#define PRIV_SOCK_RESULT_OK 1
#define PRIV_SOCK_RESULT_BAD 2

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
int is_port = 1;
struct sockaddr_in *p_addr; //port模式下对方的ip和port
int data_fd; //数据传输fd
int nobody_fd;//nobody进程所使用的fd
int proto_fd;//proto进程所使用的fd
int data_port;

int tcp_server(void);
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
void handle_LIST(int, char dirname[]);
void handle_MKD(int, char *str);
void handle_RMD(int, char *str);
void handle_RNFR(int, char *str);
void handle_RNTO(int, char *str);
void handle_QUIT(int);
void handle_RETR(int, int, char *str);
void handle_STOR(int, char *str);
void handle_TYPE(int);
void handle_PASV(int);
int recv_fd(int sockfd);
int get_trans_data_fd(int);
// void trans_list_common(int list);
// char *statbuf_get_perms(struct stat *sbuf);
// char *statbuf_get_user_info(struct stat *sbuf);
// char *statbuf_get_size(struct stat *sbuf);
// char *statbuf_get_date(struct stat *sbuf);
// char *statbuf_get_filename(struct stat *sbuf, const char *name);


void do_stat(char*,int);
void show_file_info(char* ,struct stat*,int);
void mode_to_letters(int ,char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);
int split(char dst[][80], char* str, const char* spl);
int test(char *pcBuf, char *pcRes);

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
    			
    memset(buffer,0,sizeof(buffer));
    send(client_sockfd, Entername, sizeof(Entername), 0);

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
            int datafd = get_trans_data_fd(client_sockfd);
			handle_LIST(datafd, ".");
            close(datafd);
			send(client_sockfd, listfinish, sizeof(listfinish), 0);
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
            int datafd = get_trans_data_fd(client_sockfd);
			handle_RETR(client_sockfd, datafd, args);
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

int split(char dst[][80], char* str, const char* spl)
{
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while( result != NULL )
    {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

int test(char *pcBuf, char *pcRes)
{
	char *pcBegin = NULL;
	char *pcEnd = NULL;
 
	pcBegin = strstr(pcBuf, ",");
	pcEnd = strstr(pcBuf, "\r");
 
	if(pcBegin == NULL || pcEnd == NULL || pcBegin > pcEnd)
	{
		printf("Mail name not found!\n");
	}
	else
	{
		pcBegin += strlen(":");
		memcpy(pcRes, pcBegin, pcEnd-pcBegin);
	}
 
	return SUCCESS;
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

void handle_LIST(int sockfd, char dirname[]){
    /*list files in directory called dirname*/
	DIR* dir_ptr;
	struct dirent * direntp; /*each entry*/
	if((dir_ptr = opendir(dirname)) == NULL)
		perror("opendir fails");
	while((direntp = readdir(dir_ptr)) !=NULL)
		do_stat(direntp->d_name, sockfd);
	closedir(dir_ptr);
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

void handle_RETR(int client_sockfd,int datafd, char *args){
    char filename[] = "./";
    strcat(filename, args);
    FILE *fp = fopen(filename, "rb");
    if ( fp == NULL) {
        printf("fp is NULL\n");
        exit(-1);
    }
    send(client_sockfd, get_open_succ, sizeof(get_open_succ), 0);
    // 问题读了两个有关
    memset(buffer,0,sizeof(buffer));
    size_t nreads, nwrites;
    while(nreads = fread(buffer, sizeof(char), sizeof(buffer), fp)){
        printf("nreads=%d\n",nreads);
        if((nwrites = write(datafd, buffer, nreads)) != nreads){
            printf("nwrites=%d\n",nwrites);
            fprintf(stderr, "write error\n");
            fclose(fp);
            close(datafd);
            exit(-1);
        }
        printf("nwrites=%d\n",nwrites);
        fclose(fp);
        close(datafd);
        send(client_sockfd, getfinish, sizeof(getfinish), 0);
    }
}

void do_stat(char* filename, int sockfd)
{
	struct stat info;
	if((stat(filename,&info)) == -1)
		perror(filename);
	else
		show_file_info(filename,&info,sockfd);
}
 
void show_file_info(char* filename,struct stat * info_p,int sockfd)
{
	/*display the info about filename . the info is stored in struct at * info_p*/
	
	char modestr[11];
	mode_to_letters(info_p->st_mode,modestr);
	char buf1[1024];
	char buf2[1024];
	char buf3[1024];
	char buf4[1024];
	char buf5[1024];
	char buf6[1024];
	char buf7[1024];
	char buf[128];
	memset(buf7,0,sizeof(buf7));
	sprintf(buf1,"%s",modestr);
	sprintf(buf2,"%s%4d ",buf1,(int)info_p->st_nlink);
	sprintf(buf3,"%s%-8s ",buf2,uid_to_name(info_p->st_uid));
	sprintf(buf4,"%s%-8s ",buf3,gid_to_name(info_p->st_gid));
	sprintf(buf5,"%s%8ld ",buf4,(long)info_p->st_size);
	sprintf(buf6,"%s%.12s ",buf5,ctime(&info_p->st_mtime)+4);
	sprintf(buf7,"%s%s\n",buf6,filename);
	sprintf(buf,"%s%s%s%s%s%s%s",buf1,buf2,buf3,buf4,buf5,buf6,buf7);
	send(sockfd, buf7, strlen(buf7), 0);
}
 
void mode_to_letters(int mode,char str[])
{
	strcpy(str,"----------");
	if(S_ISDIR(mode)) str[0] = 'd';  //"directory ?"
	if(S_ISCHR(mode)) str[0] = 'c';  //"char decices"?
	if(S_ISBLK(mode)) str[0] = 'b';  //block device?
	
	//3 bits for user
	if(mode&S_IRUSR) str[1] = 'r';
	if(mode&S_IWUSR) str[2] = 'w';
	if(mode&S_IXUSR) str[3] = 'x';
	
	//3 bits for group
	if(mode&S_IRGRP) str[4] = 'r';
	if(mode&S_IWGRP) str[5] = 'w';
	if(mode&S_IXGRP) str[6] = 'x';
	
	//3 bits for other
	if(mode&S_IROTH) str[7] = 'r';
	if(mode&S_IWOTH) str[8] = 'w';
	if(mode&S_IXOTH) str[9] = 'x';
}
 
char* uid_to_name(uid_t uid)
{
	struct passwd* pw_ptr;
	static char numstr[10];
	if((pw_ptr = getpwuid(uid)) == NULL)
	{
		sprintf(numstr,"%d",uid);
		printf("world");
		return numstr;
	}
	return pw_ptr->pw_name;
}
 
char* gid_to_name(gid_t gid)
{
	/*returns pointer to group number gid, used getgrgid*/
	struct group* grp_ptr;
	static char numstr[10];
	if((grp_ptr = getgrgid(gid)) == NULL)
	{
		sprintf(numstr,"%d",gid);
		return numstr;
	}
	else
		return grp_ptr->gr_name;
}


int get_trans_data_fd(int client_sockfd){
    struct sockaddr_in servaddr, mine;

    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    mine.sin_family = AF_INET;
    mine.sin_port = htons(20);  
    mine.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 设置端口复用的套接字
    int opt = 1;
    setsockopt(sock_cli, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    if(bind(sock_cli, (struct sockaddr*)&mine, sizeof(mine))<0){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = p_addr->sin_port;  
    servaddr.sin_addr.s_addr = p_addr->sin_addr.s_addr;
    
    int con = connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
    if (con < 0)
    {
        printf("Connect refusion");
        exit(1);
    }

    send(client_sockfd, LSITCODE, sizeof(LSITCODE), 0);
    return sock_cli;

}