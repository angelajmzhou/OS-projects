#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>

#define SZ_BUFFOUT 2048
using namespace std;

//note: GNU grep uses boyer-moore algo-- look into this!

int main(int argc, char *argv[]){
    int fd;
    char buffIn[1024];
    char buffOut[2048];
    uint32_t nchar;
    int bytesIn;
    int bytesOut = 0;
    int ret;
    
    if (argc == 1){
        cout<<"wunzip: file1 [file2 ...]"<<endl;
        return 1;
    } else{
        for (int i = 1; i<argc; i++){
            /*
            overall logic: read into buffIn, unzip into buffOut 
            */
            fd = open(argv[i], O_RDONLY);
            if (fd == -1){
                cout << "wunzip: cannot open file" << endl;
                return 1;
            }
            while((bytesIn = read(fd, buffIn, sizeof(buffIn))) > 0){
                for(int i = 0; i<bytesIn; i+=5){
                    nchar = *(reinterpret_cast<uint32_t*>(&buffIn[i]));
                    bytesOut += nchar;
                    if(bytesOut >= SZ_BUFFOUT){
                        ret = write(STDOUT_FILENO, buffOut, (bytesOut-nchar));
                        if (ret == -1){
                            cerr << "could not write to stdout." << endl;
                            return 1;
                        }
                        bytesOut = 0;
                    }
                    memset(buffOut, buffIn[i+4], nchar);
                }
        } 
    }
    }
    return 0;
}
