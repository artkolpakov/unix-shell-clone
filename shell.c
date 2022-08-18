/*
 7/24/2022 
 programmed by Artem Kolpakov
*/
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdint.h>         // typedef uint8_t
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <err.h>
#include <stdbool.h> 
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// Check that uint8_t type exists
#ifndef UINT8_MAX
#error "No support for uint8_t"
#endif

static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/=";

int backgroundFlag = 0;         //a flag to represent whether a command should be run as a background
int foregroundFlag = 0;         //a flag to represent whether a command should be run as a foreground


/** 
 * Parses input (the entire line) by splitting it into tokens. Checks for the special symbols <, > and &, 
 * performing according actions, expands any instance of "$$" in a command into the process ID of the smallsh
 *  
 * @param input The input line to be parsed into arguments
 * @param arguments array of arguments to store newly parsed arguments
 * @param outfilep the output file pointer to be set if argument is >
 * @param infilep the input file pointer to be set if argument is <
 * @param pid the process ID of the smallsh itself
 * 
 * @return a flag to of whether a command should be run as a background or not
 */ 
int parseInput(char* input, char** arguments, char** infilep, char** outfilep, int pid){
    int flag = 0;                   //a flag to of whether a command should be run as a background or not
    char* savePtr;
    int counter = 0;                //to keep track of arguments array indexes
    char *dollarCand;               //will hold substring starting from $$, if there is one
    char* stringPid = NULL;         //will hold pid as a string
    char* finalResult = NULL;
    char *token = strtok_r(input, " \n", &savePtr);  //break a string into tokens
    while (token != NULL){      // If there are no more tokens strktok returns NULL.
        if (strcmp(token, ">") == 0) {      // If token is '>' set the output file
            token = strtok_r(NULL, " \n", &savePtr);   //get next token that stores the output file
            if (token){
                outfilep[0] = (char *) malloc(sizeof(token));            //Allocate the string for outfile
                strcpy(outfilep[0], token);                              //set outfile
            }
        }
        else if (strcmp(token, "<") == 0) {     // If token is '<' set the input file
            token = strtok_r(NULL, " \n", &savePtr);    //get next token that stores the input file
            if (token){
                infilep[0] = (char *) malloc(sizeof(token));            //Allocate the string for infile
                strcpy(infilep[0], token);                              //set infile
            }
        }
        else if (strcmp(token, "&") == 0 && strtok_r(NULL, " ", &savePtr) == NULL) { //check for & at the end of line
            if (foregroundFlag == 0){
                flag = 1;   // set background process flag to be true
                //Any non built-in command with an & at the end must be run as a background command
            }
        }
        else if (dollarCand = strstr(token, "$$")){     //to find a $$ substring in token
            int length = snprintf( NULL, 0, "%d", pid);     //get length for allocation of pid string
            char *stringPid = malloc(length + 1);           //allocate memory for stringPid
            
            while(1){
                dollarCand = strstr(token, "$$");           //get substring, starts from first occurance of $$
                if (dollarCand){
                    int indexOfOccur = dollarCand - token;  //calculate the index where $$ occurs
                    // printf("took %s\n", dollarCand);
                    // printf("Occurs at %d\n", indexOfOccur);
                    snprintf(stringPid, length + 1, "%d", pid); //formats and stores int pid into str stringPid

                    finalResult = malloc(strlen(token) + strlen(stringPid) + 1);    //string with $$ being replaced
                    strcpy(finalResult, token);                     //coppy initial string
                    char *fake = malloc(strlen(stringPid)-2);       //string to store enough chars, for now just 'o's
                    for (int i=0; i<strlen(stringPid)-2; i++){
                        fake[i] = 'o';
                    }
                    strcat(finalResult, fake); // to get the proper full length

                    int j = 0;
                    if (indexOfOccur == 0){ //start copying from 3rd char of $$ occur at the beginning
                        j = 2;
                    }
                    
                    //start copying everything except $$, replace $$ with string pid
                    for (int i = 0; i < strlen(finalResult); i++){
                        if (i == indexOfOccur){ //if $, replace it with pid
                            for (int k=0; k<strlen(stringPid); k++){
                                finalResult[i] = stringPid[k];
                                if (k+1 != strlen(stringPid)){
                                    i++;
                                }
                            }
                        }
                        else{   //else copy the old string with no $$
                            if (j == indexOfOccur || j == indexOfOccur+1) {
                                j++;
                            }
                            finalResult[i] = token[j];
                            j++;
                        }
                    }
                    token = malloc(sizeof(finalResult)+1);  //reallocate token to be of a proper size
                    strcpy(token, finalResult);         //coppy new string

                    //this works for just first $$, if there are more $$, repeat the steps above
                    if (dollarCand = strstr(token, "$$")){
                        continue;
                    } else {
                        break;
                    }
                }
            }
            // append token result after the full $$ replacement into arguments array
            arguments[counter] = (char *) malloc(sizeof(token));            // allocate space for an argument
            strcpy(arguments[counter], token);                              // append       
            counter +=1;                                                    // go to the next index
            free(token);        //deallocate memory
        }
        else{ 
            // append token into arguments array if it's not <, > or &
            arguments[counter] = (char *) malloc(sizeof(token));            // allocate space for an argument
            strcpy(arguments[counter], token);                              // append
            counter +=1;                                                    // go to the next index
        }
        token=strtok_r(NULL, " \n", &savePtr);          //get the next token and repeat the loop
    }
    arguments[counter] = NULL;  //set last argument to be NULL, same as null terminator in strings
    return flag;
}

/** 
 * Checks if first argument is "cd" custom command
 *  
 * @param input the first argument in arguments array
 *
 * @return true if it is cd, false otherwise
 */ 
bool checkForCd(char* input){
    if(strcmp(input, "cd") == 0){
        return true;
    }
    return false;
}

/** 
 * Checks if first argument is "cd" custom command
 *  
 * @param input the first argument in arguments array
 *
 * @return true if it is cd, false otherwise
 */
bool checkForExit(char* input){
    if(strcmp(input, "exit") == 0){
        return true;
    }
    return false;
}

/** 
 * Checks if first argument is "status" custom command
 *  
 * @param input the first argument in arguments array
 *
 * @return true if it is "status", false otherwise
 */
bool checkForStatus(char* input){   
    if(strcmp(input, "status") == 0){
		return true;
    }
    return false;
}

/** 
 * Checks if first argument is "base64" custom command
 *  
 * @param input the first argument in arguments array
 *
 * @return true if it is "base64", false otherwise
 */
bool checkForBase64(char* input){   
    if(strcmp(input, "base64") == 0){
		return true;
    }
    return false;
}
/** 
 * Prints out either the exit status value or the terminating signal of the last foreground process ran by shell.
 *  
 * @param status exit status value
 *
 * @return If called before any foreground command is run, then it prints the exit status 0 or 
 * otherwise prints terminating signal of the last foreground process ran by the shell
 */
void checkStatus(int status){   //Taken from Canvas! Reference: Process API - Monitoring Child Processes
    if(WIFEXITED(status)){      //Check if the child was terminated normally
        printf("exit value %d\n", status);          //Child exited normally with status
    } 
    else{ //Child exited abnormally 
        //check if child exited abnormally due to signal (status will be greater than 1)
        if (status > 1){
            printf("terminated by signal %d\n", WTERMSIG(status));
        } //otherwise still print exit value
        else{
            printf("exit value %d\n", status);
        }
    }
    fflush(stdout); //flush out the contents of an output stream
}

/** 
 * Performs base 64 encoding, encodes FILE, or standard input, to standard output. Prints encoded data immediately.
 * With no FILE, or when FILE is -, reads from standard input. Encoded lines wrap every 76 characters.
 * The data are encoded as described in the standard base64 alphabet.
 * 
 * @param arguments filename
 *
 * @return 0 if encoded successfully, 1 if fopen of file failed, 2 if fread from a file/stdin failed, 3 if there 
 * if a problem working with data read (putchar, fflush failed), 4 if fclose of a file/stdin failed
 */
int base64enc(char** arguments){
    FILE* source;
    int flag = 0;

    if ((arguments[2] == NULL && arguments[1] != NULL && strcmp(arguments[1], "-") == 0) || arguments[1] == NULL) {
        printf("Please enter data to encrypt (Press Ctrl+D twice to stop): ");
        source = stdin;
    }
    else{
        source = fopen(arguments[1], "r");
        if(source == NULL) {     // if opening of provided file fails
            return 1;
        }
        flag = 1;
    }

    size_t counter = 0;
    uint8_t out_idx[4] = {0};

    while(1) {
        uint8_t in[3] = {0};
        size_t numRead = fread(in, 1, 3, source);           //read 3 bytes at a time
        if (numRead == 0 && feof(source)){
            break;                                  //stop reading if we have read everything
        }
        if (numRead < sizeof in/sizeof *in && ferror(source)){      //if fread fails
            return 2;
        }
        // perform the actual encoding
        if (numRead == 3){
            out_idx[0] = alphabet[in[0] >> 2];
            out_idx[1] = alphabet[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            out_idx[2] = alphabet[((in[1] & 0x0F) << 2) | (in[2] >> 6)];
            out_idx[3] = alphabet[(in[2] & 0x3F)];
        }
        else if (numRead == 2){
            out_idx[0] = alphabet[in[0] >> 2];
            out_idx[1] = alphabet[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            out_idx[2] = alphabet[((in[1] & 0x0F) << 2)];
            out_idx[3] = alphabet[64];
        }
        else if (numRead == 1){
            out_idx[0] = alphabet[in[0] >> 2];
            out_idx[1] = alphabet[((in[0] & 0x03) << 4)];
            out_idx[2] = alphabet[64];
            out_idx[3] = alphabet[64];
        }

        counter += 4;
        fwrite(out_idx, 1, sizeof(out_idx), stdout);
        if(counter == 76){
            if (putchar('\n') == EOF) {
                return 3;                // if putchar failed, something is wrong with the data
            }
            counter = 0; 
        }
        if (numRead < sizeof(in)/sizeof(*in) && feof(source)){      //check if we should stop reading
            break;
        }
    }
    if (counter!= 0){                            
        if (putchar('\n') == EOF) {             // if putchar failed, something is wrong with the data
            return 3;
        }
    }
    if (fflush(stdout) == EOF){             //if fflush failed, something is wrong with the data
        return 3;
    }
    if(flag == 1){
        if (fclose(source) == EOF){     //If fclose() fails, it returns EOF, something is wrong with a file
            return 4;
        }
    }
    return 0;
}

/** 
 * Custom handler for SIGTSTP (CTRL-Z), overrides SIGTSTP with changing foreground flag/foreground flag action
 *  
 * @param signo signal value
 * 
 */
void handle_SIGTSTP(int signo){
    // If the user sends SIGTSTP again (already running on foreground)
    if (foregroundFlag == 1){
        char* message = "\nExiting foreground only mode\n: ";                                               
        write(STDOUT_FILENO, message, 32);  //the printf() family of functions is not reentrant, using write instead
        foregroundFlag = 0; //change the flag
    }
     else {
        // When the parent process running the shell receives SIGTSTP
        char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52); //the printf() family of functions is not reentrant, using write instead
        foregroundFlag = 1; //change the flag
    }
    fflush(stdout);  //flush out the contents of an output stream
}

int main(int argc, char *argv[]){
    char *input = malloc(2048); // shell supports command lines with a maximum length of 2048 characters
    char *arguments[512];       // a maximum of 512 arguments per line
    char *infilep[1] = {NULL};  // input file for I/O redirection
    char *outfilep[1] ={NULL};  // output file for I/O redirection
    pid_t bgProcessesPid[100] ={0}; // array that holds Pids of all current background processes
    int numOfBgProcesses = 0;   // number of BgProcesses being run at the moment
    int pid = getpid(); // grabing the parent pid
    int status = 0; // exit value 

    pid_t childPid; // child pid
    int childStatus;    // child exit status

    // Some parts of code are taken from Canvas! Reference: Exploration: Signal Handling API
    struct sigaction SIGTSTP_action = {0}; // sigaction to override SIGTSTP_action
    struct sigaction ignore_SIGTSTP = {0}; // sigaction to ignore SIGTSTP (CTRL-Z)
    struct sigaction ignore_SIGINT = {0};  // sigaction to ignore SIGINT (CTRL-C)

    SIGTSTP_action.sa_handler = handle_SIGTSTP; // Register handle_SIGTSTP as the signal handler, override CTRL-Z with actions that handle_SIGTSTP function performs                          
	ignore_SIGINT.sa_handler = SIG_IGN;         // SIG_IGN is specifying that SIGINT (CTRL-C) should be completely ignored     
    ignore_SIGTSTP.sa_handler = SIG_DFL;        // SIG_DFL is specifying that we want the default action (that got overriden) to be taken for SIGTSTP (CTRL-Z)

    SIGTSTP_action.sa_flags = SA_RESTART;  // cause an automatic restart of the interrupted system call after the 'new' signal handler gets set.

    // Register the SIGTSTP_action as the handler for SIGTSTP CTRL-Z (override it)
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
    
    // Register the ignore SIGINT as the handler for SIGINT, so that nothing happens when CTRL-C is pressed    
	sigaction(SIGINT, &ignore_SIGINT, NULL);

    fflush(stdout);     //flush out the contents of an output stream                                 

    while(1){
        //at the very beginning, before even asking for an input, check if there are any background pods that are done
        for (int i = 0; i < numOfBgProcesses; i++){     //iterate through the array that holds Pids of all current background processes
                pid_t pid = waitpid(bgProcessesPid[i], &childStatus, WNOHANG);  //check the status of these processes using waitpid
                if (pid != 0) {         // if no child process has terminated, then waitpid will return 0
                    if (WIFSIGNALED(childStatus)) {  // WIFSIGNALED returns true if the child was terminated abnormally
                        status = WTERMSIG(childStatus);     //get the signal by which the child was abnormally terminated
                        printf("background pid %d is done: terminated by signal %d\n",bgProcessesPid[i],status);
                        bgProcessesPid[i] = 0;  //set pid back to 0
                        numOfBgProcesses--;     //decrement number of background processes being run
                        fflush(stdout);
                    }

                    if (WIFEXITED(childStatus)) { //WIFEXITED returns true if the child was terminated normally
                        printf("background pid %d is done: exit value %d\n",bgProcessesPid[i],childStatus);
                        fflush(stdout);
                        bgProcessesPid[i] = 0;  //set pid back to 0
                        numOfBgProcesses--;     //decrement number of background processes being run
                    }
                }
            }

        printf(": ");   // ask for command/s
        fflush(stdout);     //flush out the contents of an output stream

        ssize_t size = 0;
        getline(&input, &size, stdin); // get user input as a line

        if (input[0] == '\n' || input[0] == '#') {  // if new line or a comment loop over again
            continue;   
        }

        int n = strlen(input); 
        for (int i = 0; i < n; i++) {   //iterate through the entire line that we receive as an input
            if (input[i] == '\n'){
                input[i] = '\0'; // replace endline with a null terminator for future convenience when parsing
            }
        }

        // parse a line into tokens, append tokens into arguments array, check for a background process, replace $$, check for <>
        backgroundFlag = parseInput(input, arguments, infilep, outfilep, pid);

        //if a first argument is a custom exit command, then exit from the program
        if (checkForExit(arguments[0])){
            exit(EXIT_SUCCESS);
        }

        //if a first argument is a custom cd command, then  change directory to the second argument
        if (checkForCd(arguments[0])){
            if (arguments[1]){  // if there is a second argument
                if (chdir(arguments[1]) != 0){  // attempt to change dir, print error if failed
                    printf("Invalid directory!\n");
                    status = 1;                 // change status to 1
                }
            }
            else{ // with no arguments, change to the directory specified in the HOME environment variable
                chdir(getenv("HOME"));
            }
            continue;   //ask for a new command, this one is performed 
        }
        
        //Run in foreground
        if (checkForStatus(arguments[0])){
            checkStatus(status);    //print out either the exit status value or the terminating signal of the last foreground process ran by shell.
            continue;   //ask for a new command, this one is performed 
        }

        // if a first argument is a custom base64 command, then perform an encoding
        if (checkForBase64(arguments[0])){
            if(arguments[2] != NULL){       //if there are 3+ args provided
                printf("Too many arguments, should be 'base64 <[FILE]>' or 'base64' or 'base64 <->' \n");
                fflush(stdout);
                continue;
            }
            else{
                int ret = base64enc(arguments);     //perform the encoding and print the result
                // check if execution returned an error, print the error
                if (ret == 1){
                    printf("Opening of provided file failed, invalid/non-existing file \n");
                }
                else if (ret == 2){
                    printf("Reading data failed, invalid data! \n");
                }
                else if (ret == 3){
                    printf("Processing read data failed, invalid data! \n");
                }
                else if (ret == 4){
                    printf("Closing provided file failed, invalid file \n");
                }
                continue;
            }
        }

        if (!backgroundFlag){   //ignore SIGINT
            ignore_SIGINT.sa_handler = SIG_DFL;     
            sigaction(SIGINT, &ignore_SIGINT, NULL);
        }

        int childStatus;
        childPid = fork();
        int openingResult;

        if(childPid == -1){         // Check if fork failed
            perror("fork() failed!");
            fflush(stdout);
            status = 1;
            exit(1);
        } 
        else if(childPid == 0){      //If fork is successful, the value of childPid will be 0
            // Child process executes this branch
            if (infilep[0] != NULL){    //check if input redirection has been requested
                openingResult = open(infilep[0], O_RDONLY);     //attempt to open a specified input file
                if (openingResult == -1) {  // check if open() fails and catch errors if there are any
                    // printf("cannot open badfile for input\n", infilep);
                    perror("cannot open() for input");
                    status = 1;             // change exit value
                    fflush(stdout);
                    exit(1);
                }
                if (dup2(openingResult, 0) == -1){      //perform redirection, tch errors if there are any
                    printf("Redirecting output failed\n");
                    status = 1;              // change exit value
                    fflush(stdout);
                    exit(1);
                }
                close(openingResult);
            }
            if (outfilep[0] != NULL){   //check if output redirection has been requested
                int openingResult = open(outfilep[0], O_WRONLY | O_CREAT | O_TRUNC, 0644); //create a file
                if (openingResult == -1) { // check if open() fails and catch any errors
                    // printf("Opeing of %s failed\n", infilep);
                    perror("cannot open() for output"); //print the error
                    status = 1;              // change exit value
                    fflush(stdout);
                    exit(1);
                }

                if (dup2(openingResult, 1) == -1){
                    printf("Redirecting output failed\n");      //print the error
                    status = 1;              // change exit value
                    fflush(stdout);          // clean output
                    exit(1);
                }
                close(openingResult);        //close file
            }

            //If the user doesn't redirect the standard input for a background command, then standard input should be 
            // redirected to /dev/null
            if (backgroundFlag == 1){
                if (!infilep[0]) {  //if no redirect for input file
                    int openingResult = open("/dev/null", O_RDONLY);               
                    if (openingResult == -1) { // check if open() fails and catch any possible errors
                        perror("cannot open() for input");
                        status = 1;              // change exit value
                        fflush(stdout);
                        exit(1);
                }

                if (dup2(openingResult, 1) == -1){  // redirect standard input to /dev/null, check for error
                    printf("Redirecting input failed\n");
                    status = 1;                 // change exit value
                    fflush(stdout);
                    exit(1);
                }
                close(openingResult);       //close file
                }

                if (!outfilep[0]) {     //if no redirect for output file
                    int openingResult = open("/dev/null", O_WRONLY);                
                    if (openingResult == -1) { // check if open() fails and catch any possible errors
                        perror("cannot open() for output");    //print error
                        status = 1;              // change exit value
                        fflush(stdout);
                        exit(1);
                    }

                    if (dup2(openingResult, 1) == -1){ // redirect standard input to /dev/null, check for error
                        printf("Redirecting output failed\n");
                        status = 1;              // change exit value
                        fflush(stdout);         //clear output
                        exit(1);
                    }
                    close(openingResult);   //close file
                }                
            }
            
            if (execvp(arguments[0], arguments)){       //let execvp peform a 'non-custom' command, check for error
                //coppied this error message from TA Tyler
                perror("Exec failed, exit status set to 1.\n");                      //if command does not exist
                status = 1;              // change exit value
                fflush(stdout);
                exit(1);
            }
        }
        else{ 
            //Check when the parent process running the shell receives SIGTSTP
            sigaction(SIGTSTP, &SIGTSTP_action, NULL);

            if (backgroundFlag == 1){       //check if it's a background process
                printf("background pid is %i\n", childPid);     //print background pid
                fflush(stdout);
                bgProcessesPid[numOfBgProcesses] = childPid;    //append Pid to the array of current Pids
                numOfBgProcesses++;                             //increment index of array that stores Pids
                childPid = waitpid(childPid, &status, WNOHANG); //update the status using waitpid
            }
            else {
                ignore_SIGINT.sa_handler = SIG_IGN;              //the parent process, must ignore SIGINT
                sigaction(SIGINT, &ignore_SIGINT, NULL);

                childPid = waitpid(childPid, &childStatus, 0);   //update status to check if terminated
                if (WIFSIGNALED(childStatus)) {             //check if the child was terminated abnormally
                    status = WTERMSIG(childStatus);         //get the signal number that caused the child to terminate
                    printf("terminated by signal %d\n", status);
                    fflush(stdout);
                }
                if (WIFEXITED(childStatus)) {               // check if the child was terminated normally
                    status = WEXITSTATUS(childStatus);      // updade status value passed to exit
                    //status exited normally with status ... updated   
                }
            }
        }

        input = NULL;
        free(input);
        infilep[0] = NULL;
        outfilep[0] = NULL;
    }

    return EXIT_SUCCESS;
}