#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]){
    int fd;
    char buffer[4096];
    int ret;
    int bytesRead;

    //isatty -- is interative input (i.e. terminal)
    if (argc == 1 && !isatty(STDIN_FILENO)){
        fd = STDIN_FILENO; //this sets input to stdin if no file specified
        while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
            ret = write(STDOUT_FILENO, buffer, bytesRead);
            if (ret == -1){
                cerr << "could not write to stdout." << endl;
                return 1;
            }
        }
    }

    for (int i = 1; i<argc; i++){
        fd = open(argv[i], O_RDONLY);
        if (fd == -1){
            cout << "wcat: cannot open file" << endl;
            return 1;
        } 
        while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
            ret = write(STDOUT_FILENO, buffer, bytesRead);
            if (ret == -1){
                cerr << "could not write to stdout." << endl;
                return 1;
            }
        }
        close(fd);
    }
    return 0;
}