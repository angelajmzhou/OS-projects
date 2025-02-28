#include <iostream>
#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <sstream>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string srcFile = string(argv[2]);
  int dstInode = stoi(argv[3]);

  int fd = open(srcFile.c_str(), O_RDONLY);
  char buf[UFS_BLOCK_SIZE];
  while(read(fd, buf, 4096)>=0){
    
  }


  /*
    The ds3cp utility copies a file from your computer into the disk image using your LocalFileSystem.cpp implementation. 
    It takes three command line arguments, the disk image file, the source file from your computer that you want to copy in, 
    and the inode for the destination file within your disk image.
    For all errors, print the string Could not write to dst_file to standard error and exit with return code 1. 
    We will always use files that exist on the computer for testing, 
    so if you encounter an error opening or reading from the source file you can print any error message that makes sense.
  */

  return 0;
}
