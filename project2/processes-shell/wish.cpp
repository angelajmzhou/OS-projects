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

int parseInput(string line);
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
             if(parseInput(line) == 1){break;}
        }
        exit(0);
    }

    cout<<"wish> ";
    while(getline(cin, line)){
        cout<<"wish> ";
        if(parseInput(line) == 1){break;}
    }
    exit(0);
}

int parseInput(string line){
    //taken from: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
        istringstream iss(line);
        vector<string> tokens{istream_iterator<string>{iss},
                      istream_iterator<string>{}}; 
        if(tokens.size()==0){return 0;}
        string cmd;
        cmd = tokens[0];
        //handle our built-in commands
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
                            return 1;
                        } // Call execv
                    } else{ //if parent, wait for child
                        if(wait(NULL) == -1){
                            return 1;
                        }
                    }
                    return 0;
                }
            }
            defaultError();
        }
        return 0;
}

void defaultError(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}