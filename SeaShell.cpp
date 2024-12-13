// Make a bash 
// g++ SeaShell.cpp -o a5 -lreadline && ./a5
// sudo apt-get install libreadline-dev


#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <bits/stdc++.h>
#include <unistd.h>
#include <vector>
#include <fstream>
#include <fcntl.h>    
#include <sys/stat.h> 

#include "readline/readline.h"
#include "readline/history.h"


using namespace std;

// Prototype
void processInputCommands(char *input, char *historyLocation);
void runsFile(string fileName, char *historyLocation);
void process(char **arguments, char *historyLocation);

// string tokenizer
char** splitInput(char* input) {
    char** args = new char*[500];

    char* token = strtok(input, " "); //https://man7.org/linux/man-pages/man3/strtok_r.3.html
    int i = 0;
    while (token != nullptr && i < 500) {
        args[i] = new char[strlen(token) + 1]; //make room for arg
        //cout << "Token: "<< token << endl;
        strcpy(args[i], token); //insert token as new arg
        token = strtok(nullptr, " "); // gets next token
        i++;
    }
    
    args[i] = nullptr;
    return args;
}

bool checkArguments(char** arguments, char checkingFor) {
    for(int i = 0; arguments[i] != nullptr; i++) {
        if (strchr(arguments[i], checkingFor)){
            return true;
        }
    }
    return false;
}

void fileRedirection(char** arguments){
    for(int i = 0; arguments[i] != nullptr; i++){
        // Output redirection
        if (strcmp(arguments[i], ">") == 0){
            // i+1 is name of file
            int fd = open(arguments[i + 1], O_WRONLY | O_CREAT, 0660);
            if (fd < 0) {
                perror("File redirection output error");
                exit(0);
            }
            dup2(fd, 1);
            close(fd);
            
            while(arguments[i] != nullptr){
                arguments[i] = arguments[i+2];
                i++;
            }
            break;

        // Input redirection
        } else if (strcmp(arguments[i], "<") == 0){ 
            // i+1 is name of file
            int fd = open(arguments[i + 1], O_RDONLY);
            if (fd < 0) {
                perror("File input redirection error");
                exit(0);
            }
            dup2(fd, 0);
            close(fd);
            //TODO: remove < and name?
            while(arguments[i] != nullptr){
                arguments[i] = arguments[i+2];
                i++;
            }
            break;
        } else if (strcmp(arguments[i], ">>") == 0){ 

            int fd = open(arguments[i + 1], O_WRONLY | O_APPEND, 0660);
            if (fd < 0) {
                int fd = open(arguments[i + 1], O_WRONLY | O_CREAT, 0660);
                if (fd < 0) {
                    perror("File redirection output error");
                    exit(0);
                }
                dup2(fd, 1);
                close(fd);
                
                while(arguments[i] != nullptr){
                    arguments[i] = arguments[i+2];
                    i++;
                }
                break;
            }
            dup2(fd, 1);
            close(fd);
            while(arguments[i] != nullptr){
                arguments[i] = arguments[i+2];
                i++;
            }
            break;
        }
    }
}

char** queueCommands(char *input){
    char** argsSplit = new char*[500];
    char* token = strtok(input, ";"); //https://man7.org/linux/man-pages/man3/strtok_r.3.html
    int i = 0;

    while (token != nullptr && i < 500) {
        argsSplit[i] = new char[strlen(token) + 1]; 
        strcpy(argsSplit[i], token); 
        token = strtok(nullptr, ";"); 
        i++;
    }
    
    argsSplit[i] = nullptr;
    return argsSplit;
}

void process(char **arguments, char *historyLocation){

    if(checkArguments(arguments, ';')){
        cout << "Found ;" << endl;
    }
    
    // if cd
    else if (arguments[0][0] == 'c' && arguments[0][1] == 'd') {
        int ret = chdir(arguments[1]); //https://man7.org/linux/man-pages/man2/chdir.2.html
        if (ret == -1){
            perror("chdir");
        }
    } 
    else if(checkArguments(arguments, '.')){
        string fileName;
        for(int i = 0; arguments[i] != nullptr; i++) {
            if (strchr(arguments[i], '.')){
                fileName = arguments[i+1];
                //cout << fileName <<endl;
                break;
            }
        }
        runsFile(fileName, historyLocation);
        
    }
    else if (arguments[0][0] == 'e' && arguments[0][1] == 'x' && arguments[0][2] == 'i' && arguments[0][3] == 't') {
        write_history(historyLocation);
        exit(0);

    } else {
        pid_t pid = fork();
        if (pid == 0) { // Child process    
            fileRedirection(arguments);
            execvp(arguments[0], arguments); //https://man7.org/linux/man-pages/man3/exec.3p.html 
            cout << "This is not a proper command" << endl;
            exit(0);
        } else if (pid > 0) { // Parent process
            waitpid(pid, nullptr, 0);
        } else {
            perror("fork");
        }
        
    }
}


char * getHistory (){
    string home = getenv("HOME");
    string historyDir = home + "/SeaShellHistory";

    char* historyDirChar = new char[historyDir.length() + 1];
    strcpy(historyDirChar, historyDir.c_str()); 
    read_history(historyDirChar);  // Load previous history from history.txt
    return historyDirChar;
}


void processInputCommands(char *input, char *historyLocation) {
    char **inputQueue = queueCommands(input);
    for (int i = 0; inputQueue[i] != nullptr; i++) {
        char **inputSplit;
        if (input[0] == '\0') continue; // Skip if first character is null terminator
        else {
            inputSplit = splitInput(inputQueue[i]);
        }
        
        process(inputSplit, historyLocation);

    }
}

// Function to check if the path is a regular file
bool isFile(const string &path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0) {
        return false; 
    }
    return S_ISREG(pathStat.st_mode); // Check if it's a regular file https://linux.die.net/man/2/stat
}

void runsFile(string fileName, char *historyLocation) {
    // check to make sure it is not a directory
    if (!isFile(fileName)) {
        cout << "Error: " << fileName << " is not a valid file." << endl;
        return;
    }

    ifstream file(fileName, ios::binary | ios::ate); // go to the end

    if (file.is_open()) {
        // Check if file is empty
        streamsize size = file.tellg(); 
        if (size == 0) {
            cout << "Error: File is empty or contains zero bytes." << endl;
            file.close();
            return;
        }

        // back to the begin
        file.seekg(0, ios::beg);
        string line;
        while (getline(file, line)) {
            char *lineCStr = new char[line.length() + 1];
            strcpy(lineCStr, line.c_str());    
            processInputCommands(lineCStr, historyLocation);
            delete[] lineCStr; 
        }
        file.close();
    } else {
        cout << "Error opening file: " << fileName << endl;
    }
}

// https://stackoverflow.com/questions/7257737/gnu-readline-and-key-bindings
int deleteCharacter(int count, int key) {
    cout << endl;
    // deletes the text between one to left and where we are to remove a character
    rl_delete_text(rl_point - 1, rl_point); //https://web.mit.edu/gnu/doc/html/rlman_2.html
    rl_point--;

    rl_on_new_line();
    rl_redisplay();
    return 0;
}

int clearScreen(int count, int key) {
    // Clear the screen
    cout << "\033[2J\033[1;1H"; // https://stackoverflow.com/questions/17335816/clear-screen-using-c
    rl_on_new_line();
    rl_redisplay();
    return 0;
}

void keyBindsChange() {
    
    rl_bind_keyseq("\\C-l", deleteCharacter);// ^L deletes a character
    
    rl_bind_keyseq("\\C-o", clearScreen); // ^P clear the screen

}


int main(int argc, char* argv[]){

    char * historyLocation = getHistory();
    char *input;
    string historyInsert;

    char cwd[1234];

    runsFile("myshell", historyLocation);

    keyBindsChange();

    while (true) {
        string currentPath = string(getcwd(cwd, sizeof(cwd)));
        currentPath = currentPath + '$'+ ' ';
        input = readline(currentPath.c_str());

        historyInsert = string(input); 

        processInputCommands(input, historyLocation);   

        if (*input) add_history(historyInsert.c_str());
        free(input);
        
    }


    return 0;
}


