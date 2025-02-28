#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
The ds3cat utility prints the contents of a file to standard output. 
It takes the name of the disk image file and an inode number as the only arguments. 
It prints the contents of the file that is specified by the inode number.

For this utility, first print the string File blocks with a newline at the end,
and then print each of the disk block numbers for the file to standard out,
 one disk block number per line. 
 After printing the file blocks, print an empty line with only a newline.

Next, print the string File data with a newline at the end,
and then print the full contents of the file to standard out. 
You do not need to differentiate between files and directories, 
for this utility everything is considered to be data and should be printed to standard out.

Calling ds3cat on a directory is an error. 
For all errors print the string `Error reading file" to standard error and set your processes return code to 1.
*/
int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int inodeNumber = stoi(argv[2]);

  inode_t *inode = new inode_t();
  if(fileSystem->stat(inodeNumber, inode)!=0){
    cerr<<"Error reading file"<<endl;
    delete fileSystem;
    delete disk;
    delete inode;
    return 1;
  }

  int numBlocks = (inode->size+UFS_BLOCK_SIZE-1)/UFS_BLOCK_SIZE;

  if(inode->type == UFS_DIRECTORY){
    cerr<<"Error reading file"<<endl;
    delete fileSystem;
    delete disk;
    delete inode;
    return 1;
  }

  cout<<"File blocks"<<endl;

  unsigned int *direct = inode->direct;

  for(int i = 0; i<numBlocks; i++){
    cout<<direct[i]<<endl;
  }

  cout<<"\nFile data"<<endl;

  char buf[UFS_BLOCK_SIZE];
  int last_bytes = inode->size%UFS_BLOCK_SIZE?inode->size%UFS_BLOCK_SIZE:UFS_BLOCK_SIZE;

  for(int i = 0; i<numBlocks; i++){
    disk->readBlock(direct[i], buf);
    if(i==numBlocks-1){
      write(STDOUT_FILENO, buf, last_bytes);
      char null_terminator = '\0';
      write(STDOUT_FILENO, &null_terminator, 1);
      break;
    }
    write(STDOUT_FILENO, buf, UFS_BLOCK_SIZE);
  }

  delete fileSystem;
  delete disk;
  delete inode;

  return 0;
}

