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
#include<time.h>
#include<ctype.h>
#include<fcntl.h>
#include <arpa/inet.h>
#include <shadow.h> 

#define MAX_INPUT_SIZE 1024
#define SERVER_PORT 21
#define DATA_PORT 20
#define FILEBUF_SIZE 1024
#define TYPE_I 0
#define TYPE_A 1

int TYPE,login,pasvstate,z;
int server_sock, client_sock, data_sock;
int server_adr_len;
int i = 0;

socklen_t client_addr_len,data_addr_len;

const char *MODE[] = {"BINARY","ASCII"};//����ģʽ����
char buffer[FILEBUF_SIZE];
char username[25];
char password[25];

struct sockaddr_in server_addr, client_addr, data_addr;
struct hostent *data_host;

int replace(char *str, char *olds, char *news, int max_length)//��by����what
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
    foo = strstr(bar, olds);//���ش�whatλ�õ���β��bar
    /* keep replacing if there is somethign to replace and it
    will no over-flow
    */
    while ( (foo) && ( (str_length + new_length - old_length) < (max_length - 1) ) ) 
    {
	bar = foo + strlen(news);
	memmove(bar, foo + strlen(olds), strlen(foo + strlen(news)) + 1);
	memcpy(foo, news, strlen(news));//by����foo
	i++;
	foo = strstr(bar, olds);
	str_length = strlen(str);
    }
    return i;
}

int main(int argc, char **argv)
{
	memset(username, 0, sizeof(username));
	memset(password, 0, sizeof(password));
	login=0;
    printf("\nWelcome to wangmeng's ftp server!\n");
    
    //get the server socket
    server_sock = socket(PF_INET, SOCK_STREAM, 0);//����socket ָ��ipv4
    if(server_sock == -1)	//create socket failed
    {
		perror("socket failed!\n");
		exit(1);
    }
    else printf("Socket created!\n");
    
    //configure server address,port
    memset(&server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    //server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if(server_addr.sin_addr.s_addr == INADDR_NONE)
    {
		perror("INADDR_NONE\n");
		exit(1);
    }
    
    server_adr_len = sizeof server_addr;
    
    //bind
    z = bind(server_sock, (struct sockaddr *)&server_addr, server_adr_len);
    if(z == -1)
    {
		perror("bind failed!\n");
		exit(1);
    }
    else printf("Bind Ok!\n");
    
    //listen
    z = listen(server_sock, 5);
    if(z < 0)
    {
		perror("Server Listen Failed!\n");
		exit(1);
    }
    else printf("listening...\n");   

    //loop and wait for connection
    while(1)
    {
		TYPE = TYPE_I;
	//struct sockaddr_in client_addr;
		client_addr_len = sizeof(client_addr);
		i = 0;
		printf("wait for accept...\n");
	//accept an request
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
		if(client_sock < 0)
		{
	    	perror("Server Accept Failed!\n");
	    	exit(1);
		}
		else printf("Server Accept Succeed!New socket %d\n",client_sock);

		printf("connected to the server...\n");
		char reply[100] = "220 FTP server ready(myftpserver)\r\n";
		write(client_sock, reply, strlen(reply));
	
		printf("waiting for the username.....\n");
	
		while(1)
		{
	    
	    //deal with commands
	    	z = read(client_sock, buffer, sizeof(buffer));
	    	buffer[z-2] = '\0';
	    	printf("z = %d, buffer is '%s'\n",z,buffer);
	    
	    	char command3[5];
	    	char command4[6];
 			strncpy(command3, buffer, 3);
	    	command3[3] = '\0';
	    	strncpy(command4, buffer, 4);
	    	command4[4] = '\0';
	    	char fileoldname[255];
	    
	    	if(strcmp(command4, "USER") == 0)//USER
	    	{
				stpcpy(username, &buffer[5]);
				printf("username:%s\n",username);
				stpcpy(reply, "331 Please spicify the password.\r\n");
				write(client_sock, reply, strlen(reply));
				printf("waiting for the password....\n");
	    	}
	    	else if(strcmp(command4, "PASS") == 0)//PASS
	    	{
				stpcpy(password, &buffer[5]);
				printf("password:%s\n",password);
				struct spwd *sp;
				sp = getspnam(username);
				if(sp==NULL){
					stpcpy(reply, "530 Login incorrect.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("login failed because of password!\n");
					login=0;
				} 
				else if(strcmp(sp->sp_pwdp, (char*)crypt(password, sp->sp_pwdp))==0)
				{
					stpcpy(reply, "230 Login successful.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("login successful!\n");
					login=1;
				}
				else
				{
					stpcpy(reply, "530 Login incorrect.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("login failed because of password!\n");
					login=0;
				}

		    }
		    else if(strcmp(command4, "SYST") == 0)//SYST
	    	{
    			if(login==1)
				{
					stpcpy(reply, "215 Remote system type is UNIX.Using binary mode to transfer files.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("syst command successful!\n");
					pasvstate=0;
	    		}
	    		else
				{
	    			stpcpy(reply, "530 Please login with USER and PASS.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("no login!\n");
    			}
	    	}
	    	else if(strcmp(command4, "QUIT") == 0)//QUIT
	    	{
				stpcpy(reply, "221 Goodbye.\r\n");
				write(client_sock, reply, strlen(reply));
				printf("quit complete!\n");
				break;
	    	}
	    	else if(login==1)
			{

			   if(strcmp(command3, "PWD") == 0)//PWD
	    		{
	    			char pathofthis[255];
	    
	    			stpcpy(reply, "257 ");
   					getcwd(pathofthis,sizeof(pathofthis)-1);//��ȡ��ǰ·��
    				strcat(reply,pathofthis);
   					strcat(reply,"\r\n");
					write(client_sock, reply, strlen(reply));
					printf("PWD command successful!\n");
	    		}
	    		else if(strcmp(command3, "CWD") == 0)//CWD
	    		{
		
  					int success=chdir(&buffer[4]);
  					if(success==0)
	  				{
						stpcpy(reply, "250 Directory successfully changed.\r\n");
	  					printf("cwd success!\n");
  					}
	 				else
	 				{
 						stpcpy(reply, "550 Failed to change directory.\r\n");
						printf("cwd failed.\n");
 					}	
					write(client_sock, reply, strlen(reply));
	    		}
	    		else if(strcmp(command4, "PORT") == 0)//PORT
	    		{
				//stpcpy(reply, "200 PORT command successful\r\n");
					unsigned long addr1, addr2, addr3, addr4, port1, port2, addr;
				    sscanf(&buffer[5], "%lu,%lu,%lu,%lu,%lu,%lu", &addr1, &addr2, &addr3, &addr4, &port1, &port2);
    				addr = htonl((addr1 << 24) + (addr2 << 16) + (addr3 << 8) + addr4);
    				if(addr != client_addr.sin_addr.s_addr)
    				{
						stpcpy(reply, "500 Wrong client address.\r\n");
						printf("wrong address!\n");
    				}
    				else
					{
    				 //setup the port for the data connection
    					memset(&data_addr, 0, sizeof(data_addr));//�����׽��ֲ��� IPV4 ��ַ �˿ں�
    					data_addr.sin_family = AF_INET;
    					data_addr.sin_addr.s_addr = addr;
    					data_addr.sin_port = htons((port1 << 8) + port2);
    
    					//get the data cocket
    					data_sock = socket(PF_INET, SOCK_STREAM, 0);
    					if(data_sock == -1)
    					{
							perror("data_sock failed!\n");
							stpcpy(reply, "500 Cannot get the data socket\r\n");
    					}
    					//connect to the client data socket
    					else
						{
							if(connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0)//���ӷ�����
    						{
								perror("connect failed!\n");
								stpcpy(reply, "500 Cannot connect to the data socket\r\n");
    						}
    						else
    						{
								stpcpy(reply, "200 PORT command successful.Consider using PASV.\r\n");
								printf("PORT success!\n");
    						}
    					}				
	    			}
	    			write(client_sock, reply, strlen(reply));
	    		}
	    		else if(strcmp(command4,"PASV") == 0) {
	    			
	    			unsigned long port1,port2;
	    			char p1[20],p2[20];
	    		    srand(time(NULL));
    				port1=128 + (rand() % 64);
    				port2=rand() % 0xff;
    				sprintf(p1,"%lu",port1);
    				sprintf(p2,"%lu",port2);
					memset(&data_addr, 0, sizeof(data_addr));//�����׽��ֲ��� IPV4 ��ַ �˿ں�
					data_addr.sin_family = AF_INET;
					data_addr.sin_addr.s_addr = INADDR_ANY;
					data_addr.sin_port = htons((port1 << 8) + port2);
					data_sock = socket(PF_INET, SOCK_STREAM, 0);
					data_addr_len=sizeof(data_addr);
					
					if(data_sock==-1){
        				perror("socket create failed!\n");
        				stpcpy(reply, "500 Cannot connect to the data socket\r\n");
    				}
		   			else if(bind(data_sock,(struct sockaddr*) &data_addr, data_addr_len) < 0){
   						perror("bind error!\n");
   						stpcpy(reply, "500 Cannot connect to the data socket\r\n");
    				}
					else{
						listen(data_sock,5);					
						stpcpy(reply, "227 Entering Passive Mode (127,0,0,1,");
						strcat(reply,p1);
						strcat(reply,",");
						strcat(reply,p2);
						strcat(reply,").\r\n");
					}
 
					write(client_sock, reply, strlen(reply));
					pasvstate=1;
		    	}
  				else if(strcmp(command4, "LIST") == 0)//LIST
			    {
					stpcpy(reply, "150 Here comes the directory listing.\r\n");
					write(client_sock, reply, strlen(reply));
					printf("begin list...\n");
			
					FILE *listopen;
    				char databuf[PIPE_BUF];
				 	int n;
    
    				listopen = popen("ls -l","r");//ϵͳִ��ls -l��д���ļ���
    				if(listopen == 0)
    				{
						perror("ls stream open failed!\n");
						stpcpy(reply, "500 Transfer stream error\r\n");
						printf("stream error!\n");
						close(data_sock);
    				}
    				else
					{
    	    			memset(databuf, 0, PIPE_BUF - 1);
    					while((n = read(fileno(listopen), databuf, PIPE_BUF)) > 0)//һֱ��ȡ�ļ��� ����ӡ ����Ϊls -l������
    					{	
							replace(databuf, "\n", "\r\n", PIPE_BUF-1);
							printf("%s\n", databuf);
							write(data_sock, databuf, strlen(databuf));
							memset(databuf, 0, PIPE_BUF - 1);
    					}
  		 				memset(databuf, 0, PIPE_BUF - 1);
    					if(pclose(listopen) != 0)
    					{
							perror("Non-zero return value from \"ls -l\"\n");
							printf("stream close error!\n");
    					}
    					close(data_sock);
    					stpcpy(reply, "226 Directory send OK.\r\n");
    					printf("list complete!\n");
    				}
					write(client_sock, reply, strlen(reply));

			    }
	  	  		else if(strcmp(command4, "TYPE") == 0)//TYPE
	    		{
    			 	if(strcmp(&buffer[5],"A")==0)//�жϴ���ģʽ Ĭ���Ƕ�����
   				 	{
			    			TYPE = TYPE_A;//ascii����
			    			stpcpy(reply, "200 Switching to ASCII mode.\r\n");
			    	}
			    	else if(strcmp(&buffer[5],"I")==0)
			    	{
			    			TYPE = TYPE_I;//�����ƴ���
			    			stpcpy(reply, "200 Switching to Binary mode.\r\n");
			    	}
					else
					{
	    					TYPE = TYPE_I;
	    					stpcpy(reply, "200 Switching to Binary mode.\r\n");
 					}
	    	
					write(client_sock, reply, strlen(reply));
					printf("type set ok.\n");
	    		}
	    		else if(strcmp(command4, "RETR") == 0)//RETR
	    		{

					if(pasvstate==0){
						printf("Connecting to the file....\n");
		
						FILE *upfile;
    					unsigned char databuff[FILEBUF_SIZE] = "";
    					int bytes;
    
    					upfile = fopen(&buffer[5],"r");
    					if(upfile == 0)
    					{
							perror("file open failed.\n");
							stpcpy(reply, "550 Failed to open file.\r\n");
	
					    }
    					else
						{
							sprintf(reply, "150 Opening %s mode data connection for %s (0 bytes)\r\n", MODE[TYPE], &buffer[5]);
							write(client_sock, reply, strlen(reply));
    						while((bytes = read(fileno(upfile), databuff, FILEBUF_SIZE)) > 0)//��ȡ�ļ����ļ���
   	 						{
								if(TYPE == TYPE_A)
								{
	    							replace((char *)databuff, "\r\n", "\n", FILEBUF_SIZE-1);
	    							replace((char *)databuff, "\n", "\r\n", FILEBUF_SIZE-1);
	    							write(data_sock, (const char *)databuff, strlen((const char *)databuff));//д���ļ����ݵ�����
								}
								else if(TYPE == TYPE_I)
								{
	    							write(data_sock, (const char *)databuff, bytes);
								}
								memset(&databuff, 0, FILEBUF_SIZE);
    						}
    						memset(&databuff, 0, FILEBUF_SIZE);
    	
    						fclose(upfile);
    	    				stpcpy(reply, "226 Transfer complete\r\n");
    	    				printf("file transfer complete!\n");
    					}
    
    					close(data_sock);
						write(client_sock, reply, strlen(reply));
					}
					else{
					
						int connection;
    					int fd;
    					struct stat stat_buf;
    					off_t offset = 0;
    					int sent_total = 0;
	
    			    /* Passive mode */
    	    			if(access(&buffer[5],R_OK)==0 && (fd = open(&buffer[5],O_RDONLY))){
    	            		fstat(fd,&stat_buf);
	
	        				stpcpy(reply, "150 Opening BINARY mode data connection for");
	        				strcat(reply, &buffer[5]);
	        				strcat(reply, ".\r\n");
	        				printf("file transfer begin!\n");
	        				write(client_sock, reply, strlen(reply));
	                
	    	        	    connection=accept(data_sock,(struct sockaddr*) &client_addr,&client_addr_len);
	                	
        	        		close(data_sock);
            		    	if(sent_total = sendfile(connection, fd, &offset, stat_buf.st_size)){
	
			                    if(sent_total != stat_buf.st_size){
        		                	perror("sendfile error!\n");
            			        }
								else{
									stpcpy(reply, "226 File has been downloaded OK.\r\n");
        							printf("file transfer complete!\n");
        							write(client_sock, reply, strlen(reply));
								}
                			}
							else{
                				stpcpy(reply, "550 Failed to read file.\r\n");
 								printf("Failed to get file!\n");
        						write(client_sock, reply, strlen(reply));
                			}
            			}
						else{
            				stpcpy(reply, "550 Failed to get file\r\n");
        					printf("Failed to get file!\n");
        					write(client_sock, reply, strlen(reply));
            			}
    					close(fd);
    					close(connection);
					}
				}
	   	 		else if(strcmp(command4, "STOR") == 0)//STOR
	    		{	
	    			if(pasvstate==0) {
	    				FILE *downfile;
    					unsigned char databuff[FILEBUF_SIZE];
    					int bytes = 0;
	
					    downfile = fopen(&buffer[5], "w");
	    				if(downfile == 0)
	    				{
							perror("file open failed!\n");
							stpcpy(reply, "450 Cannot create the file\r\n");
							close(data_sock);
	    				}
						else
						{
							sprintf(reply, "150 Opening %s mode data connection for %s (0 bytes)\r\n", MODE[TYPE], &buffer[5]);
							write(client_sock, reply, strlen(reply));
							printf("file opened!\n"); 
	    					while((bytes = read(data_sock, databuff, FILEBUF_SIZE)) > 0)
	    					{
								write(fileno(downfile), databuff, bytes);//д���������ݵ��ļ���
	    					}
	    
	    					fclose(downfile);
	    					close(data_sock);
	    					stpcpy(reply, "226 Transfer complete\r\n");
	    					printf("file transfer complete!\n");
						}
						write(client_sock, reply, strlen(reply));
	    			}
	    			else{
			    		int connection, fd;
    					int pipefd[2];
    					int res;
    					const int buff_size = 8192;

    					FILE *fp = fopen(&buffer[5],"w");
	
    					if(fp==NULL){
        				/* TODO: write status message here! */
        					perror("file open failed!\n");
							stpcpy(reply, "550 Failed to create file.\r\n");
 							write(client_sock, reply, strlen(reply));
    					}
        				/* Passive mode */
        				else{
            				fd = fileno(fp);
            				connection = accept(data_sock,(struct sockaddr*) &client_addr,&client_addr_len);;
            				close(data_sock);
            				if(pipe(pipefd)==-1)
							{
								perror("pipe create failed!\n");
   								stpcpy(reply, "550 Failed to open pipe.\r\n");
        						write(client_sock, reply, strlen(reply));
            				}
							else
							{
        				    	stpcpy(reply, "150 Ok to send data.\r\n");
        				    	write(client_sock, reply, strlen(reply));
        				    	printf("Begin transfering data.....\n");

				            /* Using splice function for file receiving.
        				    * The splice() system call first appeared in Linux 2.6.17.
            				*/

					            while ((res = splice(connection, 0, pipefd[1], NULL, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE))>0)
								{
    	            				splice(pipefd[0], NULL, fd, 0, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE);
        	    				}

				            /* TODO: signal with ABOR command to exit */

				            /* Internal error */
	            				if(res==-1){
    	        				    perror("splice file failed!\n");
									stpcpy(reply, "550 Failed to open pipe.\r\n");
        							write(client_sock, reply, strlen(reply));
            					}
								else{
                					stpcpy(reply, "226 Transfer complete.\r\n");
                					write(client_sock, reply, strlen(reply));
                					printf("Transfer success!\n");
            					}
							}
            				close(connection);
            				close(fd);
        				}
    				}
	    		}
				else if(strcmp(command4, "DELE") == 0)
				{
				//stpcpy(reply, "del file!\r\n");
					FILE *outfile;
    				unsigned char databuf[FILEBUF_SIZE];
    				struct stat sbuf;
    				int bytes = 0;
    
    				if(stat(&buffer[5], &sbuf) == -1)
    				{
						stpcpy(reply, "550 Delete operation failed.\r\n");
					
						printf("File not exist!\n");
    				}
					else
					{
						char rm_cmd[30]="rm -rf ";
						strcat(rm_cmd,&buffer[5]);
						printf("%s\n",rm_cmd);
						system(rm_cmd);
						strcpy(reply,"213 Delete complete!\r\n");
						printf("delete complete!\n");
					}
					write(client_sock, reply, strlen(reply));
	    		}
				else if(strcmp(command4, "RNFR") == 0)			//RENAME
				{
					strcpy(fileoldname,buffer+5);
					struct stat sbuf;
   					if(stat(&buffer[5], &sbuf) == -1)
    					{
							stpcpy(reply, "550 Failed to rename.\r\n");
							write(client_sock, reply, strlen(reply));
							printf("File not exist.\n");
    					}
    				else{
        				strcpy(reply,"350 Ready for RNTO.\r\n");
        				write(client_sock, reply, strlen(reply));
    				}			
				}
				else if(strcmp(command4, "RNTO") == 0){
					struct stat sbuf;
					if(stat(&buffer[5], &sbuf) == -1)
					{
						   
    					if(rename(fileoldname,&buffer[5])==0)
						{
        					strcpy(reply, "250 Rename successful.\n");
        					write(client_sock, reply, strlen(reply));
							printf("rename complete!\n");
    					}
						else
						{
        					strcpy(reply,"550 Failed to rename\n");
        					write(client_sock, reply, strlen(reply));
							printf("rename failed!\n");
 						}
  					}
  					else{
  							stpcpy(reply, "550 File exist.\r\n");
							write(client_sock, reply, strlen(reply));
							printf("File name exist.\n");
				  	}	
					
				}
				else if(strcmp(command3, "MKD") == 0)
				{
					FILE *createfile;
    				unsigned char databuf[FILEBUF_SIZE];
    				struct stat sbuf;
    				int bytes = 0;
    
    				if(stat(&buffer[4], &sbuf) != -1)
    				{
						stpcpy(reply, "550 Create directory operation failed!\r\n");
						//return;
						printf("create failed!\n");
    				}
					else
					{
						char mkdir_cmd[30]="mkdir ";
						strcat(mkdir_cmd,&buffer[4]);
						printf("%s\n",mkdir_cmd);
						system(mkdir_cmd);
						strcpy(reply,"212 Mkdir complete!\r\n");
						printf("Mkdir complete!\n");
					}			
					write(client_sock, reply, strlen(reply));		
				}
	    	}
			else
			{
	    		stpcpy(reply, "530 Please login with USER and PASS.\r\n");
				write(client_sock, reply, strlen(reply));
				printf("no login!\n");
    		}

		}
	
	close(client_sock);
	printf("\n");
    }
    close(server_sock);
    return 0;
}







