#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
  The ds3ls prints the contents of a directory. 
  This utility takes two arguments: 
  the name of the disk image file to use, and the path of the directory or file to list within the disk image. 
  For directories, it prints all of the entries in the directory, sorted using the std::sort function. 
  Each entry goes on its own line. 
  For files, it prints just the information for that file. 
  Each entry will include the inode number, a tab, the name of the entry, and finishing it off with a newline.
  For all errors, we'll use a single error string: 
  "Directory not found" printed to standard error (e.g., cerr), and your process will exit with a return code of 1. 
  On success, your program's return code is 0.
*/

//Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }

  // parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string directory = string(argv[2]);

  int inode_num = 0;

  string token="";

  if(directory != "/"){
    directory=directory.substr(1);
    istringstream iss(directory);
    vector<std::string> tokens;
    while (getline(iss, token, '/')) {
        tokens.push_back(token);
    }
    for (auto token : tokens) {
      inode_num = fileSystem->lookup(inode_num, token); //start at root of filesystem
      if (inode_num < 0){
        cerr<<"Directory not found"<<endl;
        delete fileSystem;
        delete disk;
        return 1;
      }
    }
  }

  inode_t *inode = new inode_t();
  if(fileSystem->stat(inode_num, inode)!=0){
    cerr<<"Directory not found (stat)"<<endl;
        delete fileSystem;
        delete disk;
        delete inode;
        return 1;
  }

  if(inode->type == UFS_DIRECTORY){
    int dirSize = inode->size;
    char *buf = new char[dirSize];
    vector<dir_ent_t> dir_entries(dirSize/sizeof(dir_ent_t)); //how could this line be causing stack overflow?
    //read the block containing dir_ent_t
    fileSystem->read(inode_num, buf, dirSize);
    //read chunks of 32b as dir_ent_t
    for(int i = 0; i<dirSize/32; i++){
      memcpy(&dir_entries[i], buf+(sizeof(dir_ent_t)*i), sizeof(dir_ent_t));
    }
      
    sort(dir_entries.begin(), dir_entries.end(), compareByName);
    for(auto& dir_entry : dir_entries){
      cout<<dir_entry.inum<<"\t"<<dir_entry.name<<endl;
    }
    delete[] buf;
  }
  else{
    cout<<inode_num<<"\t"<<token<<endl;
  }
  delete fileSystem;
  delete disk;
  delete inode;

  return 0;
}
