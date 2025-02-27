#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

// do this second, try to pass all local tests

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cerr << argv[0] << ": diskImageFile" << endl;
    return 1;
  }

  // Parse command line arguments
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  super_t *super = new super_t();
  fileSystem->readSuperBlock(super);

  //print out "super" metadata
  cout<<"Super"<<endl;
  cout<<"inode_region_addr"<<" "<<super->inode_region_addr<<endl;
  cout<<"inode_region_len"<<" "<<super->inode_region_len<<endl;
  cout<<"num_inodes"<<" "<<super->num_inodes<<endl;
  cout<<"data_region_addr"<<" "<<super->data_region_addr<<endl;
  cout<<"data_region_len"<<" "<<super->data_region_len<<endl;
  cout<<"num_data"<<" "<<super->num_data<<endl;
  cout<<endl;

  //print out inode bitmap
  cout<<"Inode bitmap"<<endl;
  int IBS = (super->num_inodes + 7) / 8; 
  unsigned char Ibitmap[IBS];
  fileSystem->readInodeBitmap(super, Ibitmap);
  for (int idx =0; idx<IBS; idx++){
    cout << (unsigned int) Ibitmap[idx] << " ";
  }

  cout<<"\n"<<endl;

  //print out data bitmap
  cout<<"Data bitmap"<<endl;
  int DBS = (super->num_data + 7) / 8; 
  unsigned char Dbitmap[DBS];
  fileSystem->readDataBitmap(super, Dbitmap);
  for (int idx =0; idx<DBS; idx++){
    cout << (unsigned int) Dbitmap[idx] << " ";
  }
  
  cout<<endl;

  //memory cleanup
  delete disk;
  delete fileSystem;
  delete super;
  return 0;
}
