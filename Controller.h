#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

 //Disk's name

#define disk_name "Disco Local (C:)"

 //Disk specifications constants

#define sector_size 512
#define cluster_size 4
#define cluster_memory 2048.0
#define sectors_per_track 60
#define sectors_per_cylinder 300
#define tracks_per_cylinder 5
#define tracks_per_surface 10
#define sectors_amount 3000
#define average_seek 4
#define min_seek 1
#define average_latency 6
#define transfer_time 12
#define address_size 256

 //FAT Table constants

#define available 0
#define unavailable 1
#define eof_flag 0
#define not_eof_flag 1

 //Returns' constants

#define succeed 0x0
#define ptr_missing_file NULL
#define missing_file -1
#define damaged_disk NULL
#define ptr_disk_full NULL
#define disk_full -4
#define corrupted_fat NULL
#define writing_failed -2
#define reading_failed -3

typedef struct sector{
    unsigned char sector_bytes[sector_size]; //Defines a sector of 512 bytes
}Sector;

typedef struct track{
    Sector track_sectors[sectors_per_track]; //Defines a track with 60 sectors
}Track;

typedef struct cylinder{
    Track cylinder_tracks[tracks_per_cylinder]; //Define a cylinder with 5 tracks
}Cylinder;

typedef struct virtualdisk{
    Cylinder *disk_cylinders; //tracks_per_surface is the upper allocation limit
}VirtualDisk;

typedef struct fatent{ //Defines the state of each sector
    unsigned int used;
    unsigned int eof;
    unsigned int next;
}FatEnt;

typedef struct fatfiles{ //Defines the address of a file
    char file_name[address_size];
    unsigned int first_sector;
}FatFiles;

typedef struct fattable{ //Defines the disk's FAT Table
    FatEnt *entities;
    FatFiles *files;
}FatTable;

/**
   installDisk();
   This function is responsible for the allocation of the memory that will be used to the disk structure
   @return : VirtualDisk pointer type, pointing to the just allocated memory area.
*/
VirtualDisk* installDisk();

/**
   uninstallDisk();
   This function does the opposite of installDisk, releasing all the allocated memory for the disk
   @args : VirtualDisk pointer type, pointing to the disk to be "destroyed".
*/
void uninstallDisk(VirtualDisk *);

/**
   startFat();
   Allocates and starts the FAT Table with the appropriated constants
   @return : FatTable pointer type, pointing to the just allocated memory area
*/
FatTable* startFat();

/**
   finishFat();
   Releases all the memory used for the FAT Table
   @args : FatTable pointer type containing the FAT Table to be "destroyed"
*/
void finishFat(FatTable *);

/**
   createBuffer();
   Opens a file and creates a buffer with its content
   @args : string with the filename and an pointer to an integer with the size of the buffer
   @return : string containing the file
*/
unsigned char* createBuffer(char *, int *);

/**
    findCluster();
    Finds the next empty cluster
    @args : FAT Table of the disk
    @return : Number of the sector where the cluster starts (integer)
*/
int findCluster(FatTable *);

/**
    freeClusters();
    In case of an error, this function restores the disk to its previous state (before the writing has been started)
    @args : FAT Table of the disk, the clusters and the amount of clusters that were used in the writing session
*/
void freeClusters(FatTable *, int *, int);

/**
    allocateClusters();
    Sets all clusters that a file will use
    @args : FAT Table of the disk, an integer with the amount of clusters needed by the file
    @return : an array of integers with the number of all clusters
*/
int* allocateClusters(FatTable *, int);

/**
    seekCluster();
    Locates a sector in the disk
    @args : integer with the number of the sector
    @return : an array of integers containing the coordinates of the sector in the disk
*/
int* seekCluster(int);

/**
    writeFile();
    Provides the interface to write a file
    @args : Disk pointer, FAT Table pointer, string with the filename
    @return : a double with the time elapsed to write the file
*/
double writeFile(VirtualDisk *, FatTable *, char *);

/**
    readFile();
    Provides the interface to read a file
    @args : Disk pointer, FAT Table pointer, string with the filename
    @return : a double with the time elapsed to read the file
*/
double readFile(VirtualDisk *, FatTable *, char *);

/**
    deleteFile();
    Provides the interface to delete a file
    @args : FAT Table pointer, string with the filename
    @return : a integer meaning the completeness (or not) of the operation
*/
int deleteFile(FatTable *, char *);

/**
    computingTime();
    Calculates the time needed to write or read a file
    @args : array with the clusters used, size of the array
    @return : double with the time used in the operation
*/
double computingTime(int *, int);

/**
    showFATTable();
    Prints the FAT Table on the screen
    @args : FAT Table pointer
*/
void showFATTable(FatTable *);

#endif // CONTROLLER_h
