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
#include <netinet/tcp.h> // struct tcp_info类定义
#include <stdbool.h>

#define PORT1  21
#define PORT2  20
#define MAXLINE 4096
#define BUFFER_MAX 1024
#define SUCCESS 0
#define MAXSIZE 256
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
    //正式开始 
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
			char put_open_succ[]="150 OK to send data\r\n";
			char putfinish[] = "226 Transfer complete\r\n";
			char changeDirs[]="250 Directory successfully changed.\r\n";
	        char changeDirf[]="550 Failed to change directory.\r\n";
	        char deletFiles[]="250 Requested file action okay, completed.\r\n";
	        char deletFilef[]="550 Cannot delete directory.\r\n";
	        char deleteDirf[]="550 Cannot delete directory.\r\n";
	        char replyRNFR[]="350 ready for RNTO\r\n";
	        char renames[]="250 Rename successful!\r\n";
	        char renamef[]="550 Cannot change the name.\r\n";
			
			int flag_pass = 1;
			
			memset(buffer,0,sizeof(buffer));
	    	send(client_sockfd, Entername, sizeof(Entername), 0);//发送，请输入用户名 
   			int name_len = recv(client_sockfd, buffer, sizeof(buffer),0);//接收用户名 
   			printf("\nReceive username: %s",buffer);
   			
			memset(buffer,0,sizeof(buffer));
   			send(client_sockfd, Enterpass, sizeof(Enterpass), 0);
			int pass_len = recv(client_sockfd,buffer,sizeof(buffer),0);//接收密码 

			pass_len = sizeof(buffer);			
			printf("\nReceive password: %s", buffer);
			if(strncmp(buffer,"PASS 111111",11)!=0){//判断密码是否正确
			memset(buffer,0,sizeof(buffer));
   			send(client_sockfd, Wrongpass, sizeof(Wrongpass), 0);
			while(1){//否的话进入循环 
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
				send(client_sockfd, sysback , sizeof(sysback), 0);//发送syst命令的回复 
			}
			

    while(flag_pass){//如果没出错，进到这个while里，直到按quit 
       
        memset(buffer,0,sizeof(buffer));
        int len = recv(client_sockfd, buffer, sizeof(buffer),0);
        printf("from client:%s\n",buffer);
		//退出功能 
        if(strncmp(buffer,"QUIT",4)==0 || len<=0){
			send(client_sockfd, Quitout, sizeof(Quitout), 0);
			break; 
		} //if quit的括号
		//list,get,put功能(现在我知道的需要开启20,tcp的)
		else if(strncmp(buffer,"PORT 127,0,0,1",14)==0 || len<=0){
			//开始截取目标端口号 
			char cRes[40960] = {0};	
			int i = 0;
			test(buffer, cRes);	
 			char dst[10][80];
			int num = split(dst, cRes, ","); 
			int one = atoi(dst[3]);
			int two = atoi(dst[4]);
			int data_port=one*256 + two;
			//截取完毕 
			
			send(client_sockfd, Activeport, sizeof(Activeport), 0);
    		memset(buffer,0,sizeof(buffer));
        	int len2 = recv(client_sockfd, buffer, sizeof(buffer),0);
        	printf("from client:%s\n",buffer);
			
        	
        	//如果命令是ls 
			if(strncmp(buffer,"LIST",4)==0){		
			//开始连接目标端口，自身为20
    		struct sockaddr_in servaddr,mine;
    		int on,ret;
   			int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
   			on = 1;
			ret = setsockopt( sock_cli, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
			mine.sin_family = AF_INET;
			mine.sin_port = htons(20);  
			mine.sin_addr.s_addr = inet_addr(SERVER_IP);
			int b = bind(sock_cli,(struct sockaddr*)&mine,sizeof(mine));
    		if(b==-1)printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
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
			//完毕 
			send(client_sockfd, LSITCODE, sizeof(LSITCODE), 0);
			do_ls(".",sock_cli);
			close(sock_cli);
			send(client_sockfd, listfinish, sizeof(listfinish), 0);
			} //list的括号 
			
	    	//如果命令是get 
		    else if(strncmp(buffer,"RETR",4)==0){
	    		//开始连接目标端口，自身为20
    		struct sockaddr_in servaddr,mine;
    		int on,ret;
   			int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
   			on = 1;
			ret = setsockopt( sock_cli, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
			mine.sin_family = AF_INET;
			mine.sin_port = htons(20);  
			mine.sin_addr.s_addr = inet_addr(SERVER_IP);
			int b = bind(sock_cli,(struct sockaddr*)&mine,sizeof(mine));
    		if(b==-1)printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
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
			//完毕 
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

    		}//get命令的括号 
			
			
			//如果命令是put
			else if(strncmp(buffer,"STOR",4)==0){
				//开始连接目标端口，自身为20
    		struct sockaddr_in servaddr,mine;
    		int on,ret;
   			int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
   			on = 1;
			ret = setsockopt( sock_cli, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );
			mine.sin_family = AF_INET;
			mine.sin_port = htons(20);  
			mine.sin_addr.s_addr = inet_addr(SERVER_IP);
			int b = bind(sock_cli,(struct sockaddr*)&mine,sizeof(mine));
    		if(b==-1)printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
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
			//完毕 
			char put_open_succ[]="150 OK to send data\r\n";
			char putfinish[] = "226 Transfer complete\r\n";
			char *str2 ="STOR ";
			char put_name[10]; 
        	int put_filename = delete_sub_str(buffer, str2, put_name);
			char filehead[16]="";
			char *cut;
			char cutoff[1]="\r";
			strcat(filehead,put_name);
			cut=strtok(filehead,cutoff);
			printf("%s\nd",filehead);
			FILE *save_fp = fopen(filehead, "w"); 
  			
  			send(client_sockfd, put_open_succ, sizeof(put_open_succ), 0);
  			memset(buffer,0,sizeof(buffer));
 			ssize_t length = read( sock_cli, buffer, sizeof(buffer) );  
				if( length == -1 ) {
					fprintf(stderr, "Read Error:%s\n", strerror( errno ));
					exit(1);
				}
				else if ( length > 0){
					fwrite(buffer, sizeof(char), length, save_fp);
				}
				fclose( save_fp );
			close( sock_cli );
  			send(client_sockfd, putfinish, sizeof(putfinish), 0);
			}//put的结尾 
			
		}//需要20端口的命令合集的(else if)括号
		else if(strncmp(buffer,"PWD",3)==0){
        char *workDir = NULL;//To make getcwd call malloc to allocate workDir dynamically, we set workDir to null and size to zero
        char workDiroutput[MAXSIZE];
        memset(workDiroutput, 0, MAXSIZE);
        workDir = getcwd(NULL,0);
        strcat(workDiroutput,"257 \"");		//5 characters
        strcat(workDiroutput,workDir);		//strlen(workDir)
        strcat(workDiroutput,"\"\n");		//2 characters + 0 byte
        send(client_sockfd, workDiroutput, sizeof(workDiroutput), 0);
        free(workDir);//Release buffer to prevent memory leakage
        }
        
        else if(strncmp(buffer,"CWD",3)==0){
	    int a = 4;
        int b = 0;
        char changeworkingDir[MAXSIZE];
        memset(changeworkingDir,0,MAXSIZE);
        while (a<(strlen(buffer)-2)) //buf[0-4]="USER"
            changeworkingDir[b++] = buffer[a++]; 
        printf("%s\n",changeworkingDir);
	    if(chdir(changeworkingDir)==0)
		    send(client_sockfd, changeDirs, sizeof(changeDirs), 0);
        else
      	    send(client_sockfd, changeDirf, sizeof(changeDirf), 0);
        }
        
        else if(strncmp(buffer,"DELE",4)==0){
	    char filename[MAXSIZE];
	    int a = 5;
        int b = 0;
        memset(filename,0,MAXSIZE);
        while (a<(strlen(buffer)-2)) //buf[0-4]="USER"
        filename[b++] = buffer[a++];
        strcat(filename,"\0");
        printf("%s\n",filename);
	    if(unlink(filename)==-1){
		perror("error\n");
     	send(client_sockfd, deletFilef, sizeof(deletFilef), 0);
        }else
   		send(client_sockfd, deletFiles, sizeof(deletFiles), 0);
        }
        
        
        else if(strncmp(buffer,"MKD",3)==0){;
	    char cwd[MAXSIZE];
	    char dirName[MAXSIZE];
        char mkdResult[2*MAXSIZE+32]; //Increasing buffer size to accomodate all usecases, doubling and adding 32 extra characters for the sprintf
        memset(cwd,0,MAXSIZE);
        memset(mkdResult,0,2*MAXSIZE+32);
        getcwd(cwd,MAXSIZE);
        
	    int a = 4;
        int b = 0;
        memset(dirName,0,MAXSIZE);
        while (a<(strlen(buffer)-2)) //buf[0-4]="USER"
        dirName[b++] = buffer[a++];
        /* TODO: check if directory already exists with chdir? */

        /* Absolute path */
        if(dirName[0]=='/'){
        if(mkdir(dirName,S_IRWXU)==0){
        strcat(mkdResult,"257 \"");
        strcat(mkdResult,dirName);
        strcat(mkdResult,"\" new directory created.\n");
        send(client_sockfd, mkdResult, sizeof(mkdResult), 0);
        }else
        send(client_sockfd, deleteDirf, sizeof(deleteDirf), 0);
        }
        /* Relative path */
        else{
        if(mkdir(dirName,S_IRWXU)==0){
        sprintf(mkdResult,"257 \"%s/%s\" new directory created.\n",cwd,dirName); //32 additional characters
        send(client_sockfd, mkdResult, sizeof(mkdResult), 0);
        }else
        send(client_sockfd, deleteDirf, sizeof(deleteDirf), 0);     }
        }
        
        else if(strncmp(buffer,"RNFR",4)==0){
	    char oldName[MAXSIZE];
	    char newName[MAXSIZE];
	    char newNameInput[MAXSIZE];
	    int a = 5;
        int b = 0;
        memset(oldName,0,MAXSIZE);
        while (a<(strlen(buffer)-2)) //buf[0-4]="USER"
        oldName[b++] = buffer[a++];
        memset(newNameInput,0,MAXSIZE);
	    send(client_sockfd, replyRNFR, sizeof(replyRNFR), 0);
	    recv(client_sockfd, newNameInput, sizeof(newNameInput), 0);
	
	    a = 5;
        b = 0;
 	    memset(newName,0,MAXSIZE);
        while (a<(strlen(newNameInput)-2)) //buf[0-4]="USER"
        newName[b++] = newNameInput[a++];    
	    if(rename(oldName,newName)<0)
		send(client_sockfd, renamef, sizeof(renamef), 0);
	    else
		send(client_sockfd, renames, sizeof(renames), 0);
        }
	
    }//while(flag_pass)的括号 
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

int split(char dst[][80], char* str, const char* spl)//分割字符串 
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
