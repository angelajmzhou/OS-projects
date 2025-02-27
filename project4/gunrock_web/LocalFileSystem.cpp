#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>
#include <type_traits>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;


LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char buf[UFS_BLOCK_SIZE];
  (*this->disk).readBlock(UFS_ROOT_DIRECTORY_INODE_NUMBER, buf);
  memcpy(super, buf, sizeof(super_t)); 
}
  // Helper functions, you should read/write the entire inode and bitmap regions

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  char buf[UFS_BLOCK_SIZE]; 
  int bytes = (super->num_inodes+7)/8;
  bytes = bytes%UFS_BLOCK_SIZE?bytes:UFS_BLOCK_SIZE; 
  
  //calculate leftover bytes from last block
  for (int i = 0; i < super->inode_bitmap_len; i++) {
    (*this->disk).readBlock(super->inode_bitmap_addr + i, buf); 

    //handle last block
    if(i==super->inode_bitmap_len-1){
      memcpy(inodeBitmap + (i * UFS_BLOCK_SIZE), buf, bytes); 
      break;
    }
    memcpy(inodeBitmap + (i * UFS_BLOCK_SIZE), buf, UFS_BLOCK_SIZE); 
  }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {

}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  char buf[UFS_BLOCK_SIZE];

  //calculate leftover bytes from last block
  int bytes = (super->num_data+7)/8; //BYTES not bits smh my head
  bytes = bytes%UFS_BLOCK_SIZE?bytes:UFS_BLOCK_SIZE; 

  for (int i = 0; i < super->data_bitmap_len; i++) {
    (*this->disk).readBlock(super->data_bitmap_addr + i, buf); 
    //handle last block
    if(i==super->inode_bitmap_len-1){
      memcpy(dataBitmap + (i * UFS_BLOCK_SIZE), buf, bytes); 
      break;
    }
    memcpy(dataBitmap + (i * UFS_BLOCK_SIZE), buf, UFS_BLOCK_SIZE); 
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {

}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {

}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {

}
/**
   * Lookup an inode.
   *
   * Takes the parent inode number (which should be the inode number
   * of a directory) and looks up the entry name in it. The inode
   * number of name is returned.
   *
   * Success: return inode number of name
   * Failure: return -ENOTFOUND, -EINVALIDINODE.
   * Failure modes: invalid parentInodeNumber, name does not exist.
   */
int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  inode_t *inode= new inode_t();

  //error checking
  if(stat(parentInodeNumber, inode)==EINVALIDINODE) {
    delete inode;
    return EINVALIDINODE;
  }
  if(inode->type != UFS_DIRECTORY){ 
    delete inode;
    return ENOTFOUND;
  }


  int dirSize = inode->size;
  char buf[dirSize];
  //read the block containing dir_ent_t
  read(parentInodeNumber, buf, dirSize);
  
  dir_ent_t *dir = new dir_ent_t();
  //read chunks of 32b as dir_ent_t
  for(int i = 0; i<dirSize; i+=32){
    memcpy(dir, buf+(32*i), sizeof(dir_ent_t));
    if(dir->name == name){
      return dir->inum;
    }
  }
  delete inode;
  delete dir;
  return ENOTFOUND;
}

/**
   * Read an inode.
   *
   * Given an inodeNumber this function will fill in the `inode` struct with
   * the type of the entry and the size of the data, in bytes, and direct blocks.
   *
   * Success: return 0
   * Failure: return -EINVALIDINODE
   * Failure modes: invalid inodeNumber
   */
int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  super_t *super = new super_t();
  readSuperBlock(super);
  //max inodes
  if(inodeNumber >= super->num_inodes || inodeNumber < 0){
    delete super;
    return EINVALIDINODE;
  }

  //check if inode is valid in bitmap
  int byteBlock = inodeNumber/8;
  int bitPos = inodeNumber%8;
  unsigned char bit = 1 << bitPos;
  unsigned char readBit[((super->num_inodes + 7) / 8)];
  readInodeBitmap(super, readBit);
  if (!(readBit[byteBlock] & bit)){
    delete super;
    return EINVALIDINODE;
  }

  //if valid, fill inode struct 
  char buf[UFS_BLOCK_SIZE];
  int block_num = (super->inode_bitmap_addr) + (inodeNumber/32); //UFS_BLOCK_SIZEB / 128B = 32 inode entries/block
  int offset = (inodeNumber%32) * sizeof(inode_t);
  (*this->disk).readBlock(block_num, buf); 
  memcpy(inode, buf+offset, sizeof(inode_t));

  delete super;
  return 0;
}

/**
   * Read the contents of a file or directory.
   *
   * Reads up to `size` bytes of data into the buffer from file specified by
   * inodeNumber. The routine should work for either a file or directory;
   * directories should return data in the format specified by dir_ent_t.
   *
   * Success: number of bytes read
   * Failure: -EINVALIDINODE, -EINVALIDSIZE.
   * Failure modes: invalid inodeNumber, invalid size.
*/

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t *inode= new inode_t();
  if(stat(inodeNumber, inode)==EINVALIDINODE) return EINVALIDINODE; 
  
  char buf[UFS_BLOCK_SIZE];

  unsigned int *direct = inode->direct;

  if (size>inode->size || size == 0) return EINVALIDSIZE;

  //calculate # blocks to read
  int num_blocks = (size+(UFS_BLOCK_SIZE-1))/UFS_BLOCK_SIZE;
  //calcuate bytes to read from the last block
  int last_bytes_read = size%UFS_BLOCK_SIZE?size%UFS_BLOCK_SIZE:UFS_BLOCK_SIZE;

  for (int i = 0; i < num_blocks; i++) {
    (*this->disk).readBlock(direct[i], buf); 
    if(i==num_blocks-1){
      memcpy(static_cast<char*>(buffer) + (i * UFS_BLOCK_SIZE), buf, last_bytes_read);

      break;
    }
    memcpy(static_cast<char*>(buffer) + (i * UFS_BLOCK_SIZE), buf, UFS_BLOCK_SIZE);

  }
  return size;
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

