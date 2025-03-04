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
#include <cstdlib>
  
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
  if(fd==-1){
    cout<<"Could not write to dst_file"<<endl;
      exit(1);
  }

  char buf[UFS_BLOCK_SIZE];
  char *buffer = (char *) malloc(UFS_BLOCK_SIZE); //initial allocation
  int i = 1;
  int bytesRead;
  while((bytesRead = read(fd, buf, UFS_BLOCK_SIZE))>0){
    if(bytesRead<UFS_BLOCK_SIZE){
      buffer = (char*) realloc(buffer, (i-1) * UFS_BLOCK_SIZE + bytesRead);
      memcpy(buffer+(i-1)*UFS_BLOCK_SIZE, buf, bytesRead);
      break;
    }
    buffer = (char*) realloc(buffer, i * UFS_BLOCK_SIZE);  
    if(buffer == nullptr){
      free(buffer);
      close(fd);
      cout<<"Could not write to dst_file"<<endl;
      exit(1);
    }
    memcpy(buffer+ (i-1)*UFS_BLOCK_SIZE, buf, UFS_BLOCK_SIZE);
    i++;
  }


  if(fileSystem->write(dstInode, buffer, (i-1)*UFS_BLOCK_SIZE + bytesRead)<0){
    cout<<"Could not write to dst_file"<<endl;
    free(buffer);
    close(fd);
    exit(1);
  }

  free(buffer);
  close(fd);
  delete fileSystem;
  delete disk;
  return 0;
}
