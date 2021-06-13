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
char* SERVER_IP = "127.0.0.1";
void do_ls(char [],int);
void do_stat(char*,int);
void show_file_info(char* ,struct stat*,int);
void mode_to_letters(int ,char[]);
char* uid_to_name(uid_t);
char* gid_to_name(gid_t);
int delete_sub_str(const char *str, const char *sub_str, char *result_str);

int main(int argc, char** argv){
    int  server_sockfd, client_sockfd;
    struct sockaddr_in  server_addr, client_addr;

    if( (server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT1);
	
    if( bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }

    if( listen(server_sockfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        return 0;
    }
         printf("======waiting for client's request======\n");
		char buffer[BUFFER_MAX];
        socklen_t socket_len = sizeof(client_addr);
    if( (client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &socket_len)) == -1){
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
            exit(1);
    }
    
    printf("Accept client %s on TCP port %d\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
			char Entername[] = "220 please enter username\r\n";
			char Enterpass[] = "331 please enter password\r\n";
			char Wrongpass[] = "530 Login incorrect\r\n";
			char Afterwrong[]= "530 Please login with USER and PASS\r\n"; 
			char Quitout[]   = "221 Goodbye\r\n";
			char Activeport[]= "200 PORT command successful\r\n";
			char LSITCODE[]  = "150 Here comes the directory listing\r\n";
			char listfinish[]= "226 Directory send OK\r\n";
			char get_open_succ[]="150 Openning data connection\r\n";
			char getfinish[] = "226 Transfer complete\r\n";

			int flag_pass = 1;
			
			memset(buffer,0,sizeof(buffer));
	    	send(client_sockfd, Entername, sizeof(Entername), 0);
   			int name_len = recv(client_sockfd, buffer, sizeof(buffer),0);
   			printf("\nReceive username: %s",buffer);
   			
			memset(buffer,0,sizeof(buffer));
   			send(client_sockfd, Enterpass, sizeof(Enterpass), 0);
			int pass_len = recv(client_sockfd,buffer,sizeof(buffer),0);

			pass_len = sizeof(buffer);			
			printf("\nReceive password: %s", buffer);
			if(strncmp(buffer,"PASS 123456",11)!=0){
				memset(buffer,0,sizeof(buffer));
				send(client_sockfd, Wrongpass, sizeof(Wrongpass), 0);
				while(1){
					memset(buffer,0,sizeof(buffer));
					int errorlen = recv(client_sockfd, buffer, sizeof(buffer),0);
					printf("from client:%s\n",buffer);
					if(strncmp(buffer,"QUIT",4)==0){
						send(client_sockfd, Quitout, sizeof(Quitout), 0);
						flag_pass=0;
						break;
					}
					send(client_sockfd, Afterwrong, sizeof(Afterwrong), 0);
				}
			}
			char succ[] ="230 Login in successful\r\n";//need to add check system
			char sysback[19] = "215 UNIX Type: L8\r\n";
			send(client_sockfd, succ , sizeof(succ), 0);
			int sys = recv(client_sockfd, buffer, sizeof(buffer),0);
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
		
		else if(strncmp(buffer,"PORT 127,0,0,1",14)==0 || len<=0){
			char cRes[40960] = {0};	
			int i = 0;
			test(buffer, cRes);	
 			char dst[10][80];
			int num = split(dst, cRes, ","); 
			int one = atoi(dst[3]);
			int two = atoi(dst[4]);
			int data_port=one*256 + two;
			
			send(client_sockfd, Activeport, sizeof(Activeport), 0);
    		memset(buffer,0,sizeof(buffer));
        	int len2 = recv(client_sockfd, buffer, sizeof(buffer),0);
        	printf("from client:%s\n",buffer);
			
        	
			if(strncmp(buffer,"LIST",4)==0){
    		struct sockaddr_in servaddr,mine;
    		int on,ret;
   			int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
			mine.sin_family = AF_INET;
			mine.sin_port = htons(20);  
			mine.sin_addr.s_addr = inet_addr(SERVER_IP);
			int b = bind(sock_cli,(struct sockaddr*)&mine,sizeof(mine));
    		memset(&servaddr, 0, sizeof(servaddr));
    		servaddr.sin_family = AF_INET;
    		servaddr.sin_port = htons(data_port);  
    		servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);  
    		
    		int con =connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) ;
			
   			if (con < 0)
    		{
        		perror("connect");
        		exit(1);
  			 }

			send(client_sockfd, LSITCODE, sizeof(LSITCODE), 0);
			do_ls(".",sock_cli);
			close(sock_cli);
			send(client_sockfd, listfinish, sizeof(listfinish), 0);
			}
			
		    else if(strncmp(buffer,"RETR",4)==0){
    		struct sockaddr_in servaddr,mine;
    		int on,ret;
   			int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
			mine.sin_family = AF_INET;
			mine.sin_port = htons(20);  
			mine.sin_addr.s_addr = inet_addr(SERVER_IP);
			int b = bind(sock_cli,(struct sockaddr*)&mine,sizeof(mine));
    		memset(&servaddr, 0, sizeof(servaddr));
    		servaddr.sin_family = AF_INET;
    		servaddr.sin_port = htons(data_port);  
    		servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);  
    		
    		int con =connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) ;
			
   			if (con < 0)
    		{
        		perror("connect");
        		exit(1);
  			 }

   			char *str1 ="RETR ";
			char *str2 ="STOR ";
			char get_name[10];
			char put_name[10]; 
        	int get_filename = delete_sub_str(buffer, str1, get_name);
			char filehead[16]="./";
			char *cut;
			char cutoff[1]="\r";
			strcat(filehead,get_name);
			cut=strtok(filehead,cutoff);
        	
			FILE *fp = fopen( filehead, "rb" );
			if ( fp == NULL) {
			fprintf(stderr, "Open file error\n");
			exit( 1 );
			}
			send(client_sockfd, get_open_succ, sizeof(get_open_succ), 0);
			memset(buffer,0,sizeof(buffer));
			size_t nreads, nwrites;
			while( nreads = fread( buffer, sizeof(char), sizeof( buffer ), fp) ) {
			if ( ( nwrites = write( sock_cli, buffer , nreads) ) != nreads ) {
				fprintf(stderr, "write error\n");
				fclose( fp );
				close( sock_cli );
				exit( 1 );
			}
		}	
			fclose( fp );
			close( sock_cli );
    		send(client_sockfd, getfinish, sizeof(getfinish), 0);

    		}
			
		}
		else if(strncmp(buffer, "cd", 2)){
			int c;
		    char directory[1000];
		    recv(client_sockfd, directory, sizeof(directory), 0);
		    if(chdir(directory) == 0) {
			    c = 1; }
		    else	{
			    c = 0; }
		    send(client_sockfd, &c, sizeof(int), 0);}//cd
    }
         close(server_sockfd);
         close(client_sockfd);
    return 0;
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

void do_ls(char dirname[],int sockfd)
{
	/*list files in directory called dirname*/
	DIR* dir_ptr;
	struct dirent * direntp; /*each entry*/
	if((dir_ptr = opendir(dirname)) == NULL)
		perror("opendir fails");
	while((direntp = readdir(dir_ptr)) !=NULL)
		do_stat(direntp->d_name, sockfd);
	closedir(dir_ptr);
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

int delete_sub_str(const char *str, const char *sub_str, char *result_str)  
{  
    int count = 0;  
    int str_len = strlen(str);  
    int sub_str_len = strlen(sub_str);  
  
    if (str == NULL)  
    {  
        result_str = NULL;  
        return 0;  
    }  
  
    if (str_len < sub_str_len || sub_str == NULL)  
    {  
        while (*str != '\0')  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
        }  
  
        return 0;  
    }  
  
    while (*str != '\0')  
    {  
        while (*str != *sub_str && *str != '\0')  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
        }  
  
        if (strncmp(str, sub_str, sub_str_len) != 0)  
        {  
            *result_str = *str;  
            str++;  
            result_str++;  
            continue;  
        }  
        else  
        {  
            count++;  
            str += sub_str_len;  
        }  
    }  
  
    *result_str = '\0';  
  
    return count;  
} 
