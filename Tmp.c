
static int ftp_get_upload_file_name(const char *upload_file, char *file_name)
{
    int i = 0;
    int path_lenth = 0;
 
    if (upload_file == NULL || file_name == NULL)
    {
        return -1;
    }
 
    path_lenth = strlen(upload_file);
 
    while (path_lenth - i > 0)
    {
        // find index of '/'
        if (upload_file[path_lenth - i]== 47)
        {
            i--;
            break;
        }
        i++;
    }
 
    strcpy(file_name, &upload_file[path_lenth - i]);
 
 
    return 0;
}
 
int ftp_upload(const char *ip, unsigned int port, const char *user, const char *pwd, const char *upload_file,const char *upload_name)
{
	int ret;
	int size;
	char buff[MAX_LEN];
	char cmd[MAX_CMD_LEN];
    char file_name[256];
	int fd_socket, fd_data;
	struct sockaddr_in addr;
	struct sockaddr_in data;
	int send_ret=0;
 
	addr.sin_family = AF_INET;
	inet_aton(ip, &addr.sin_addr);
	addr.sin_port   = htons(port);
	data.sin_family = AF_INET;
	inet_aton(ip, &data.sin_addr);
 
	fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket == -1)
    {
        return -1;
    }
 
	fd_data = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_data == -1)
    {
		close(fd_socket);
        return -1;
    }
 
	ret = connect(fd_socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0)
    {
		close(fd_data);
		close(fd_socket);
        printf("connet falied\n");
        return -1;
    }
    
	size = recv(fd_socket, buff, MAX_LEN-1, 0);
	buff[size] = 0;
 
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "USER %s\r\n", user);
    // ftp_socket_send(fd_socket, "PASS shikejun\r\n");
	ftp_socket_send(fd_socket, cmd);
	ftp_socket_recv(fd_socket, buff);
 
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "PASS %s\r\n", pwd);
	ftp_socket_send(fd_socket, cmd);
	ftp_socket_recv(fd_socket, buff);
 
	ftp_socket_send(fd_socket, "SYST\r\n");
	ftp_socket_recv(fd_socket, buff);
 
	ftp_socket_send(fd_socket, "TYPE I\r\n");
	ftp_socket_recv(fd_socket, buff);
 
	ftp_socket_send(fd_socket, "PASV\r\n");
	ftp_socket_recv(fd_socket, buff);
 
	ftp_get_data_port(buff, &data.sin_port);
 
    memset(file_name, 0, sizeof(file_name));
	if(upload_name==NULL||strlen(upload_name)<=1)
	{
    	ftp_get_upload_file_name(upload_file, file_name);
	}
	else
	{
		strcpy(file_name,upload_name);
	}
    memset(cmd, 0, sizeof(cmd));
    sprintf(cmd, "STOR %s\r\n", file_name);
	ftp_socket_send(fd_socket, cmd);
 
	ret = connect(fd_data, (struct sockaddr *)&data, sizeof(data));
    if (ret != 0)
    {
        printf("connet falied\n");
		close(fd_data);
		close(fd_socket);
        return -1;
    }
 
	ftp_socket_recv(fd_socket, buff);
 
    int fd = open(upload_file, O_RDONLY);
    if (fd == -1)
    {
        printf("open: \n");
		close(fd_data);
		close(fd_socket);
        return -1;
    }
 
    while ((ret = read(fd, buff, sizeof(buff))) > 0)
    {
        send_ret = send(fd_data, buff, ret, 0);
        if(send_ret<=0)
        {
        	break;
        }
        usleep(30*1000);
    }
    
    close(fd);
	close(fd_data);
 
	ftp_socket_recv(fd_socket, buff);
 
	close(fd_socket);
	
	printf("ftp_upload [%s] [%s] result: %s\n",upload_file,upload_name,buff);
 
	return 0;
