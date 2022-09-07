This program is my own custom shell in C (very similar to Bash Unix shell).
The program provides: 

• a prompt for running commands + execution of these commands 

• input and output redirection

• expansion for the variable $$ into the process ID of the shell itself

• running commands in foreground and background processes

• a custom signal handlers for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z) 

• a custom base64 encoder command that encodes the data.

The implementation details are documented directly in the shell.c file.

To run the program use:
gcc -std=gnu99 -o shell shell.c
