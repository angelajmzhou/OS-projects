#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sstream>
#include <string>

using namespace std;

//note: GNU grep uses boyer-moore algo-- look into this!

int main(int argc, char *argv[]){
    int fd;
    char buffer[4096];
    int ret;
    int newline;
    int bytesRead;
    string line;

    //read stdin if no input file given (and is not terminal)
    if (argc == 1 && !isatty(STDIN_FILENO)){
        fd = STDIN_FILENO; //this sets input to stdin if no file specified
    }

    for (int i = 1; i<argc; i++){
        /*
        overall logic: read into buffer, append to string
        find newline char in string, get rid of all after that
        then move filepointer back

        then, for each string to be grep-d, see if it's in that line
        if it is, print it out :3
        */
        fd = open(argv[i], O_RDONLY);
        if (fd == -1){
            cout << "wgrep: cannot open file" << endl;
            return 1;
        } 
        while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
            line += buffer;
            //newline in buffer
            if((newline == line.find_first_of('\n')) != string::npos){
                 = line.substr(0, newline);
                line = line.substr(newline);
            };
            ret = write(STDOUT_FILENO, line.c_str(), line.size());
            if (ret == -1){
                cerr << "could not write to stdout." << endl;
                return 1;
            }
        }
    }
    return 0;
}

bool hasString(){
    
}