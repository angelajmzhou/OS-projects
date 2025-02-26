#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


// 1. implement readSuperBLock, readInodeBitmap, readDataBitmap

LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char buf[4096];
  (*this->disk).readBlock(UFS_ROOT_DIRECTORY_INODE_NUMBER, buf);
  memcpy(super, buf, sizeof(super_t)); 
}
  // Helper functions, you should read/write the entire inode and bitmap regions

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  char buf[4096];

  for (int i = 0; i < super->inode_bitmap_len; i++) {
    (*this->disk).readBlock(super->inode_bitmap_addr + i, buf); 

    //handle last block
    if(i==super->inode_bitmap_len-1){
      int bytes = (super->num_inodes)%4096? (super->num_inodes)%4096 : 4096;
      memcpy(inodeBitmap + (i * 4096), buf, bytes); 
      break;
    }
    memcpy(inodeBitmap + (i * 4096), buf, 4096); 
  }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  char buf[4096];
  for (int i = 0; i < super->data_bitmap_len; i++) {
    (*this->disk).readBlock(super->data_bitmap_addr + i, buf); 
    if(i==super->inode_bitmap_len-1){
      int bytes = (super->num_data)%4096? (super->num_data)%4096 : 4096;
      memcpy(dataBitmap + (i * 4096), buf, bytes); 
      break;
    }
    memcpy(dataBitmap + (i * 4096), buf, 4096); 
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {

}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {

}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  return 0;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  //do this after finishing ds3bits
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  return 0;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}

