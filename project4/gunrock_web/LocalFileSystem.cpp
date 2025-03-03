#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>
#include <type_traits>

#include "LocalFileSystem.h"
#include "ufs.h"

//passing all but 36!
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
  char buf[UFS_BLOCK_SIZE]; //Memory access at offset 4368 overflows this variable
  int total_bytes = (super->num_inodes+7)/8;
  (*this->disk).readBlock(super->inode_bitmap_addr, buf);
  memcpy(inodeBitmap, buf, total_bytes); 

}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  char buf[UFS_BLOCK_SIZE]; 
  int total_bytes = (super->num_inodes + 7) / 8; //calculate # bytes needed for bitmap
  memcpy(buf, inodeBitmap, total_bytes);
  (*this->disk).writeBlock(super->inode_bitmap_addr, buf);
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
  char buf[UFS_BLOCK_SIZE]; 
  int total_bytes = (super->num_data + 7) / 8;
  int last_block_bytes = total_bytes % UFS_BLOCK_SIZE;

  for (int i = 0; i < super->inode_bitmap_len; i++) {
      if (i == super->inode_bitmap_len - 1) {
          memset(buf, 0, UFS_BLOCK_SIZE);
          memcpy(buf, dataBitmap + (i * UFS_BLOCK_SIZE), last_block_bytes);
      } else {
          memcpy(buf, dataBitmap + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
      }
      (*this->disk).writeBlock(super->data_bitmap_addr + i, buf);
  }
}


void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  //UFS_BLOCK_SIZEB / 128B = 32 inode entries/block
  int num_blocks = (super->num_inodes+31)/32;

  int buf_size = UFS_BLOCK_SIZE*num_blocks;
  char buf[buf_size];

  for(int i = 0; i<num_blocks; i++){
    (*this->disk).readBlock(super->inode_region_addr + i, buf+i*UFS_BLOCK_SIZE); 
  }

  memcpy(inodes, buf, sizeof(inode_t)*super->num_inodes);
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  int num_blocks = (super->num_inodes + 31) / 32;
  int buf_size = UFS_BLOCK_SIZE * num_blocks;
  char buf[buf_size];

  size_t nbytes = sizeof(inode_t) * super->num_inodes;
  memcpy(buf, inodes, nbytes);

  for (int i = 0; i < num_blocks; i++) {
    disk->writeBlock(super->inode_region_addr + i, buf + i * UFS_BLOCK_SIZE);
  }
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
    return -EINVALIDINODE;
  }
  if(inode->type != UFS_DIRECTORY){ 
    delete inode;
    return -ENOTFOUND;
  }

  int dirSize = inode->size;
  char buf[dirSize];
  //read the block containing dir_ent_t
  read(parentInodeNumber, buf, dirSize);
  
  dir_ent_t *dir = new dir_ent_t();
  //read chunks of 32b as dir_ent_t
  for(int i = 0; i<dirSize; i+=32){
    memcpy(dir, buf+(i), sizeof(dir_ent_t));
    if(dir->name == name){
      delete inode;
      int inum = dir->inum;
      delete dir;
      return inum;
    }
  }
  delete inode;
  delete dir;
  return -ENOTFOUND;
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
    return -EINVALIDINODE;
  }

  //check if inode is valid in bitmap
  //individual bytes are REVERSED
  int byteBlock = inodeNumber/8;
  int bitPos = inodeNumber%8;
  unsigned char bit = 1 << bitPos;
  unsigned char IBM[((super->num_inodes + 7) / 8)]; //stack overflow??
  readInodeBitmap(super, IBM);
  if (!(IBM[byteBlock] & bit)){ //bitwise and of the two chars
    delete super;
    return -ENOTALLOCATED;
  }

  //if valid, fill inode struct 
  char buf[UFS_BLOCK_SIZE];
  //
  int block_num = (super->inode_region_addr) + (inodeNumber/32); //UFS_BLOCK_SIZEB / 128B = 32 inode entries/block
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
// Calls to read always read data starting from the beginning of the file, 
// but if the caller specifies a size of less than the size of the object then you should return only these bytes. 
// If the caller specifies a size of larger than the size of the object, then you only return the bytes in the object.

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t *inode= new inode_t();
  if(stat(inodeNumber, inode)==EINVALIDINODE) {
    delete inode;
    return -EINVALIDINODE; 
  }
  
  char buf[UFS_BLOCK_SIZE];

  unsigned int *direct = inode->direct;

  if (size <= 0){
    delete inode;
    return -EINVALIDSIZE;
  }
  else if(size>inode->size){
    size=inode->size;
  }

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
  delete inode;
  return size;
}
/**
   * Makes a file or directory.
   *
   * Makes a file (type == UFS_REGULAR_FILE) or directory (type == UFS_DIRECTORY)
   * in the parent directory specified by parentInodeNumber of name name.
   *
   * Success: return the inode number of the new file or directory
   * Failure: -EINVALIDINODE, -EINVALIDNAME, -EINVALIDTYPE, -ENOTENOUGHSPACE.
   * Failure modes: parentInodeNumber does not exist or is not a directory, or
   * name is too long. If name already exists and is of the correct type,
   * return success, but if the name already exists and is of the wrong type,
   * return an error.
   * 
   * 
   * One important class of errors that your LocalFileSystem needs to handle is out of storage errors. 
   * Out of storage errors can happen when one of the file system modification calls -- create and write 
   * -- does not have enough availabe storage to complete the request.
  // For write calls, you should write as many bytes as you can and return success to the caller with the number of bytes actually written.
  // For create calls, you'll need to allocate both an inode and a disk block for directories. 
  // If you have allocated one of these entities but can't allocate the other, 
  // make sure you free allocated inodes or disk blocks before returning an error from the call.
   */
int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  //take care of edge cases first
  if(name.length()>=28){//max 27 chars (due to \0)
    ////cout<<"name invalid"<<endl;
    return -EINVALIDNAME;
  }
  //check if already exists in parent directory
  int inode_exist=lookup(parentInodeNumber, name);
  inode_t *inode = new inode_t();
  //parent inode is invalid (error from lookup), return
  if(inode_exist==-EINVALIDINODE){
    //cout<<"invalid inode"<<endl;
    return -EINVALIDINODE;
  }
  //found in parent directory, check if valid
  if(inode_exist>=0){//not negative (error) means it was found
    if(stat(inode_exist, inode)==-EINVALIDINODE){
      //cout<<"stat fail"<<endl;
      delete inode;
      return -EINVALIDINODE;
    }
    //wrong type, error
    if(inode->type!=type){
      //cout<<"wrong type"<<endl;
      delete inode;
      return -EINVALIDTYPE;
    }
    //already exists and correct type-- success!
    delete inode;
    return 0;
  }

  //start allocating space.
  super_t *super = new super_t();
  readSuperBlock(super);

  stat(parentInodeNumber, inode); 

  if(inode->size==30*UFS_BLOCK_SIZE){
    delete super;
    delete inode;
    //cout<<"directory full"<<endl;
    return -ENOTENOUGHSPACE;
  }
  
  //CHECK VALID INODES
  int IBS = ((super->num_inodes + 7) / 8);
  unsigned char IBM[IBS];
  readInodeBitmap(super, IBM);
  int i_num = -1;
  int pos = 0;
  int i = 0;
  while(pos<IBS){
    if (IBM[pos] == 0xFF) {
      pos++;
      continue;
    }
    while((1<<i) & IBM[pos]) i++;
    i_num=pos*8+i;
    IBM[pos] = IBM[pos]|1<<i;
    break;
  }
  if(i_num<0){
    //cout<<"no free inodes"<<endl;
    delete super;
    delete inode;
    return -ENOTENOUGHSPACE;
  }
  //update inode bitmap

  //CHECK VALID DATA BLOCKS (if directory)
  //also check if the dir_ents are full?? just in case we need to allocate two?? 
  bool full_dir = (inode->size % UFS_BLOCK_SIZE) == 0;
  int blocksNeeded = (full_dir)?1:0;
  vector<int> block_nums;
  if(type==UFS_DIRECTORY){blocksNeeded++;}
  if(blocksNeeded){
    int DBS = ((super->num_data + 7) / 8);

    unsigned char DBM[DBS];
    readDataBitmap(super, DBM);
    pos = 0, i = 0;
    while(pos<DBS && static_cast<int>(block_nums.size()) < blocksNeeded){
      if (DBM[pos] == 0xFF) {
        pos++;
        continue;
      }
      for(i=0;i<8;i++){
        if(!((1<<i) & DBM[pos])){
          block_nums.push_back(pos*8+i);
          DBM[pos] = DBM[pos]|1<<i;
          if(static_cast<int>(block_nums.size()) == blocksNeeded){break;}
        }
      }
    }
    if(static_cast<int>(block_nums.size())<blocksNeeded){
      delete super;
      delete inode;
      //cout<<"not enough dblocks"<<endl;
      return -ENOTENOUGHSPACE;
    }
    writeDataBitmap(super, DBM);
  }
  //write bitmaps
  writeInodeBitmap(super, IBM);

  //once here, we know we have room! add as dir_ent()
  //create an inode OBJ and put data inside!
  int directInd = inode->size/(UFS_BLOCK_SIZE);
  if(full_dir){
    directInd++;
    inode->direct[directInd] = block_nums.back()+super->data_region_addr;
    block_nums.pop_back();
  }
  char buf[UFS_BLOCK_SIZE];
  inode_t *new_inode = new inode_t();
  new_inode->type = type;
  if(!block_nums.empty()){ //allocate to directory
    new_inode->size = 2*sizeof(dir_ent_t);
    new_inode->direct[0] = block_nums.back()+super->data_region_addr;
    dir_ent_t entries[2];
    entries[0] = dir_ent_t();
    entries[1] = dir_ent_t();
    strcpy(entries[0].name, ".");
    entries[0].inum = i_num;
    strcpy(entries[1].name, "..");
    entries[1].inum = parentInodeNumber;
    memcpy(buf,&entries, 2*sizeof(dir_ent_t));
    disk->writeBlock(new_inode->direct[0],buf);
    block_nums.pop_back();
  }else{
    new_inode->size = 0;
  }
  dir_ent_t *dirent = new dir_ent_t();
  strcpy(dirent->name, name.c_str());
  dirent->inum = i_num;
  int blocknum = inode->direct[directInd];
  disk->readBlock(blocknum, buf);
  memcpy(buf+(inode->size%UFS_BLOCK_SIZE), dirent, sizeof(dir_ent_t));
  inode->size+=sizeof(dir_ent_t);
  disk->writeBlock(blocknum, buf);

  inode_t inode_buf[super->num_inodes];
  readInodeRegion(super, inode_buf);
  inode_buf[parentInodeNumber] = *inode;
  inode_buf[i_num] = *new_inode;
  writeInodeRegion(super,inode_buf);

  delete inode;
  delete new_inode;
  delete super;
  delete dirent;
  return 0;
}
  /**
   * Write the contents of a file.
   *
   * Writes a buffer of size to the file, replacing any content that
   * already exists.
   *
   * Success: number of bytes written
   * Failure: -EINVALIDINODE, -EINVALIDSIZE, -EINVALIDTYPE.
   * Failure modes: invalid inodeNumber, invalid size, not a regular file
   * (because you can't write to directories).
   */
int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {

  //account for allocating more data blocks HERE
  inode_t *inode= new inode_t();
  if(stat(inodeNumber, inode)==EINVALIDINODE) {
    delete inode;
    return -EINVALIDINODE; 
  }
  if(inode->type==UFS_DIRECTORY){
    delete inode;
    return -EINVALIDTYPE;
  }
  if (size <= 0){
    delete inode;
    return -EINVALIDSIZE;
  }
  if(size>=30*UFS_BLOCK_SIZE){
    size = 30*UFS_BLOCK_SIZE;
  }
  //calculate # blocks we need to write
  int num_blocks = (size+(UFS_BLOCK_SIZE-1))/UFS_BLOCK_SIZE;

  //calcuate # of blocks the inode already has
  int prev_blocks = (inode->size+(UFS_BLOCK_SIZE-1))/UFS_BLOCK_SIZE;

  int block_diff = prev_blocks - num_blocks;

  super_t *super = new super_t();
  readSuperBlock(super);
  //get gnarly! "delete" extraneous blocks
  
  if(block_diff>0){//more old blocks than new blocks
    //read in data bitmap, then "clear" corresponding block bits in loop.
    int DBS = ((super->num_data + 7) / 8);
    unsigned char DBM[DBS];
    readDataBitmap(super, DBM);
    int pos = 0, i = 0;
    int block_num;
    for(int j = prev_blocks-1; j>=prev_blocks-block_diff; j--) {
      block_num = inode->direct[j];
      pos = block_num/8;
      i = block_num%8;
      DBM[pos] &= ~(1 << i);
    }
    writeDataBitmap(super, DBM);
  }
  //and assign new blocks if needed...this is probs where the error is
  else if(block_diff<0){//more new blocks than old blocks
    block_diff=-block_diff;
    deque<int> block_nums;
    int DBS = ((super->num_data + 7) / 8);

    unsigned char DBM[DBS];
    readDataBitmap(super, DBM);
    int pos = 0, i = 0;
    while(pos<DBS && static_cast<int>(block_nums.size()) < block_diff){
      if (DBM[pos] == 0xFF) {
        pos++;
        continue;
      }
      for(i=0;i<8;i++){
        if(!((1<<i) & DBM[pos])){
          block_nums.push_back(pos*8+i);
          DBM[pos] = DBM[pos]|1<<i; //this is correct
          if(static_cast<int>(block_nums.size()) == block_diff){break;}
        }
      }
      pos++;
    }
    if(static_cast<int>(block_nums.size())<block_diff){
      //if we can't allocate enough for everything, adjust how much we're writing
      size =(prev_blocks + block_nums.size())*UFS_BLOCK_SIZE;
    }
    writeDataBitmap(super, DBM);
    //add new blocks to direct
    for(int k = 0; k < static_cast<int>(block_nums.size()); k++){
      inode->direct[prev_blocks+k] = super->data_region_addr + block_nums.front();
    }
  }
  //in case size has changed (i.e. didn't have enough space to allocate)
  char buf[UFS_BLOCK_SIZE];
  num_blocks = (size+(UFS_BLOCK_SIZE-1))/UFS_BLOCK_SIZE;
  int last_bytes_write = size%UFS_BLOCK_SIZE?size%UFS_BLOCK_SIZE:UFS_BLOCK_SIZE;
  inode->size = size;

  //do the actual writing!
  for (int i = 0; i < num_blocks; i++) {
    if(i==num_blocks-1){
      memcpy(buf, static_cast<const char *>(buffer) + (i * UFS_BLOCK_SIZE), last_bytes_write);
    }else{
      memcpy(buf, static_cast<const char *>(buffer) + (i * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE);
    }
    (*this->disk).writeBlock(inode->direct[i], buf); 
  }

  //write back inode with updated information
  inode_t inode_buf[super->num_inodes];
  readInodeRegion(super, inode_buf);
  inode_buf[inodeNumber] = *inode;
  writeInodeRegion(super,inode_buf);

  delete inode;
  return size;
}
  /**
   * Remove a file or directory.
   *
   * Removes the file or directory name from the directory specified by
   * parentInodeNumber.
   *
   * Success: 0
   * Failure: -EINVALIDINODE, -EDIRNOTEMPTY, -EINVALIDNAME, -EUNLINKNOTALLOWED
   * Failure modes: parentInodeNumber does not exist or isn't a directory,
   * directory is NOT empty, or the name is invalid. Note that the name not
   * existing is NOT a failure by our definition. You can't unlink '.' or '..'
   */
int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  inode_t *inode = new inode_t();
  if(stat(parentInodeNumber, inode)==-EINVALIDINODE){
    delete inode;
    return -EINVALIDINODE;
  }else if(inode->type==UFS_REGULAR_FILE){
    delete inode;
    return -EINVALIDINODE;
  }
  if(name.size()>27){
    delete inode;
    return -EINVALIDNAME;
  }else if(name == "." || name ==".."){
    delete inode;
    return -EUNLINKNOTALLOWED;
  }
  //if name doesn't exist, return -- doesn't fail!
  int inode_num = -120;
  inode_t *dead_inode = new inode_t();

  //delete the dir_entry, scoot everything back
  char buf[inode->size];
  read(parentInodeNumber, buf, inode->size);
  //delete the dir_entry, scoot everything back
  dir_ent_t *dir = new dir_ent_t();
  bool found = false;
  //read chunks of 32b as dir_ent_t
  for(int i = 0; i<inode->size; i+=sizeof(dir_ent_t)){
    if(found){
      //copy next entry into current entry
      memcpy(buf+(i), buf+(i+sizeof(dir_ent_t)), sizeof(dir_ent_t));
    }else{
      memcpy(dir, buf+(i), sizeof(dir_ent_t));
      if(dir->name == name){
        inode_num = dir->inum;
        found=true;
        //overwrite next entry into current entry
        memcpy(buf+(i), buf+(i+sizeof(dir_ent_t)), sizeof(dir_ent_t));
        inode->size-=sizeof(dir_ent_t);
      }
    }
  }
  //update the directory entries
  write(parentInodeNumber, buf, inode->size);
  delete dir;
  if(inode_num < 0){
    delete inode;
    delete dead_inode;
    delete dir;
    return 0;
  }
  if(stat(inode_num, dead_inode)<0){
    delete inode;
    delete dead_inode;
    delete dir;
    return -EINVALIDINODE;
  }
  //collect all the blocks we need to free
  vector<int> block_nums;
  if(dead_inode->type==UFS_DIRECTORY){
    if(dead_inode->size > 2*static_cast<int>(sizeof(dir_ent_t))){
      delete inode;
      delete dead_inode;
      delete dir;
      return -EDIRNOTEMPTY;
    }
    block_nums.push_back(dead_inode->direct[0]);
    }else{
      int blocks = dead_inode->size / UFS_BLOCK_SIZE;
      if ((dead_inode->size % UFS_BLOCK_SIZE) != 0) {
        blocks += 1;
      }
      for(int i = 0; i<(blocks); i++){
        block_nums.push_back(dead_inode->direct[i]);
      }
    }

  super_t *super = new super_t();
  readSuperBlock(super);
  //read in data bitmap, then "clear" corresponding block bits in loop.
  int DBS = ((super->num_data + 7) / 8);
  unsigned char DBM[DBS];
  readDataBitmap(super, DBM);
  int pos = 0, i = 0;
  for(auto block_num:block_nums){
    pos = block_num/8;
    i = block_num%8;
    DBM[pos] = DBM[pos]&0<<i;
  }
  writeDataBitmap(super, DBM);

  //read in inode bitmap, "free" corresponding inode and write back.
  int IBS = ((super->num_inodes + 7) / 8);
  unsigned char IBM[IBS];
  readInodeBitmap(super, IBM);
  pos = inode_num/8;
  i = inode_num%8;
  IBM[pos] &= ~(1 << i);
  writeInodeBitmap(super, IBM);

  delete inode;
  delete dead_inode;
  delete dir;
  delete super;
  return 0;
}

