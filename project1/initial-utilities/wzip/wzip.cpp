#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>
#define SZ_BUFFIN 4000
#define SZ_BUFFOUT 2000
using namespace std;


int main(int argc, char *argv[]){
    int fd;
    char buffIn[SZ_BUFFIN];
    char buffOut[SZ_BUFFOUT];
    uint32_t nchar;
    int bytesIn = 0;
    int buffInd = 0;
    char currch;
    if (argc == 1){
        cout<<"wunzip: file1 [file2 ...]"<<endl;
        return 1;
    } else{
        
        for (int i = 1; i<argc; i++){
            /*
            overall logic: read into buffIn, zip into buffOut -- need it to treat input file stream as cts. 
            */
            fd = open(argv[i], O_RDONLY);
            if (fd == -1){
                cout << "wunzip: cannot open file" << endl;
                return 1;
            }
            while((bytesIn = read(fd, buffIn, SZ_BUFFIN)) > 0){
                int j;
                while (buffInd < bytesIn && j < SZ_BUFFOUT) {
                    currch = buffIn[buffInd++];
                    nchar = 1;
                    while (buffInd < bytesIn && buffIn[buffInd] == currch) {
                        nchar++;
                        buffInd++;
                    }

                    memcpy(&buffOut[j], &nchar, sizeof(uint32_t));
                    buffOut[j + 4] = currch;
                    j += 5;
                }
                if (write(STDOUT_FILENO, buffOut, j) == -1) {
                        cerr << "wunzip: cannot write to stdout" << endl;
                        return 1;
                }
            } 
        }
    }
    return 0;
}