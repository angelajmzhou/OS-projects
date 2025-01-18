#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#define SZ_BUFFIN 2000
#define SZ_BUFFOUT 4000
using namespace std;

int main(int argc, char *argv[]) {
    int fd;
    char buffIn[SZ_BUFFIN];
    char buffOut[SZ_BUFFOUT];
    uint32_t nchar;
    int bytesIn;
    int bytesOut = 0;
    char currch;

    if (argc == 1) {
        cout << "wunzip: file1 [file2 ...]" << endl;
        return 1;
    }

    for (int fn = 1; fn < argc; fn++) {
        fd = open(argv[fn], O_RDONLY);
        if (fd == -1) {
            cerr << "wunzip: cannot open file" << endl;
            return 1;
        }
        while ((bytesIn = read(fd, buffIn, SZ_BUFFIN)) > 0) {
            for (int i = 0; i < bytesIn; i += 5) {
                memcpy(&nchar, &buffIn[i], sizeof(uint32_t));
                currch = buffIn[i + 4];

                for (uint32_t j = 0; j < nchar; j++) {
                    buffOut[bytesOut++] = currch;
                    if (bytesOut == SZ_BUFFOUT) {
                        if (write(STDOUT_FILENO, buffOut, bytesOut) == -1) {
                            cerr << "wunzip: cannot write to stdout" << endl;
                            return 1;
                        }
                        bytesOut = 0;
                    }
                }
            }
        }
        close(fd);
    }
    if (bytesOut > 0) {
        if (write(STDOUT_FILENO, buffOut, bytesOut) == -1) {
            cerr << "wunzip: cannot write to stdout" << endl;
            return 1;
        }
    }

    return 0;
}
