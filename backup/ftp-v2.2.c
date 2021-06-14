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
// typedef struct 
// {
//     char command[COMMAND_MAX];//client发来的FTP指令
//     char com[COMMAND_MAX];//FTP指令
//     char args[COMMAND_MAX];//FTP指令的参数

//     uint32_t ip; //客户端ip地址
//     struct sockaddr_in ip_addr; //客户端ip地址
//     char username[100]; //用户名

//     int isLogin;//登陆状态来限制功能

//     int peer_fd;//客户连接的fd

//     int nobody_fd;//nobody进程所使用的fd
//     int proto_fd;//proto进程所使用的fd

//     struct sockaddr_in *p_addr; //port模式下对方的ip和port
//     int data_fd; //数据传输fd
//     int listen_fd; //监听fd，用于PASV模式

//     long long restart_pos; //文件传输断点
//     char *rnfr_name; //文件重命名 RNTR RNTO

//     int limits_max_upload; //限定的最大上传速度
//     int limits_max_download; //限定的最大下载速度
// }Session_t;

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
int get_trans_data_fd(int);
int recv_fd(int sockfd);
void trans_list_common(int list);
char *statbuf_get_perms(struct stat *sbuf);
char *statbuf_get_user_info(struct stat *sbuf);
char *statbuf_get_size(struct stat *sbuf);
char *statbuf_get_date(struct stat *sbuf);
char *statbuf_get_filename(struct stat *sbuf, const char *name);

void do_stat(char*,int);
void show_file_info(char* ,struct stat*,int);
void mode_to_letters(int ,char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);

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
    // 下面的可以删了，处理都在while循环里了
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
			handle_LIST(client_sockfd);
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
    // int list = 1;
    // //发起数据连接
    // if(get_trans_data_fd(client_sockfd) == 0)
    //     return ;

    // //给出150 Here comes the directory listing.
    // char buf1[] = "150 Here comes the directory listing.\r\n";
    // send(client_sockfd, buf1, sizeof(buf1), 0);

    // //传输目录列表
    // if(list == 1)
    //     trans_list_common(1);
    // else
    //     trans_list_common(0);
    // close(data_fd); //传输结束记得关闭
    // data_fd = -1;

    // //给出226 Directory send OK.
    // char buf2[] = "226 Directory send OK.\r\n";
    // send(client_sockfd, buf2, sizeof(buf2), 0);


    /*list files in directory called dirname*/
	DIR* dir_ptr;
	struct dirent * direntp; /*each entry*/
	if((dir_ptr = opendir(".")) == NULL)
		perror("opendir fails");
	while((direntp = readdir(dir_ptr)) !=NULL)
		do_stat(direntp->d_name, client_sockfd);
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

void handle_RETR(int client_sockfd, char *args){

}

// 建立数据连接
int get_trans_data_fd(int client_sockfd)
{
    //主动模式
    if(is_port)
    {
        //发送cmd
        char cmd = PRIV_SOCK_GET_DATA_SOCK;
        if(write(proto_fd, &cmd, sizeof(cmd))!=sizeof(1)){
            exit(-1);
        }
        //发送ip port
        char *ip = inet_ntoa(p_addr->sin_addr);
        uint16_t port = ntohs(p_addr->sin_port);
        size_t len = strlen(ip);
        if(write(proto_fd, &len, sizeof((int)ip))!=strlen(ip)){
            exit(-1);
        }
        if(write(proto_fd, ip, strlen(ip))!=strlen(ip)){
            exit(-1);
        }
        if(write(proto_fd, &port, sizeof(int))!=sizeof(port)){
            exit(-1);
        }
        //接收应答
        char result;
        if(read(proto_fd, &result, sizeof result) != sizeof(result))
        {
            exit(-1);
        }

        if(result == PRIV_SOCK_RESULT_BAD)
        {
            char buf[] = "500 get pasv data_fd error.\r\n";
            send(client_sockfd, buf, sizeof(buf), 0);
            exit(-1);
        }
        //接收fd
        data_fd = recv_fd(proto_fd);

        //释放port模式
        free(p_addr);
        p_addr = NULL;
    }

    // if(!is_port)
    // {
    //     //先给nobody发命令
    //     priv_sock_send_cmd(sess->proto_fd, PRIV_SOCK_PASV_ACCEPT);

    //     //接收结果
    //     char res = priv_sock_recv_result(sess->proto_fd);
    //     if(res == PRIV_SOCK_RESULT_BAD)
    //     {
    //         ftp_reply(sess, FTP_BADCMD, "get pasv data_fd error");
    //         fprintf(stderr, "get data fd error\n");
    //         exit(EXIT_FAILURE);
    //     }

    //     //接收fd
    //     sess->data_fd = priv_sock_recv_fd(sess->proto_fd);  
    // }


    return 1;
}


int recv_fd(int sockfd)
{
    int ret;
    struct msghdr msg;
    char recvchar;
    struct iovec vec;
    int recvfd;
    char cmsgbuf[CMSG_SPACE(sizeof(recvfd))];
    struct cmsghdr *p_cmsg;
    int *p_fd;
    vec.iov_base = &recvchar;
    vec.iov_len = sizeof(recvchar);
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);
    msg.msg_flags = 0;

    p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
    *p_fd = -1;
    ret = recvmsg(sockfd, &msg, 0);
    if(ret != 1)
        exit(-1);

    p_cmsg = CMSG_FIRSTHDR(&msg);
    if(p_cmsg == NULL)
        exit(-1);

    p_fd = (int *)CMSG_DATA(p_cmsg);
    recvfd = *p_fd;
    if(recvfd == -1)
        exit(-1);

    return recvfd;    
}

void trans_list_common(int list)
{
    DIR *dir = opendir(".");
    if(dir == NULL)
        exit(-1);

    struct dirent *dr;
    while((dr = readdir(dir)))
    {
        const char *filename = dr->d_name;
        if(filename[0] == '.')
            continue;

        char buf[1024] = {0};
        struct stat sbuf;
        if(lstat(filename, &sbuf) == -1)
            exit(-1);

        if(list == 1) // LIST
        {
            strcpy(buf, statbuf_get_perms(&sbuf));
            strcat(buf, " ");
            strcat(buf, statbuf_get_user_info(&sbuf));
            strcat(buf, " ");
            strcat(buf, statbuf_get_size(&sbuf));
            strcat(buf, " ");
            strcat(buf, statbuf_get_date(&sbuf));
            strcat(buf, " ");
            strcat(buf, statbuf_get_filename(&sbuf, filename));
        }
        else //NLST
        {
            strcpy(buf, statbuf_get_filename(&sbuf, filename));
        }

        strcat(buf, "\r\n");
        write(data_fd, buf, strlen(buf));
    }

    closedir(dir);
}

char *statbuf_get_perms(struct stat *sbuf)
{
    //这里使用static返回perms
    static char perms[] = "----------";
    mode_t mode = sbuf->st_mode;

    //文件类型
    switch(mode & S_IFMT)
    {
        case S_IFSOCK:
            perms[0] = 's';
            break;
        case S_IFLNK:
            perms[0] = 'l';
            break;
        case S_IFREG:
            perms[0] = '-';
            break;
        case S_IFBLK:
            perms[0] = 'b';
            break;
        case S_IFDIR:
            perms[0] = 'd';
            break;
        case S_IFCHR:
            perms[0] = 'c';
            break;
        case S_IFIFO:
            perms[0] = 'p';
            break;
    }
    //权限
    if(mode & S_IRUSR)
        perms[1] = 'r';
    if(mode & S_IWUSR)
        perms[2] = 'w';
    if(mode & S_IXUSR)
        perms[3] = 'x';
    if(mode & S_IRGRP)
        perms[4] = 'r';
    if(mode & S_IWGRP)
        perms[5] = 'w';
    if(mode & S_IXGRP)
        perms[6] = 'x';
    if(mode & S_IROTH)
        perms[7] = 'r';
    if(mode & S_IWOTH)
        perms[8] = 'w';
    if(mode & S_IXOTH)
        perms[9] = 'x';

    if(mode & S_ISUID)
        perms[3] = (perms[3] == 'x') ? 's' : 'S';
    if(mode & S_ISGID)
        perms[6] = (perms[6] == 'x') ? 's' : 'S';
    if(mode & S_ISVTX)
        perms[9] = (perms[9] == 'x') ? 't' : 'T';

    return perms;
}

char *statbuf_get_user_info(struct stat *sbuf)
{
    static char info[1024] = {0};
    snprintf(info, sizeof info, " %3d %8d %8d", sbuf->st_nlink, sbuf->st_uid, sbuf->st_gid);

    return info;
}

char *statbuf_get_size(struct stat *sbuf)
{
    static char buf[100] = {0};
    snprintf(buf, sizeof buf, "%8lu", (unsigned long)sbuf->st_size);
    return buf;
}

//获取文件最近更改日期
char *statbuf_get_date(struct stat *sbuf)
{
    static char datebuf[1024] = {0};
    struct tm *ptm;
    time_t ct = sbuf->st_ctime;
    if((ptm = localtime(&ct)) == NULL)
        exit(-1);

    const char *format = "%b %e %H:%M"; //时间格式

    if(strftime(datebuf, sizeof datebuf, format, ptm) == 0)
    {
        fprintf(stderr, "strftime error\n");
        exit(EXIT_FAILURE);
    }

    return datebuf;
}

//获取文件名字
char *statbuf_get_filename(struct stat *sbuf, const char *name)
{
    static char filename[1024] = {0};
    //name 处理链接名字
    if(S_ISLNK(sbuf->st_mode))
    {
        char linkfile[1024] = {0};
        if(readlink(name, linkfile, sizeof linkfile) == -1)
            exit(-1);
        snprintf(filename, sizeof filename, "%s -> %s", name, linkfile);
    }else
    {
        strcpy(filename, name);
    }

    return filename;
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
	/*sprintf(buf1,"%s",modestr);
	sprintf(buf2,"%4d ",(int)info_p->st_nlink);
	sprintf(buf3,"%-8s ",uid_to_name(info_p->st_uid));
	sprintf(buf4,"%-8s ",gid_to_name(info_p->st_gid));
	sprintf(buf5,"%8ld ",(long)info_p->st_size);
	sprintf(buf6,"%.12s ",ctime(&info_p->st_mtime)+4);
	sprintf(buf7,"%s\n",filename);
	*/
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
	if((pw_ptr =getpwuid(uid)) == NULL)
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
	if((grp_ptr =getgrgid(gid)) == NULL)
	{
		printf("hello wofjl");
		sprintf(numstr,"%d",gid);
		return numstr;
	}
	else
		return grp_ptr->gr_name;
}
