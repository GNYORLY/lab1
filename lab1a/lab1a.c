//
//  lab1a.c
//
//  Created by Jeannie Chiem on 10/09/17.
//  Copyright © 2017 Jeannie Chiem. All rights reserved.
//  Email: gnyorly@gmail.com
//  ID: 504-666-652
//

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

pthread_t threadin;
pid_t pchild;

int s_flag = 0; //flag to indicate the --shell command was used

int pipeto[2];
int pipefr[2];

char* buf;
char crlf[2] = {0x0D, 0x0A};

struct termios old_settings; //used to save old terminal settings

void sighandler(int sig)
{
    if(s_flag)
        if(sig == SIGINT)
        {
            kill(pchild, SIGINT);
        }
    if(sig == SIGPIPE)
    {
        exit(1);
    }
}

void reset_input_mode()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
}

void read_in(int ifd, int ofd, int isthread)
{
    buf = (char*)malloc(sizeof(char));
    int offset = 0;
    ssize_t rbyte = read(ifd, buf, 1);
    while(rbyte)
    {
        if(*(buf + offset) == 0x04) //^D
        {
            if(s_flag)
            {
                close(pipeto[1]);
                close(pipefr[0]);
                close(pipeto[0]);
                close(pipefr[1]);
                pthread_cancel(threadin);
                kill(pchild, SIGHUP);
                exit(0);
            }
            exit(0);
        }
        
        if(*(buf + offset) == '\r' || *(buf + offset) == '\n') //<cr> or <lf>
        {
            if(!s_flag)
            {
                write(ofd, crlf, 2);
                if(isthread)
                {
                    write(pipeto[1], crlf, 2);
                }
                offset++;
                continue;
            }
        }
        write(ofd, (buf + offset),1);
        if(isthread)
        {
            write(pipeto[1], (buf + offset) , 1);
        }
        offset++;
        rbyte = read(ifd, (buf + offset) ,1);
    }
}

void* do_read()
{
    read_in(pipefr[0], 1, 0);
    exit(0);
}

int main(int argc, char** argv)
{
    struct termios terminal_settings;
    
    int opt = 0;
    static struct option long_options[] =
    {
        {"shell", no_argument, 0, 's'},
        {0,0,0,0}
    };
    
   while((opt = getopt_long(argc,argv,"s",long_options,NULL)) != -1)
    {
        switch(opt)
        {
            case 's':
                s_flag = 1;
                break;
            default:
                break;
        }
    }
    
    if(!isatty(STDIN_FILENO)) //checks if stdin is associated with an open terminal device
    {
        perror("ERROR: invalid terminal ");
        exit(EXIT_FAILURE);
    }
    
    tcgetattr(STDIN_FILENO, &old_settings); //saves current terminal settings for restoration
    atexit(reset_input_mode);
    
    tcgetattr (STDIN_FILENO, &terminal_settings);
    terminal_settings.c_lflag &= ~(ICANON|ECHO); //clears ICANON and ECHO for non-canonical no-echo mode
    terminal_settings.c_cc[VMIN] = 1; //blocks read until 1 character arrives
    terminal_settings.c_cc[VTIME] = 0; //disables inter-character timer
    terminal_settings.c_iflag |= ICRNL; //compounds carriage return and linefeed when either is used
    tcsetattr (STDIN_FILENO, TCSANOW, &terminal_settings);
    
    if(pipe(pipeto) == -1)
    {
        perror("ERROR: pipe1 ");
        exit(EXIT_FAILURE);
    }
    
    if(pipe(pipefr) == -1)
    {
        perror("ERROR: pipe2 ");
        exit(EXIT_FAILURE);
    }
    
    if(s_flag)
    {
        signal(SIGPIPE, sighandler);
        signal(SIGINT, sighandler);

        pchild = fork();
        
        if(pchild == -1)
        {
            perror("ERROR: failed fork ");
            exit(EXIT_FAILURE);
        }
       
        if(pchild == 0)
        {
            close(pipeto[1]);
            close(pipefr[0]);
            dup2(pipeto[0], 0); //dup terminal stdin to shell
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
            pthread_create(&threadin, NULL, do_read, NULL);
        }
    }
    read_in(0, 1, 1);
    exit(0);
}
