# Simple-Ftp-Server
The requirements are stated in Project-FTP server-20210513.pdf.
We need to use vsftpd as FTP client and use our program as FTP server.

backup directory is the version control in our coding process.
The Final Version is ftp.c file

## How to Use
1. Install vsftpd following requirement file.
2. Close vsftpd by `sudo service vsftpd close`
3. Compile ftp.c by `gcc -o ftp ftp.c`
4. Run by `sudo ./ftp` **(Must be root)**
5. Connect to server through client by `ftp 127.0.0.1`
6. Run corresponding command such as `user, ls, cd, mkdir, del, rename`

## Summary
I am satisfied and confident in our code. The command is well implemented, and it's a simple ftp server
as we wished not a complex one like https://github.com/dasima/ftpServer. But it can run correctly in my PC
and implements many functions than ours.

There is not Passive mode in our `list` command, it will enter exception and exit.
