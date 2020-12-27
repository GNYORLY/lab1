
README

Contents: lab1a.c, Makefile, README

lab1a

This program creates a character-at-a-time I/O terminal that can pass input to and read output from the shell.


USAGE

	lab1a [-s] 

-s will pass input/output between the terminal and shell


lab1a.c
This contains the source code of the program.

Makefile
This allows the user to compile the program into a distributable form as well as cleanly remove the program.

Makefile Options
Using make on its own will just compile the lab0 program.
  dist: using make dist will create a tarball containing the lab1a program files
  clean: using make clean will remove all files previously created using this makefile and return it to its untared state

Citation
I used the following websites for examples and guides to help with this project.
http://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html
https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.3/html_chapter/libc_17.html#SEC360
https://en.wikibooks.org/wiki/Serial_Programming/termios
http://www.thegeekstuff.com/2012/05/c-fork-function/
