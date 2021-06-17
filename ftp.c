#define _GNU_SOURCE
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
#include<ctype.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<dirent.h>
#include<sys/time.h>
#include<time.h>
#include<ctype.h>
#include<fcntl.h>
#include <arpa/inet.h>
#include <shadow.h>

#define BUFFER_MAX 4096
#define COMMAND_MAX 1024
#define SERVER_IP "127.0.0.1"
#define bw_upload_rate_max 10
#define bw_download_rate_max 10

char buffer[BUFFER_MAX];
char username[100]; //用户名
char password[100]; //密码
char *rnfr_name;
struct sockaddr_in client_addr, data_addr;
socklen_t client_addr_len, data_addr_len;
struct sockaddr_in *p_addr; //port模式下对方的ip和port
int ascii_mode = 1;
const char *mode[] = {"BINARY","ASCII"};
int isLogin = 0;
int isPasv = 0;
int data_sock;
long bw_transfer_start_sec = 0;
long bw_transfer_start_usec = 0;
static struct timeval tv = {0, 0}; //全局变量

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
void handle_RETR(int, char *str);
void handle_STOR(int, char *str);
void handle_TYPE(int, char *str);
void handle_PASV(int);
int recv_fd(int sockfd);
int get_trans_data_fd(int);
int replace(char *str, char *olds, char *news, int max_length);
void limit_rate(int bytes_transfered, int is_upload);
int get_time_sec();
int get_time_usec();
int nano_sleep(double t);

int main(int argc, char** argv){

    // 使用root权限运行
    if(getuid())
    {
        fprintf(stderr, "FtpServer must be started by root\n");
        exit(-1);
    }

    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr;

    //创建一个监听fd
    server_sockfd = tcp_server();

    printf("Welcom to use FTP, waiting for client's requests.\n");

    socklen_t socket_len = sizeof(client_addr);
    if((client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &socket_len)) == -1){
        printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
        exit(-1);
    }
    printf("Accept client %s on TCP port %d\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
    			
    memset(buffer, 0, sizeof(buffer));
    char reply[] = "220 please enter username\r\n";
    write(client_sockfd, reply, sizeof(reply));

    while(1){
        char com[COMMAND_MAX];//FTP指令
        char args[COMMAND_MAX];//FTP指令的参数
        
        memset(com, 0, sizeof (com));
        memset(args, 0, sizeof (args));

        memset(buffer,0,sizeof(buffer));
        int len = read(client_sockfd, buffer, sizeof(buffer));

        str_trim_crlf(buffer);
        str_split(buffer, com, args, ' ');
        str_upper(com);
        // Need to display user IP Action speed&file when transmitting
        printf("Username=%s, IP=%s, ", username, inet_ntoa(client_addr.sin_addr));
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
			handle_LIST(client_sockfd, ".");
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
            if(isLogin){
                handle_RETR(client_sockfd, args);
            }
            else{
                char buf[] = "Please Login in to Download File.\r\n";
                write(client_sockfd, buf, strlen(buf));
            }
		} else if (strcmp("STOR", com) == 0) {
			handle_STOR(client_sockfd, args);
		} else if (strcmp("TYPE", com) == 0) {
			handle_TYPE(client_sockfd, args);
		} else if (strcmp("PASV", com) == 0) {
			handle_PASV(client_sockfd);
		} else {
            char buf[] = "Unimplement command.\r\n";
            write(client_sockfd, buf, strlen(buf));
		}

    }

    return 0;
}

int tcp_server(){
    int listenfd;
    struct sockaddr_in seraddr;

    //创建一个监听fd
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    seraddr.sin_family = AF_INET;
    seraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    seraddr.sin_port = htons(21);

    if(bind(listenfd, (struct sockaddr*)&seraddr, sizeof(seraddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    if(listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
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
    char reply[] = "331 Please specify the password.\r\n";
    stpcpy(username, args);
    // printf("Username:%s\n",username);
    stpcpy(reply, "331 Please spicify the password.\r\n");
    write(client_sockfd, reply, strlen(reply));
    // printf("waiting for the password....\n");
}

void handle_PASS(int client_sockfd, char *args){

    char reply[] = "230 Login successful! Welcome!\r\n";

    stpcpy(password, args);
    // printf("password:%s\n", password);
    struct spwd *sp;
    if((sp = getspnam(username)) == NULL)
    {
        stpcpy(reply, "530 Login incorrect.\r\n");
        write(client_sockfd, reply, strlen(reply));
        printf("login failed because of password!\n");
        isLogin=0;
    }
    else if(strcmp(sp->sp_pwdp, (char*)crypt(password, sp->sp_pwdp))==0)
    {
        stpcpy(reply, "230 Login successful.\r\n");
        write(client_sockfd, reply, strlen(reply));
        // printf("login successful!\n");
        isLogin=1;
        //home---切换到主目录
		if(chdir("/home/student") == -1)
			exit(-1);
    }
    else
    {
        stpcpy(reply, "530 Login incorrect.\r\n");
        write(client_sockfd, reply, strlen(reply));
        isLogin=0;
    }

}

void handle_SYST(int client_sockfd){
    char buf[] = "215 UNIX.\r\n";
    write(client_sockfd, buf, strlen(buf));
}

void handle_PWD(int client_sockfd){
    char tmp[1024] = {0};
    if(getcwd(tmp, sizeof tmp) == NULL)
    {
        //return值为-1/0，函数进入系统内核，
        char buf[] = "504 get cwd error\r\n";
        write(client_sockfd, buf, strlen(buf));
        return;
    }
    char text[1024] = {0};
    snprintf(text, sizeof text, "257 \"%s\"\r\n", tmp);
    write(client_sockfd, text, sizeof(text));
}

void handle_CWD(int client_sockfd, char *args){
    char buf[] = "250 Directory successfully changed.\r\n";
    if(chdir(args) != 0)
    {
        sprintf(buf, "550 Failed to change directory.\r\n");
    }

    write(client_sockfd, buf, strlen(buf));
}

void handle_PORT(int client_sockfd, char *args){
    //设置主动工作模式
    isPasv = 0;
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
    write(client_sockfd, buf, strlen(buf));
}

void handle_LIST(int client_sockfd, char dirname[]){

    int datafd = get_trans_data_fd(client_sockfd);

    char reply[] = "150 Here comes the directory listing.\r\n";
    write(client_sockfd, reply, strlen(reply));
    // printf("begin list...\n");

    FILE *listopen;
    char databuf[PIPE_BUF];
    int n;

    listopen = popen("ls -l", "r"); //popen()会调用fork()产生子进程，然后从子进程中调用/bin/sh -c 来执行参数command 的指令
    if(listopen == 0)
    {
        perror("ls stream open failed!\n");
        stpcpy(reply, "500 Transfer stream error\r\n");
        printf("stream error!\n");
        close(datafd);
    }
    else
    {
        memset(databuf, 0, PIPE_BUF - 1); // PIPE_BUF为4096字节
        while((n = read(fileno(listopen), databuf, PIPE_BUF)) > 0)
        {	
            replace(databuf, "\n", "\r\n", PIPE_BUF-1);
            // printf("%s\n", databuf);
            write(datafd, databuf, strlen(databuf));
            memset(databuf, 0, PIPE_BUF - 1);
        }
        memset(databuf, 0, PIPE_BUF - 1);
        if(pclose(listopen) != 0)
        {
            perror("Non-zero return value from \"ls -l\"\n");
            printf("stream close error!\n");
        }
        close(datafd);
        stpcpy(reply, "226 Directory send OK.\r\n");
        // printf("list complete!\n");
    }
    write(client_sockfd, reply, strlen(reply));


}

void handle_MKD(int client_sockfd, char *args){
    if(mkdir(args, 0777) == -1)
    {
        char buf[] = "550 Create directory operation failed.\r\n";
        write(client_sockfd, buf, strlen(buf));
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
    write(client_sockfd, text, sizeof(text));
}

void handle_DELE(int client_sockfd, char *args){
    char buf[] = "250 Delete operation successful.\r\n";
    if(unlink(args) == -1)
    {
        //550 Delete operation failed.
        sprintf(buf, "550 Delete operation failed.\r\n");
        write(client_sockfd, buf, strlen(buf));
        return;
    }
    //250 Delete operation successful.
    write(client_sockfd, buf, strlen(buf));

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
    write(client_sockfd, buf, strlen(buf));
}

void handle_RNTO(int client_sockfd, char *args){
    char buf[] = "250 Rename successful.\r\n";
    if(rnfr_name == NULL)
    {
        //503 RNFR required first.
        sprintf(buf, "503 RNFR required first.\r\n");
        write(client_sockfd, buf, strlen(buf));
        return;
    }

    if(rename(rnfr_name, args) == -1)
    {
        printf("Rename Failed.\n");
        sprintf(buf, "550 Rename failed.\r\n");
        write(client_sockfd, buf, strlen(buf));
        return;
    }
    free(rnfr_name);
    rnfr_name = NULL;

    //250 Rename successful.
    write(client_sockfd, buf, strlen(buf));
}

void handle_QUIT(int client_sockfd){
    char buf[] = "221 Goodbye\r\n";
    write(client_sockfd, buf, strlen(buf));
}

void handle_RETR(int client_sockfd, char *args){

    bw_transfer_start_sec = get_time_sec();
    bw_transfer_start_usec = get_time_usec();

    if(isPasv==0){
        int datafd = get_trans_data_fd(client_sockfd);

        FILE *file;
        unsigned char databuff[BUFFER_MAX] = "";
        int bytes;

        char reply[] = "226 Transfer complete\r\n";

        file = fopen(args, "r");
        if(file == 0)
        {
            perror("file open failed.\n");
            stpcpy(reply, "550 Failed to open file.\r\n");
        }
        else
        {
            sprintf(reply, "150 Opening %s mode data connection for %s.\r\n", mode[ascii_mode], args);
            write(client_sockfd, reply, strlen(reply));
            while((bytes = read(fileno(file), databuff, BUFFER_MAX)) > 0)
            {
                if(ascii_mode==1)
                {
                    replace((char *)databuff, "\r\n", "\n", BUFFER_MAX-1);
                    replace((char *)databuff, "\n", "\r\n", BUFFER_MAX-1);
                    write(datafd, (const char *)databuff, strlen((const char *)databuff));
                }
                else if(ascii_mode==0)
                {
                    write(datafd, (const char *)databuff, bytes);
                }
                limit_rate(bytes, 0);
                memset(&databuff, 0, BUFFER_MAX);
            }
            memset(&databuff, 0, BUFFER_MAX);

            fclose(file);
            stpcpy(reply, "226 Transfer complete\r\n");
            // printf("file transfer complete!\n");
        }

        close(datafd);
        write(client_sockfd, reply, strlen(reply));
    }
    else{
        int connection, fd;
        struct stat stat_buf;
        off_t offset = 0;
        int sent_total = 0;
        char reply[100] = {0};

        client_addr_len = sizeof(client_addr);
        if(access(args, R_OK)==0 && (fd = open(args, O_RDONLY))){
            fstat(fd, &stat_buf); // 复制文件状态到stat_buf中

            stpcpy(reply, "150 Opening BINARY mode data connection for ");
            strcat(reply, args);
            strcat(reply, ".\r\n");
            // printf("file transfer begin!\n");
            write(client_sockfd, reply, strlen(reply));

            connection = accept(data_sock, (struct sockaddr*) &client_addr, &client_addr_len);
        
            close(data_sock);
            if(sent_total = sendfile(connection, fd, &offset, stat_buf.st_size)){

                if(sent_total != stat_buf.st_size){
                    perror("sendfile error!\n");
                }
                else{
                    stpcpy(reply, "226 File has been downloaded OK.\r\n");
                    // printf("file transfer complete!\n");
                    write(client_sockfd, reply, strlen(reply));
                }
            }
            else{
                stpcpy(reply, "550 Failed to read file.\r\n");
                printf("Failed to get file!\n");
                write(client_sockfd, reply, strlen(reply));
            }
        }
        else{
            stpcpy(reply, "550 Failed to get file\r\n");
            printf("Failed to get file!\n");
            write(client_sockfd, reply, strlen(reply));
        }
        close(fd);
        close(connection);
    }
    

}

void handle_STOR(int client_sockfd, char* args){

    bw_transfer_start_sec = get_time_sec();
    bw_transfer_start_usec = get_time_usec();

    if(isPasv==0){
        int datafd = get_trans_data_fd(client_sockfd);

        FILE *file;
        unsigned char databuff[BUFFER_MAX];
        int bytes = 0;

        char buf[] = "226 Transfer complete\r\n";

        file = fopen(args, "w");
        if(file == 0)
        {
            perror("file open failed!\n");
            stpcpy(buf, "450 Cannot create the file\r\n");
            close(datafd);
        }
        else
        {
            sprintf(buf, "150 Opening %s mode data connection for %s.\r\n", mode[ascii_mode], args);
            write(client_sockfd, buf, strlen(buf));
            // printf("file opened!\n"); 
            while((bytes = read(datafd, databuff, BUFFER_MAX)) > 0)
            {
                if(isLogin==0){
                    limit_rate(bytes, 1);
                }
                write(fileno(file), databuff, bytes);
            }

            fclose(file);
            close(datafd);
            stpcpy(buf, "226 Transfer complete\r\n");
            // printf("file transfer complete!\n");
        }
        write(client_sockfd, buf, strlen(buf));
    }
    else{
        int connection, fd;
        int pipefd[2];
        int res;
        const int buff_size = 8192;
        char reply[100] = {0};
        FILE *fp = fopen(args,"w");

        if(fp==NULL){
        /* TODO: write status message here! */
            perror("file open failed!\n");
            stpcpy(reply, "550 Failed to create file.\r\n");
            write(client_sockfd, reply, strlen(reply));
        }
        /* Passive mode */
        else{
            fd = fileno(fp);
            connection = accept(data_sock, (struct sockaddr*) &client_addr, &client_addr_len);;
            close(data_sock);
            if(pipe(pipefd)==-1)
            {
                perror("pipe create failed!\n");
                stpcpy(reply, "550 Failed to open pipe.\r\n");
                write(client_sockfd, reply, strlen(reply));
            }
            else
            {
                stpcpy(reply, "150 Ok to send data.\r\n");
                write(client_sockfd, reply, strlen(reply));
                // printf("Begin transfering data.....\n");

                /* Using splice function for file receiving.
                * The splice() system call first appeared in Linux 2.6.17.
                */

               // ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags);  
                while ((res = splice(connection, 0, pipefd[1], NULL, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE))>0)
                {
                    splice(pipefd[0], NULL, fd, 0, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE);
                }

                /* TODO: signal with ABOR command to exit */

                /* Internal error */
                if(res==-1){
                    perror("splice file failed!\n");
                    stpcpy(reply, "550 Failed to open pipe.\r\n");
                    write(client_sockfd, reply, strlen(reply));
                }
                else{
                    stpcpy(reply, "226 Transfer complete.\r\n");
                    write(client_sockfd, reply, strlen(reply));
                    // printf("Transfer success!\n");
                }
            }
            close(connection);
            close(fd);
        }
    }
}

int get_trans_data_fd(int client_sockfd){
    struct sockaddr_in clntaddr, srvraddr;

    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    srvraddr.sin_family = AF_INET;
    srvraddr.sin_port = htons(20);  
    srvraddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 设置端口复用的套接字
    int opt = 1;
    setsockopt(sock_cli, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    if(bind(sock_cli, (struct sockaddr*)&srvraddr, sizeof(srvraddr))<0){
        printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return 0;
    }

    memset(&clntaddr, 0, sizeof(clntaddr));
    clntaddr.sin_family = AF_INET;
    clntaddr.sin_port = p_addr->sin_port;  
    clntaddr.sin_addr.s_addr = p_addr->sin_addr.s_addr;
    
    int con = connect(sock_cli, (struct sockaddr *)&clntaddr, sizeof(clntaddr));
    
    if (con < 0)
    {
        printf("Connect refusion");
        exit(1);
    }

    return sock_cli;

}


void handle_TYPE(int client_sockfd, char *args){
    //指定FTP的传输模式
    if (strcmp(args, "A") == 0)
    {
        ascii_mode = 1;
        char buf[] = "200 Switching to ASCII mode.\r\n";
        write(client_sockfd, buf, strlen(buf));
    }
    else if (strcmp(args, "I") == 0)
    {
        ascii_mode = 0;
        char buf[] = "200 Switching to Binary mode.\r\n";
        write(client_sockfd, buf, strlen(buf));
    }
    else
    {
        char buf[] = "500 Unrecognised TYPE command.\r\n";
        write(client_sockfd, buf, strlen(buf));
    }

}


int replace(char *str, char *olds, char *news, int max_length)
{
    char *foo, *bar = str;
    int i = 0;
    int str_length, old_length, new_length;
    
    /* do a sanity check */
    if (! str) return 0;
    if (! olds) return 0;
    if (! news) return 0;
    
    old_length = strlen(olds);
    new_length = strlen(news);
    str_length = strlen(str);
    //*bar = str;
    foo = strstr(bar, olds); // 检索子字符串首次出现的位置
    /* keep replacing if there is something to replace and it will no over-flow */
    while ( (foo) && ( (str_length + new_length - old_length) < (max_length - 1) ) ) 
    {
	bar = foo + strlen(news);
	memmove(bar, foo + strlen(olds), strlen(foo + strlen(news)) + 1); // 复制后面
	memcpy(foo, news, strlen(news)); // 填空缺的
	i++;
	foo = strstr(bar, olds);
	str_length = strlen(str);
    }
    return i;
}


void handle_PASV(int client_sockfd){
    
    char reply[100] = {0};
    unsigned long port1, port2;
    char p1[20], p2[20];

    srand(time(NULL));
    port1 = 128 + (rand() % 64);
    port2 = rand() % 0xff;
    sprintf(p1, "%lu", port1);
    sprintf(p2, "%lu", port2);

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY; // 本机地址
    data_addr.sin_port = htons((port1 << 8) + port2);
    data_sock = socket(PF_INET, SOCK_STREAM, 0);
    data_addr_len = sizeof(data_addr);

    if(data_sock==-1){
        perror("socket create failed!\n");
        stpcpy(reply, "500 Cannot connect to the data socket\r\n");
    }
    else if(bind(data_sock, (struct sockaddr*) &data_addr, data_addr_len) < 0){
        perror("bind error!\n");
        stpcpy(reply, "500 Cannot connect to the data socket\r\n");
    }
    else{
        listen(data_sock, 5);					
        stpcpy(reply, "227 Entering Passive Mode (127,0,0,1,");
        strcat(reply, p1);
        strcat(reply, ",");
        strcat(reply, p2);
        strcat(reply,").\r\n");
    }

    write(client_sockfd, reply, strlen(reply));
    isPasv = 1;
    
}

//限速，计算睡眠时间，第一个参数是当前传输的字节数
void limit_rate(int bytes_transfered, int is_upload)
{ 
	// 睡眠时间=（当前传输速度/最大传输速度-1）*当前传输时间;
	long curr_sec = get_time_sec();
	long curr_usec = get_time_usec();
 
	//流过的时间，当前所用的传输时间
	double elapsed;
	elapsed = (double)(curr_sec - bw_transfer_start_sec);
	//秒+微秒
	elapsed += (double)(curr_usec - bw_transfer_start_usec) / (double)1000000;
	if (elapsed <= (double)0) {//等于0的情况有可能，因为传的太快了
		elapsed = (double)0.01;
	}
 
	// 计算当前传输速度，传输的量除以传输时间,忽略了传输速度的小数部分
	unsigned int bw_rate = (unsigned int)((double)bytes_transfered / elapsed);
 
	double rate_ratio;
	//上传
	if (is_upload) {
		//当前速度小于上传速度
		if (bw_rate <= bw_upload_rate_max) {
			// 不需要限速，也需要更新时间
			bw_transfer_start_sec = curr_sec;
			bw_transfer_start_usec = curr_usec;
			return;
		}
 
		//根据公式进行计算
		rate_ratio = bw_rate / bw_upload_rate_max;
	}
	//下载
	else {
		if (bw_rate <= bw_download_rate_max) {
			//不需要限速 
			bw_transfer_start_sec = curr_sec;
			bw_transfer_start_usec = curr_usec;
			return;
		}
 
		rate_ratio = bw_rate / bw_download_rate_max;
	}
 
	//计算睡眠时间
	//睡眠时间=（当前传输速度/最大传输速度-1）*当前传输时间��;
	double pause_time;
	//需要睡眠的时间
	pause_time = (rate_ratio - (double)1) * elapsed;
 
	//进行睡眠
	nano_sleep(pause_time);
 
	//更新时间，下一次要开始传输的时间更新为当前时间
	bw_transfer_start_sec = get_time_sec();
	bw_transfer_start_usec = get_time_usec();
 
}


int get_time_sec()
{
    if(gettimeofday(&tv, NULL) == -1)
        exit(-1);
    return tv.tv_sec;
}

int get_time_usec()
{
    return tv.tv_usec;
}

int nano_sleep(double t)
{
    int sec = (time_t)t;//取整数部分
    int nsec = (t - sec) * 1000000000;
    //int nanosleep(const struct timespec *req, struct timespec *rem);
    struct timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = nsec;

    int ret;
    do
    {//当睡眠被打断时，剩余时间放到ts里面
        ret = nanosleep(&ts, &ts);
    }
    while(ret == -1 && errno == EINTR);

    return ret;
}