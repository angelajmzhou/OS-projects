#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sstream>
#include <string>

using namespace std;

bool isitgreppy(string line, string grep);
//note: GNU grep uses boyer-moore algo-- look into this!

int main(int argc, char *argv[]){
    int fd;
    char buffer[4096];
    string line = "";
    long unsigned int nind;
    int bytesRead;
    //read stdin if no input file given (and is not terminal)
    if (argc == 1){
        cout<<"wgrep: searchterm [file ...]"<<endl;
        return 1;
    } else if (argc == 2 && !isatty(STDIN_FILENO)){
        fd = STDIN_FILENO;
        while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
            line.append(buffer, bytesRead);
            while((nind = line.find_first_of('\n')) != string::npos){
                if(!isitgreppy(line.substr(0, nind+1), argv[1])){
                    return 1;
                }
                line = line.substr(nind+1);

             }
            }

    } else{
        for (int i = 2; i<argc; i++){
            /*
            overall logic: read into buffer, append to string
            find newline char in string, get rid of all after that

            then, for each string to be grep-d, see if it's in that line
            if it is, print it out :3
            */
            fd = open(argv[i], O_RDONLY);
            if (fd == -1){
                cout << "wgrep: cannot open file" << endl;
                return 1;
            }
            while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
            line.append(buffer, bytesRead);
            //nind in buffer
            //runnign inf
            while((nind = line.find_first_of('\n')) != string::npos){
                if(!isitgreppy(line.substr(0, nind+1), argv[1])){
                    return 1;
                }
                line = line.substr(nind+1);

             }
            }
        } 
    }
    return 0;
}

bool isitgreppy(string line, string grep){
    int ret;
    if(line.find(grep) != string::npos){
        ret = write(STDOUT_FILENO, line.c_str(), line.size());
        if (ret == -1){
            cerr << "could not write to stdout." << endl;
            return false;
        }
    }
    return true;
}