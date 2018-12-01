#include "fat12.h"

#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fsuid.h>

/* read_unsigned_le: Reads a little-endian unsigned integer number
   from buffer, starting at position.
   
   Parameters:
     buffer: memory position of the buffer that contains the number to
             be read.
     position: index of the initial position within the buffer where
               the number is to be found.
     num_bytes: number of bytes used by the integer number within the
                buffer. Cannot exceed the size of an int.
   Returns:
     The unsigned integer read from the buffer.
 */
unsigned int read_unsigned_le(const char *buffer, int position, int num_bytes) {
  long number = 0;
  while (num_bytes-- > 0) {
    number = (number << 8) | (buffer[num_bytes + position] & 0xff);
  }
  return number;
}

/* open_volume_file: Opens the specified file and reads the initial
   FAT12 data contained in the file, including the boot sector, file
   allocation table and root directory.
   
   Parameters:
     filename: Name of the file containing the volume data.
   Returns:
     A pointer to a newly allocated fat12volume data structure with
     all fields initialized according to the data in the volume file,
     or NULL if the file is invalid, data is missing, or the file is
     smaller than necessary.
 */
fat12volume *open_volume_file(const char *filename) {
  FILE * fatd = fopen(filename, "r");
  char* buff = (char*) malloc(BOOT_SECTOR_SIZE);
  struct fat12volume *fat = malloc(sizeof(struct fat12volume));
  fread(buff, BOOT_SECTOR_SIZE, 1, fatd);

  if (buff != NULL){

    char* readBuff;
    // setbuff(*filename, buff);
    // fseek(fatd,0,SEEK_END);
    // int size = ftell(fatd);
    // fseek(fatd,0,SEEK_SET);
    // char * buff = malloc(size);
    // setbuffer(fatd, buff,size);


    fat->volume_file = fatd;
    
    fat->sector_size = read_unsigned_le(buff, 11, 2);

    fat->cluster_size = read_unsigned_le(buff, 13, 1);

    fat->reserved_sectors = read_unsigned_le(buff, 14, 2);

    fat->hidden_sectors = read_unsigned_le(buff, 28, 2);

    fat->fat_num_sectors = read_unsigned_le(buff, 22, 2);

    fat->fat_copies = read_unsigned_le(buff, 16 , 1);

    fat->fat_offset = fat->reserved_sectors + fat->rootdir_num_sectors * fat->fat_copies;

    read_sectors(fat, 1, fat->fat_num_sectors, &fat->fat_array);

    fat->rootdir_offset = fat->reserved_sectors + fat->fat_num_sectors * fat->fat_copies;

    fat->rootdir_entries = read_unsigned_le(buff, 17 , 2);

    fat->rootdir_num_sectors  = (fat->rootdir_entries * 32 / fat->sector_size);

    //read_sectors(fat->volume_file, fat->rootdir_offset, fat->rootdir_num_sectors, &fat->rootdir_array);

    fat->cluster_offset = fat->fat_offset + fat->rootdir_offset -  (2 * fat->rootdir_num_sectors / fat->sector_size);

    return fat;
  }

  return NULL;

}

/* close_volume_file: Frees and closes all resources used by a FAT12 volume.
   
   Parameters:
     volume: pointer to volume to be freed.
 */
void close_volume_file(fat12volume *volume) {
  
  /* TO BE COMPLETED BY THE STUDENT */
  free(&volume->fat_array);
  free(&volume->rootdir_array);
  fclose(volume);
}

/* read_sectors: Reads one or more contiguous sectors from the volume
   file, saving the data in a newly allocated memory space. The caller
   is responsible for freeing the buffer space returned by this
   function.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     first_sector: number of the first sector to be read.
     num_sectors: number of sectors to read.
     buffer: address of a pointer variable that will store the
             allocated memory space.
   Returns:
     In case of success, it returns the number of bytes that were read
     from the set of sectors. In that case *buffer will point to a
     malloc'ed space containing the actual data read. If there is no
     data to read (e.g., num_sectors is zero, or the sector is at the
     end of the volume file, or read failed), it returns zero, and
     *buffer will be undefined.
 */
int read_sectors(fat12volume *volume, unsigned int first_sector,
		 unsigned int num_sectors, char **buffer) {
  
  /* TO BE COMPLETED BY THE STUDENT */
  *buffer = (char*) malloc(num_sectors * volume->sector_size);
  fseek(volume->volume_file, first_sector * volume->sector_size, SEEK_SET);
  int ret = fread(*buffer, volume->sector_size, num_sectors, volume->volume_file);
  rewind(volume->volume_file);

  if (*buffer == NULL || num_sectors == 0) {
    //*buffer = NULL;
    return 0;
  } else {
    return ret;
  }
}

/* read_cluster: Reads a specific data cluster from the volume file,
   saving the data in a newly allocated memory space. The caller is
   responsible for freeing the buffer space returned by this
   function. Note that, in most cases, the implementation of this
   function involves a single call to read_sectors with appropriate
   arguments.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     cluster: number of the cluster to be read (the first data cluster
              is numbered two).
     buffer: address of a pointer variable that will store the
             allocated memory space.
   Returns:
     In case of success, it returns the number of bytes that were read
     from the cluster. In that case *buffer will point to a malloc'ed
     space containing the actual data read. If there is no data to
     read (e.g., the cluster is at the end of the volume file), it
     returns zero, and *buffer will be undefined.
 */
int read_cluster(fat12volume *volume, unsigned int cluster, char **buffer) {

  /* TO BE COMPLETED BY THE STUDENT */
  return read_sectors(volume->volume_file, volume->cluster_offset + (volume->cluster_size*volume->sector_size), volume->cluster_size, buffer);
}

/* get_next_cluster: Finds, in the file allocation table, the number
   of the cluster that follows the given cluster.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     cluster: number of the cluster to seek.
   Returns:
     Number of the cluster that follows the given cluster (i.e., whose
     data is the sequence to the data of the current cluster). Returns
     0 if the given cluster is not in use, or a number larger than or
     equal to 0xff8 if the given cluster is the last cluster in a
     file.
 */
unsigned int get_next_cluster(fat12volume *volume, unsigned int cluster) {
  unsigned char entry[2];
  uint32_t new_cluster;
  // if cluster is odd valued, last half of the array 
//   if (cluster % 2){
//     entry[0] = &volume[volume->fat_offset + cluster];
//     entry[1] = &volume[volume->fat_offset + cluster];
//     new_cluster = &entry[0];
//     new_cluster = new_cluster>>4;
//   }
//   // cluster is even in this case, first half of array
//   else{
//     entry[0] = &volume[volume->fat_offset + cluster];
//     entry[1] = &volume[volume->fat_offset + cluster];
//     entry[1] = entry[1]&0x0f;
//     new_cluster = &entry[0];
//   }

  return 0;
}

/* fill_directory_entry: Reads the directory entry from a
   FAT12-formatted directory and assigns its attributes to a dir_entry
   data structure.
   
   Parameters:
     data: pointer to the beginning of the directory entry in FAT12
           format. This function assumes that this pointer is at least
           DIR_ENTRY_SIZE long.
     entry: pointer to a dir_entry structure where the data will be
            stored.
 */
void fill_directory_entry(const char *data, dir_entry *entry) {

  /* TO BE COMPLETED BY THE STUDENT */
  /* OBS: Note that the way that FAT12 represents a year is different
     than the way used by mktime and 'struct tm' to represent a
     year. In particular, both represent it as a number of years from
     a starting year, but the starting year is different between
     them. Make sure to take this into account when saving data into
     the entry. */
}

/* find_directory_entry: finds the directory entry associated to a
   specific path.
   
   Parameters:
     volume: Pointer to FAT12 volume data structure.
     path: Path of the file to be found. Will always start with a
           forward slash (/). Path components (e.g., subdirectories)
           will be delimited with "/". A path containing only "/"
           refers to the root directory of the FAT12 volume.
     entry: pointer to a dir_entry structure where the data associated
            to the path will be stored.
   Returns:
     In case of success (the provided path corresponds to a valid
     file/directory in the volume), the function will fill the data
     structure entry with the data associated to the path and return
     0. If the path is not a valid file/directory in the volume, the
     function will return -ENOENT, and the data in entry will be
     undefined. If the path contains a component (except the last one)
     that is not a directory, it will return -ENOTDIR, and the data in
     entry will be undefined.
 */
int find_directory_entry(fat12volume *volume, const char *path, dir_entry *entry) {
  
  /* TO BE COMPLETED BY THE STUDENT */
  /* OBS: In the specific case where the path corresponds to the root
     directory ("/"), this function should fill the entry with
     information for the root directory, but this entry will not be
     based on a proper entry in the volume, since the root directory
     is not obtained from such an entry. In particular, the date/time
     for the root directory can be set to Unix time 0 (1970-01-01 0:00
     GMT). */
  return -ENOENT;
}

