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


using namespace std;

int main(int argc, char *argv[]){
    //return if >1 batch file
    if (argc>2){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }
    string line;
    //one batch file
    if (argc==2){
        ifstream file(argv[1]);
        if (file.fail()){
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            exit(1);
        }
        while (getline(file, line)) {
            
        }
        exit(0);
    }

    cout<<"wish> ";
    while(getline(cin, line) && line!="exit"){
        cout<<"wish> ";
        //taken from: https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
        istringstream iss(line);
        vector<string> tokens{istream_iterator<string>{iss},
                      istream_iterator<string>{}}; 

        
        if(tokens.size()==0){continue;}
    }
    exit(0);
}