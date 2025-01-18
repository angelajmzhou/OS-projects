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

int main(int argc, char *argv[]) {
    if (argc == 1) {
        cout << "wzip: file1 [file2 ...]" << endl;
        return 1;
    }

    char buffIn[SZ_BUFFIN];
    char buffOut[SZ_BUFFOUT];
    uint32_t nchar = 0;
    int bytesIn = 0;
    int buffInd = 0;
    int j = 0;
    char currch = 0, prevch = 0;
    bool firstChar = true;

    for (int file = 1; file < argc; file++) {
        int fd = open(argv[file], O_RDONLY);
        if (fd == -1) {
            cerr << "wzip: cannot open file " << argv[file] << endl;
            return 1;
        }

        while ((bytesIn = read(fd, buffIn, SZ_BUFFIN)) > 0) {
            buffInd = 0;

            while (buffInd < bytesIn) {
                currch = buffIn[buffInd];
                buffInd++;

                if (firstChar) {
                    prevch = currch;
                    nchar = 1;
                    firstChar = false;
                //keep track of continuous streams across files
                } else if (currch == prevch) {
                    nchar++;
                } else {
                    if (j + 5 > SZ_BUFFOUT) {
                        if (write(STDOUT_FILENO, buffOut, j) == -1) {
                            cerr << "wzip: cannot write to stdout" << endl;
                            close(fd);
                            return 1;
                        }
                        j = 0;
                    }
                    memcpy(&buffOut[j], &nchar, sizeof(uint32_t));
                    buffOut[j + 4] = prevch;
                    j += 5;

                    prevch = currch;
                    nchar = 1;
                }
            }
        }

        if (bytesIn == -1) {
            cerr << "wzip: error reading file " << argv[file] << endl;
            close(fd);
            return 1;
        }

        close(fd);
    }
    //flush remaining buffer
    if (!firstChar) {
        if (j + 5 > SZ_BUFFOUT) {
            if (write(STDOUT_FILENO, buffOut, j) == -1) {
                cerr << "wunzip: cannot write to stdout" << endl;
                return 1;
            }
            j = 0;
        }
        memcpy(&buffOut[j], &nchar, sizeof(uint32_t));
        buffOut[j + 4] = prevch;
        j += 5;
    }

    if (write(STDOUT_FILENO, buffOut, j) == -1) {
        cerr << "wunzip: cannot write to stdout" << endl;
        return 1;
    }

    return 0;
}
