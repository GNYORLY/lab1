#!/bin/bash
#Jeannie Chiem
#'gnyorly@gmail.com'
#ID:504666652

default:
	gcc -Wall -Wextra -g -pthread -lmcrypt lab1b-client.c -o lab1b-client
	gcc -Wall -Wextra -g -pthread -lmcrypt lab1b-server.c -o lab1b-server

clean:
	rm -rf lab1b-client lab1b-server

dist:
	tar -czvf lab1b-504666652.tar.gz Makefile README lab1b-client.c lab1b-server.c my.key
