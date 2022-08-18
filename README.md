This program is my own custom shell in C (very similar to Bash unix shell).
The program provides: 1) a prompt for running commands + execution of these commands, 2) input and output redirection, 3) expansion for the variable $$ into the process ID of the shell itself,
4) running commands in foreground and background processes, 5) a custom signal handlers for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z), 6) a custom base64 encoder command that encodes the data.
The implementation details are documented directly in the shell.c file.

To run the program use:
gcc -std=gnu99 -o shell shell.c
