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
#include <iterator>
#include <unistd.h>
#include <sys/wait.h>


using namespace std;

bool handleBuiltIn(string cmd);
void parseInput(string line);
void parseCommand(string redir[], bool redirect);
void handleRedirect(string line);
void defaultError();

vector<string> paths = {"/bin"};

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

bool handleBuiltIn(string cmd){
    //handle our built-in commands
    istringstream iss(cmd);
    vector<string> tokens{istream_iterator<string>{iss}, istream_iterator<string>{}}; 
    if (tokens.empty()) {
        return true; 
    } 
    if (tokens[tokens.size()-1]==">"){
        defaultError();
        return true;
    }
    else if (tokens[0] == "exit"){
        if(tokens.size()==1){
            exit(0);
        } else {
            defaultError(); 
            return true;
        }
    }
    else if(tokens[0] == "cd"){
        if(tokens.size()==2){
            if(chdir(tokens[1].c_str())==-1){
                defaultError();
            }
            return true;
        } else{
            defaultError(); 
            return true;
        }
    }
    else if(tokens[0] == "path"){
        //set search path(s) here
        paths.clear();
        for(long unsigned int i = 1; i<tokens.size(); i++){
            paths.push_back(tokens[i]);
        }
        return true;
    }
    return false;
}

void parseInput(string line){
    stringstream ss(line);
    vector<string> commands;
    string command;

    while (getline(ss, command, '&')) {
        commands.push_back(command);
    }
    if(commands.size() == 1){
        if(handleBuiltIn(commands[0])){
            return;
        }
    }
    for (long unsigned int i = 0; i < commands.size(); i++) {

        if (fork() == 0) {
            handleRedirect(commands[i]);
            exit(0);                 
        }
    }

    for (long unsigned int idx = 0; idx < commands.size(); idx++) {
        //https://stackoverflow.com/questions/27306764/capturing-exit-status-code-of-child-process
        wait(NULL);

    }
}

void handleRedirect(string line){
    stringstream ss(line);
    string tokens[2];
    string tmp;
    int pos;

    pos = line.find_first_not_of(" \t\n");

    if(line[pos]=='>'){
        defaultError();
        exit(0);
    }
    pos = 0;
    while (getline(ss,tmp, '>')) {
        if(pos>=2){
            defaultError();
            exit(0);
        }
        tokens[pos] = tmp;
        pos++;
    }

    if(pos == 1){
        tokens[1] = "";
        parseCommand(tokens, false);
    }
    else if (pos == 2){
        istringstream issdir(tokens[1]);
        vector<string> files{istream_iterator<string>{issdir},
                    istream_iterator<string>{}};
        if(files.size() != 1){
            defaultError();
            exit(0);
        } 
        tokens[1] = files[0];
        parseCommand(tokens, true);
    }

}
void parseCommand(string redir[], bool redirect){
    //taken from: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
        istringstream iss(redir[0]);
        vector<string> tokens{istream_iterator<string>{iss},
                      istream_iterator<string>{}}; 

        if(tokens.empty()){
            return;
        }
        if(tokens[0] == "exit" || tokens[0] == "cd" || tokens[0] == "path") {
            handleBuiltIn(redir[0]);
        }
        
        else{
            for(long unsigned int i = 0; i<paths.size(); i++){
                char path[(paths[i].size() + tokens[0].size() + 2)]; //+2 for backslash n null terminator
                snprintf(path, sizeof(path), "%s/%s", paths[i].c_str(), tokens[0].c_str());

                if(access(path, X_OK) == 0){ 
                    const char* argsv[tokens.size() + 1]; //pointer to an array of constant chars
                    for (size_t j = 0; j < tokens.size(); j++) {
                        argsv[j] = tokens[j].c_str(); 
                    }
                    argsv[tokens.size()] = NULL;

                    if(fork() == 0){ //if child
                        //pointer to an constant pointer to a character
                        if(redirect){
                            if (redir[1].empty() || redir[1].find_first_not_of(" \t\n") == string::npos) {
                                defaultError();
                                exit(0);
                            }
                            int fd = open(redir[1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if (fd < 0) {
                                defaultError();
                                exit(0);
                            }
                            dup2(fd, STDOUT_FILENO);
                            dup2(fd, STDERR_FILENO);
                        }   
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