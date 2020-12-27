//
//  lab1b-server.c
//
//  Created by Jeannie Chiem on 10/16/17.
//  Email: gnyorly@gmail.com
//  ID: 504-666-652
//

#include <stdio.h>
#include <mcrypt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <termios.h>
#include <pthread.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ioctl.h>


const int BUFFERSIZE = 1024;
const int key_len = 7;
const char crlf[2] = {0x0D, 0x0A};

pthread_t threadin;
pid_t pchild;

int pipeto[2];
int pipefr[2];

int eFlag;
int sock_fd;
int key_fd;
int newsock_fd;

char* buf;
MCRYPT c_fd, d_fd;

static void handler(int signum)
{
    perror("sigpipe ");
    if(signum)
        ;
    kill(pchild, SIGINT);
    exit(0);
}

void mcrypt_close()
{
    mcrypt_generic_deinit(c_fd);
    mcrypt_module_close(c_fd);
    
    mcrypt_generic_deinit(d_fd);
    mcrypt_module_close(d_fd);
}

void read_in(int ifd, int ofd, int sock)
{
    buf = (char*)malloc(sizeof(char));
    int offset = 0;
    ssize_t rbyte = read(ifd, buf, 1);
    while(rbyte > 0)
    {
        if(eFlag && sock)
        {
            mdecrypt_generic(d_fd, buf, 1);
        }
        
        if(*(buf + offset) == 0x04) //^D
        {
            break;
        }
        
        if(eFlag && !sock)
        {
            mcrypt_generic(c_fd, buf, 1);
        }
        
        write(ofd, (buf + offset),1);
        offset++;
        rbyte = read(ifd, (buf + offset) ,1);
    }
    
    close(0);
    close(1);
    close(2);
    close(pipeto[1]);
    close(pipefr[0]);
    close(sock_fd);
    kill(pchild, SIGKILL);
    if(!sock)
    {
        pthread_cancel(threadin);
    }
}

void* do_read()
{
    read_in(pipefr[0], 1, 0);
    return NULL;
}

int main(int argc, char** argv)
{
    int opt = 0;
    int portNum = 0;
    int clilen = 0;
    
    static struct option long_opts[] =
    {
        {"port", required_argument, 0, 'p'},
        {"encrypt", no_argument, 0, 'e'},
        {0,0,0,0}
    };
    
    while((opt = getopt_long(argc, argv, "p:e", long_opts, NULL)) != -1)
    {
        switch(opt)
        {
            case 'p':
                portNum = atoi(optarg);
                break;
            case 'e':
                eFlag = 1;
                break;
            default:
                fprintf(stderr, "ERROR: unrecognized argument\nUsage lab1b-server -[e] -[p] portno\n");
                exit(1);
        }
    }
    
    if(eFlag)
    {
     char key[128];
     key_fd = open("my.key", O_RDONLY);
     read(key_fd, key, key_len);
         
     c_fd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
     char * IV = malloc(mcrypt_enc_get_iv_size(c_fd));
     memset(IV, 0, sizeof(char) * mcrypt_enc_get_iv_size(c_fd));
     mcrypt_generic_init(c_fd, key, key_len, IV);
     
     d_fd = mcrypt_module_open("twofish", NULL, "cfb", NULL);
     IV = malloc(mcrypt_enc_get_iv_size(d_fd));
     memset(IV, 0, sizeof(char) * mcrypt_enc_get_iv_size(d_fd));
     mcrypt_generic_init(d_fd, key, key_len, IV);
     
     atexit(mcrypt_close);
    }

    
    struct sockaddr_in serv_addr, cli_addr;
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        fprintf(stderr, "ERROR: cannot open socket\n");
        exit(1);
    }
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portNum);
    if ((bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
    {
        fprintf(stderr,"ERROR: cannot bind socket\n");
        exit(1);
    }
    
    listen(sock_fd,5);
    clilen = sizeof(cli_addr);
    
    newsock_fd = accept(sock_fd, (struct sockaddr*)&cli_addr, (unsigned*)&clilen);
    if (newsock_fd < 0)
    {
        fprintf(stderr, "ERROR: cannot accept socket\n");
        exit(1);
    }
    
    signal(SIGPIPE, handler);
    
    if(pipe(pipeto) == -1)
    {
        perror("ERROR: cannot create pipe1 \n");
        exit(1);
    }
    
    if(pipe(pipefr) == -1)
    {
        perror("ERROR: cannot create pipe2 \n");
        exit(1);
    }
    
    pchild = fork();
    if(pchild < 0)
    {
        perror("ERROR: failed fork \n");
        exit(1);
    }
    
    if(pchild == 0)
    {
        close(pipeto[1]);
        close(pipefr[0]);
        dup2(pipeto[0], 0);
        dup2(pipefr[1], 1);
        dup2(pipefr[1], 2);
        close(pipeto[0]);
        close(pipefr[1]);
        execvp("/bin/bash", NULL);
    }
    else
    {
        close(pipeto[0]);
        close(pipefr[1]);
        pthread_create(&threadin, NULL, do_read, &newsock_fd);
    }
    
    dup2(newsock_fd, 0);
    dup2(newsock_fd, 1);
    dup2(newsock_fd, 2);
    close(newsock_fd);
    
    read_in(0, pipeto[1], 1);
    exit(0);
}






