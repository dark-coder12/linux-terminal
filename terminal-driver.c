#include <iostream>

#include <sys/wait.h>

#include <vector>

#include <cstring>

#include <sstream>

#include <unistd.h>

#include  <stdio.h>

#include  <sys/types.h>

#include <fcntl.h>

using namespace std;

int spaces = 0;

// this is a personal shell that helps a user keep typing commands on the command 

//line and with the help of a forked child these commands are successfully executed

// until a user exits by typing "exit"

int totalCmds = 0;

string history[1000];

char ** tokenizedArr;

// runs execvp command after all checks by other functions

void runCommand(int start, char * runExec[]) {

    if (execvp(runExec[start], runExec + start) < 0) {

        cout << "Command rejected by Execvp!" << endl;

        exit(0);

    } else cout << endl;

}

// this function handles input output and error redirection.

int inputOutputErrRedirection(int start, int size) {

    bool input = false;

    bool output = false;

    char * read;

    char * write;

    bool foundRed = false;

    int beforeRedirection = size;

    for (int i = start; i < size; i++) {

        // input file redirection

        if ((strcmp(tokenizedArr[i], "<") == 0) || (strcmp(tokenizedArr[i], "0<") == 0)) {

            input = true;

            if (!(i + 1 < size)) {

                cout << "File not specified!" << endl;

                return -1;

            } else {

                read = tokenizedArr[i + 1];

            }

            if (!foundRed)

                beforeRedirection = i;

            foundRed = true;

        }

        // output file redirection
        else if ((strcmp(tokenizedArr[i], ">") == 0) || (strcmp(tokenizedArr[i], "1>") == 0)) {

            output = true;

            if (!(i + 1 < size)) {

                cout << "File not specified!" << endl;

                return -1;

            } else {

                write = tokenizedArr[i + 1];

            }

            if (!foundRed)

                beforeRedirection = i;

            foundRed = true;

        }

        // error file redirection
        else if (strcmp(tokenizedArr[i], "2>") == 0) {

            output = true;

            if (!(i + 1 < size)) {

                cout << "File not specified!" << endl;

                return -1;

            } else {

                write = tokenizedArr[i + 1];

            }

            if (!foundRed)

                beforeRedirection = i;

            foundRed = true;

        }

    }

    // input files can't be created

    if (input == true) {

        int fd = open(read, O_RDWR, 0666);

        dup2(fd, STDIN_FILENO);

    }

    // output file may be created if it does not exist

    if (output == true) {

        int fd = open(write, O_RDWR | O_CREAT, 0666);

        dup2(fd, 1);

    }

    char * runExec[beforeRedirection];

    for (int i = start; i < beforeRedirection; i++) {

        runExec[i] = tokenizedArr[i];

    }

    // necessary null at the end of execvp

    runExec[beforeRedirection] = NULL;

    // call of execvp function

    runCommand(start, runExec);

    return 0;

}

// this function recieves the whole command. It breaks it into pieces if there are more than one

// pipes and calls each command through recursion. Pipe redirection is done here and each portion

// of the command is checked to see if input / output redirection exists, then execvp is called

int executeCommands(int input, int output) {

    // base condition

    if (input > output || input == output)

        return -1;

    int pipeIndex = 0;

    bool pipeFound = false;

    // finds the end of command (breaks into pieces)

    for (int i = input; i < output; i++) {

        if (strcmp(tokenizedArr[i], "|") == 0) {

            pipeIndex = i;

            pipeFound = true;

            break;

        }

    }

    // if no more pipe is there, input output redirection is checked

    if (pipeFound == false)

        return inputOutputErrRedirection(input, output);

    int fd[2];

    pipe(fd);

    // child (redirection of pipe)

    if (pid_t child = fork() == 0) {

        close(fd[0]);

        dup2(fd[1], 1);

        inputOutputErrRedirection(input, pipeIndex);

        // parent (redirection of pipe)

    } else {

        if (pipeIndex != output - 2) {

            close(fd[1]);

            dup2(fd[0], 0);

            executeCommands(pipeIndex + 1, output);

        }

        wait(NULL);

    }

    return 0;

}

// this function tokenizes command

void tokenize(string command) {

    istringstream iss(command);

    string tokens;

    spaces = 0;

    for (int i = 0; i < command.length(); i++) {

        if (command[i] == ' ') {

            spaces++;

        }

    }

    // 2 extra spaces; 1 for NULL at the end and one because the number of spaces are one less than

    // the total amount of words

    tokenizedArr = new char * [spaces + 2];

    int cmdIndex = 0;

    while (iss >> tokens) {

        tokenizedArr[cmdIndex] = new char[tokens.length() + 1];

        strcpy(tokenizedArr[cmdIndex], tokens.c_str());

        cmdIndex++;

    }

    // execvp should always end with a NULL to ensure end of arguments

    tokenizedArr[spaces + 1] = new char();

    tokenizedArr[spaces + 1] = NULL;

}

// driver

int main(int argc, char * argv[]) {

    string command;

    while (true) {

        getline(cin, command);

        bool cmdFound = true;

        if (command[0] == '!' && command.length() >= 2 && command[1] != '!') {

            command[0] = '0';

            int cmdNum = atoi(command.c_str());

            if (totalCmds >= cmdNum && (totalCmds - cmdNum) < 10) {

                command = history[cmdNum - 1];

                cmdFound = true;

            } else {

                cmdFound = false;

            }

        }

        if (command == "!!") {

            if (totalCmds > 0)

                command = history[totalCmds - 1];

            else cout << "There are no commands yet" << endl;

        }

        if (command == "history") {

            int counter = totalCmds;

            for (int i = totalCmds - 1; i >= 0 && i >= (totalCmds - 10); i--) {

                cout << counter << " : " << history[i] << endl;

                counter--;

            }

        } else {

            history[totalCmds] = command;

            if (cmdFound == true) {

                totalCmds++;

                // the loop only ends when the command exit is made

                if (command == "exit") {

                    exit(99);

                }

                // a stream to the command is opened so that the string words can automatically be read	

                tokenize(command);

                // a child process is created

                pid_t returnValue, PID;

                returnValue = fork();

                // incase fork does not succeed

                if (returnValue < 0) {

                    perror("Fork unsucessful");

                }

                // child calls the user's command
                else if (returnValue == 0) {

                    executeCommands(0, spaces + 1);

                }

                // parent waits for the child to complete and then this runs in a loop 

                // until user calls exit
                else {

                    int status = -1;

                    PID = wait( & status);

                    // deallocation 

                    for (int i = 0; i < spaces + 2; i++) {

                        delete tokenizedArr[i];

                    }

                    delete[] tokenizedArr;

                    //exit(0);

                }

            } else cout << "Command exceeds the scope of history / Command not found!" << endl;

        }

    }

}
