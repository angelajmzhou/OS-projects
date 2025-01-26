#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string>
#include <cstring> 
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <unistd.h>
#include <sys/wait.h>


using namespace std;

void handleBuiltIn(string cmd);
void parseInput(string line);
void parseCommand(string line);
void defaultError();

vector<const char *> paths = {"/bin"};

int main(int argc, char *argv[]){
    //return if >1 batch file
    if (argc>2){
        defaultError();
        exit(1);
    }
    string line;
    //one batch file
    if (argc==2){
        ifstream file(argv[1]);
        if (file.fail()){
            defaultError();
            exit(1);

        }
        while (getline(file, line)) {
            parseInput(line);
        }
        exit(0);
    }
    cout<<"wish> "<<flush;
    while (getline(cin, line)) {
        parseInput(line);
        cout << "wish> " << flush;
    }
    exit(0);

}

void handleBuiltIn(string cmd){
    //handle our built-in commands
    istringstream iss(cmd);
    vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}}; 
    if (cmd == "exit"){
        if(tokens.size()==1){
            exit(0);
        }
        defaultError(); 
    }
    else if(cmd == "cd"){
        if(tokens.size()==2){
            if(chdir(tokens[1].c_str())==-1){
                defaultError();
            }
        } else{
            defaultError(); 
        }
    }
    else if(cmd == "path"){
        //set search path(s) here
        paths.clear();
        for(long unsigned int i = 1; i<tokens.size(); i++){
            paths.push_back(tokens[i].c_str());
        }
        
    }
}

void parseInput(string line){
    stringstream ss(line);
    vector<string> commands;
    string command;

    while (getline(ss, command, '&')) {
        commands.push_back(command);
    }

    if(commands.size() == 1){
        handleBuiltIn(command);
    }

    for (long unsigned int i = 0; i < commands.size(); i++) {
        if (fork() == 0) {
            parseCommand(commands[i]);
            exit(0);                 
        }
    }

    int status;

    for (long unsigned int idx = 0; idx < commands.size(); idx++) {
        //https://stackoverflow.com/questions/27306764/capturing-exit-status-code-of-child-process
        wait(&status);
        if (WEXITSTATUS(status)==5){
            exit(0);
        }
    }
}

void parseCommand(string line){
    //taken from: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
        istringstream iss(line);
        vector<string> tokens{istream_iterator<string>{iss},
                      istream_iterator<string>{}}; 
        if(tokens.size()==0){return;}
        else if(tokens[0] == "exit" || tokens[0] == "cd" || tokens[0] == "path") {handleBuiltIn(line);}
        else{
            for(long unsigned int i = 0; i<paths.size(); i++){
                char path[(strlen(paths[i]) + tokens[0].size() + 2)]; //+2 for backslash n null terminator
                snprintf(path, sizeof(path), "%s/%s", paths[i], tokens[0].c_str());
                if(access(path, X_OK) == 0){ 
                    const char* argsv[tokens.size() + 1]; //pointer to an array of constant chars
                    for (size_t j = 0; j < tokens.size(); j++) {
                        argsv[j] = tokens[j].c_str(); 
                    }
                    argsv[tokens.size()] = NULL;
                    if(fork() == 0){ //if child
                        //pointer to an constant pointer to a character
                        if(execv(path, const_cast<char* const*>(argsv)) == -1){ 
                            defaultError();
                        } 
                        exit(0);
                    } else{ //if parent, wait for child
                        if(wait(NULL) == -1){
                            defaultError();
                        }
                    }
                    return;
                }
            }
            defaultError();
        }
}

void defaultError(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}