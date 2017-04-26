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

typedef struct fatfiles{ //Defines the address of
    char file_name[address_size];
    unsigned int first_sector;
}FatFiles;

typedef struct fattable{ //Defines the disk's FAT Table
    FatEnt *entities;
    FatFiles *files;
}FatTable;


VirtualDisk* installDisk();
void uninstallDisk(VirtualDisk *);
FatTable* start_Fat();
void finish_Fat(FatTable *);
unsigned char* createBuffer(char *, int *);
int findCluster(FatTable *);
int* allocateClusters(FatTable *, int);
int* seekCluster(int);
float writeFile(VirtualDisk *, FatTable *, char *);
float readFile(VirtualDisk *, FatTable *, char *);
int deleteFile(FatTable *, char *);
void showFATTable(FatTable *);
float computingTime(int*, int);

#endif // CONTROLLER_h
