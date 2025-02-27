#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <sstream>

#include "StringUtils.h"
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


  istringstream iss(directory);
  vector<std::string> tokens;
  string token;

  while (getline(iss, token, '/')) {
      tokens.push_back(token);
  }

  for (auto& token : tokens) {
    fileSystem->lookup(0, token);
  }
  
  return 0;
}
