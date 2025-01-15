#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sstream>
#include <cstring>

using namespace std;

int main(int argc, char *argv[]){
    int fd;
    if (argc == 1){
        fd = STDIN_FILENO;
    }
    else{
        fd = open(argv[1], O_RDONLY);
        if (fd == -1){
        cerr << "Error opening file: " << argv[1] << " (" << strerror(errno) << ")" << endl;

            return 1;
        }
    }
    char buffer[4096];
    int ret;
    int bytesRead;
    stringstream stringStream;
    while((bytesRead = read(fd, buffer, sizeof(buffer))) > 0){
        ret = write(STDOUT_FILENO, buffer, bytesRead);
        if (ret == -1){
            cerr << "could not write to stdout." << endl;
            return 1;
        }
    }
    return 0;
}