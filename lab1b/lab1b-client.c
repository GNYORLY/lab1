//
//  lab1b-client.c
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

const int BUFFERSIZE = 1024;
const int key_len = 7;
const int buffer_len = 16;

struct termios old_settings;
int lFlag = 0;
int eFlag = 0;

int log_fd, sock_fd, key_fd;

const char crlf[2] = {0x0D, 0x0A};

MCRYPT td;

void reset_input_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
    close(sock_fd);
}

void mcrypt_close()
{
    mcrypt_generic_deinit(td);
    mcrypt_module_close(td);
}

void* read_in(void* sock_fd)
{
    int rbyte;
    int logf;
    char logbuffer[BUFFERSIZE + 1024];
    int* t_fd = (int*)sock_fd;
    char buffer[BUFFERSIZE + 1];
    
    while((rbyte = read(*t_fd, buffer, BUFFERSIZE)) > 0)
    {
        if(lFlag)
        {
            logf = sprintf(logbuffer, "RECEIVED %d bytes: %s\n", (int)(rbyte * sizeof(char)), buffer);
            write(log_fd, logbuffer, logf);
        }
        if(eFlag)
        {
            mdecrypt_generic(td, buffer, rbyte);
        }
        write(1, buffer, rbyte);
    }
    return NULL;
}

void read_write(int ifd, int ofd)
{
    char buf[BUFFERSIZE + 1];
    ssize_t rbyte = read(ifd, buf, BUFFERSIZE);
    buf[rbyte] = '\0';
    
    if(rbyte == -1)
    {
        fprintf(stderr, "ERROR: cannot read input\n");
        exit(1);
    }
    
    while(rbyte > 0)
    {
        for(int i = 0; i < rbyte; i++)
        {
            if( buf[i] == '\r' || buf[i] == '\n') //<cr> or <lf>
            {
                write(1, crlf, 2);
                continue;
            }
            write(1, buf, 1);
        }
        
        if(eFlag)
        {
           mcrypt_generic(td, buf, buffer_len);
        }
        
        int loglen = write(ofd, buf, 1);
        
        if(lFlag)
        {
            char logbuffer[BUFFERSIZE + 1024];
            int logf = sprintf(logbuffer, "SENT %d bytes: %s \n", loglen, buf);
            write(ofd, logbuffer, logf);
        }
        
        rbyte = read(ifd, buf, BUFFERSIZE);
    }
}

int main(int argc, char** argv)
{
    struct termios terminal_settings;
    int opt = 0;
    int portNum = 0;
   
    static struct option long_opts[] =
    {
        {"port", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {"encrypt", no_argument, 0, 'e'},
        {0,0,0,0}
    };
    
    while((opt = getopt_long(argc, argv, "p:l:e", long_opts, NULL)) != -1)
    {
        switch(opt)
        {
            case 'p':
                portNum = atoi(optarg);
                break;
            case 'l':
                lFlag = 1;
                char* logfile = optarg;
                if((log_fd = creat(logfile, 0666)) == -1)
                {
                    perror("ERROR: cannot create log file\n");
                    exit(1);
                }
                break;
            case 'e':
                eFlag = 1;
                break;
            default:
                fprintf(stderr, "ERROR: unrecognized argument\nUsage lab1b-client -[e] -[l] filename -[p] portno\n");
        }
    }
    
    if(!isatty(STDIN_FILENO))
    {
        perror("ERROR: invalid terminal\n");
        exit(1);
    }
    
    tcgetattr(STDIN_FILENO, &old_settings);
    atexit(reset_input_mode);
    
    tcgetattr (STDIN_FILENO, &terminal_settings);
    terminal_settings.c_lflag &= ~(ICANON|ECHO);
    terminal_settings.c_cc[VMIN] = 1;
    terminal_settings.c_cc[VTIME] = 0;
    terminal_settings.c_iflag |= ICRNL;
    tcsetattr (STDIN_FILENO, TCSANOW, &terminal_settings);
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("ERROR: cannot open socket\n");
        exit(1);
    }
    
    if((server = gethostbyname("localhost")) == NULL)
    {
        fprintf(stderr,"ERROR: no such host\n");
        exit(1);
    }
   
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
           (char *)server->h_addr,
           server->h_length);
    serv_addr.sin_port = htons(portNum);
    
    if (connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        fprintf(stderr,"ERROR: cannot connect\n");
        exit(1);
    }
   
    if(eFlag)
    {
        char key[128];
        key_fd = open("my.key", O_RDONLY);
        read(key_fd, key, key_len);
        
        td = mcrypt_module_open("twofish", NULL, "cfb", NULL);

        char * IV = malloc(mcrypt_enc_get_iv_size(td));
        memset(IV, 0, sizeof(char) * mcrypt_enc_get_iv_size(td));
        
        mcrypt_generic_init(td, key, key_len, IV);
        atexit(mcrypt_close);
    }
    
    
    pthread_t tid;
    pthread_create(&tid, NULL, read_in, &sock_fd);
    read_write(0, sock_fd);
}
