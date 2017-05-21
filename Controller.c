#include "Controller.h"

VirtualDisk* installDisk(){
    VirtualDisk *drive = (VirtualDisk *) malloc(sizeof(VirtualDisk));
    if(drive == NULL)
        return damaged_disk;

    drive->disk_cylinders = (Cylinder *) malloc(tracks_per_surface*sizeof(Cylinder));
    if(drive->disk_cylinders == NULL)
        return damaged_disk;

    return drive;
}

void uninstallDisk(VirtualDisk *disk){
    free(disk->disk_cylinders);
    free(disk);
}

FatTable* startFat(){
    int i;

    FatTable *fat = (FatTable *) malloc(sizeof(FatTable));
    if(fat == NULL)
        return corrupted_fat;

    fat->files = (FatFiles *) malloc(sizeof(FatFiles));
    strcpy(fat->files[0].file_name, disk_name);
    fat->files[0].first_sector = 0;

    fat->entities = (FatEnt *) malloc(sectors_amount*sizeof(FatEnt));
    if(fat->entities == NULL)
        return corrupted_fat;
    for(i=0; i<sectors_amount; i++){
        fat->entities[i].used = available;
        fat->entities[i].eof = not_eof_flag;
        fat->entities[i].next = missing_file;
    }
    return fat;
}

void finishFat(FatTable *fat){
    free(fat->entities);
    free(fat->files);
    free(fat);
}

unsigned char* createBuffer(char *file_name, int *buffer_size){
    int file_size = 0, set_ending = 0;
    unsigned char *buffer;
    char current_char;
    FILE *input_file;

    input_file = fopen(file_name, "r");
    if(input_file == ptr_missing_file)
        return ptr_missing_file;

    while(!feof(input_file)){
        current_char = getc(input_file);
        if(current_char == '\n'){
            file_size+=2;
            set_ending++;
        }
        else
            file_size++;
    }

    *buffer_size = file_size;
    fseek(input_file, 0, SEEK_SET);
    buffer = (unsigned char*) malloc(file_size*sizeof(unsigned char));
    if(buffer == ptr_disk_full)
        return ptr_disk_full;
    fread(buffer, file_size, sizeof(unsigned char*), input_file);
    buffer[file_size-set_ending-1] = '\0';
    fclose(input_file);

    return buffer;
}

int findCluster(FatTable *fat){
    int i;

    for(i=0; i<sectors_amount; i+=cluster_size){
        if(fat->entities[i].used == available){
            fat->entities[i].used = unavailable;
            return i;
        }
    }
    return disk_full;
}

void freeClusters(FatTable *fat, int *clusters, int quantity){
    int i;
    for(i=0; i<quantity; i++){
        fat->entities[clusters[i]].used = available;
    }
}

int* allocateClusters(FatTable *fat, int quantity){
    int *clusters_allocated = (int *) malloc(quantity*sizeof(int));
    int i;

    for(i=0; i<quantity; i++){
        clusters_allocated[i] = findCluster(fat);
        if(clusters_allocated[i]==disk_full){
            freeClusters(fat, clusters_allocated, i);
            free(clusters_allocated);
            return ptr_disk_full;
        }
    }
    return clusters_allocated;
}

int* seekCluster(int cluster_id){
    int *cluster_address = (int *) malloc(3*sizeof(int));
    cluster_address[0] = (int) floor(cluster_id/sectors_per_cylinder);
    cluster_address[1] = (int) floor((cluster_id-(cluster_address[0]*sectors_per_cylinder))/sectors_per_track);
    cluster_address[2] = (int) cluster_id - ((cluster_address[0]*sectors_per_cylinder)+(cluster_address[1]*sectors_per_track));
    return cluster_address;
}

double writeFile(VirtualDisk *drive, FatTable *fat, char *file_name){
    unsigned char *buffer;
    int buffer_size, buffer_position = 0;
    int i, l, k, j;
    int *clusters_allocated, clusters_needed, previous_cluster;
    int *p;
    double write_time = 0;

    buffer = createBuffer(file_name, &buffer_size);
    if(buffer == ptr_missing_file){
        return writing_failed;
    }

    clusters_needed = (int) ceil(buffer_size/cluster_memory);
    clusters_allocated = allocateClusters(fat, clusters_needed);
    if(clusters_allocated == ptr_disk_full)
        return disk_full;

    write_time = computingTime(clusters_allocated, clusters_needed);

    fat->files[0].first_sector++;
    fat->files = (FatFiles *) realloc(fat->files,(fat->files[0].first_sector+1)*sizeof(FatFiles));
    strcpy(fat->files[fat->files[0].first_sector].file_name, file_name);
    fat->files[fat->files[0].first_sector].first_sector = clusters_allocated[0];

    for(i=0; i<clusters_needed; i++){
    	if(i!= 0)
    		fat->entities[previous_cluster].next = clusters_allocated[i];

        previous_cluster = clusters_allocated[i];

        for(l=0; l<cluster_size; l++){
            p = (int *)seekCluster(clusters_allocated[i]+l);
            for(k = buffer_position , j=0; ; k++ , j++){
                drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes[j] = buffer[k];
                if(k==buffer_size || j==(sector_size-2))
                    break;
            }
            buffer_position = k+1;
            if(j == (sector_size-2)){
                drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes[sector_size-1] = '\0';
                fat->entities[previous_cluster].next = clusters_allocated[i]+l;
                previous_cluster = clusters_allocated[i]+l;
            }
            if(k == buffer_size){
                drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes[j] = '\0';
                fat->entities[previous_cluster].next = clusters_allocated[i]+l;
                fat->entities[clusters_allocated[i]+l].eof = eof_flag;
                break;
            }
            free(p);
        }
    }
    free(p);
    free(clusters_allocated);
    free(buffer);

    return write_time;
}

double readFile(VirtualDisk *drive, FatTable *fat, char *file_name){
    unsigned char *cluster_buffer = (unsigned char *) malloc(sector_size*sizeof(unsigned char));
    char output_name[address_size];
    int i, current_sector, files_amount = fat->files[0].first_sector;
    int *sectors_used, sectors_needed = 1, cluster_flag = 1;
    int *p;
    FILE *output_file;
    double read_time = 0;

    for(i=1; i<=files_amount; i++){
        if(strcmp(file_name, fat->files[i].file_name)==0)
            break;
    }
    if(strcmp(file_name, fat->files[i].file_name)!=0 || files_amount == 0)
        return missing_file;

    current_sector = fat->files[i].first_sector;
    sectors_used = (int *) malloc(sectors_needed*sizeof(int));
    sectors_used[0] = current_sector;
    strcpy(output_name, "Output_");
    strcat(output_name, file_name);
    output_file = fopen(output_name, "w");

    while (fat->entities[current_sector].eof != eof_flag){
        p = seekCluster(current_sector);

        if(cluster_flag == cluster_size){
            sectors_used = (int *) realloc(sectors_used, (sectors_needed+1)*sizeof(int));
            sectors_used[sectors_needed] = current_sector;
            sectors_needed++;
            cluster_flag = 0;
        }

        strcpy(cluster_buffer, drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes);
        fprintf(output_file, "%s", cluster_buffer);
        current_sector = fat->entities[current_sector].next;
        cluster_flag++;
        free(p);
    }

    p = seekCluster(current_sector);
    sectors_used[sectors_needed-1] = current_sector;
    strcpy(cluster_buffer, drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes);
    fprintf(output_file, "%s", cluster_buffer);
    cluster_buffer = (unsigned char *) memset(cluster_buffer, '\0', sector_size);
    free(p);

    read_time = computingTime(sectors_used, sectors_needed);

    free(sectors_used);
    free(cluster_buffer);
    fclose(output_file);

    return read_time;
}

int deleteFile(FatTable *fat, char *file_name){
	int i, files_amount = fat->files[0].first_sector;
	int current_sector;

	for(i=1; i<=files_amount; i++){
        if(strcmp(file_name, fat->files[i].file_name)==0)
            break;
    }
    if(strcmp(file_name, fat->files[i].file_name)!=0)
        return missing_file;

    current_sector = fat->files[i].first_sector;
    strcpy(fat->files[i].file_name, fat->files[fat->files[0].first_sector].file_name);
    fat->files[i].first_sector = fat->files[fat->files[0].first_sector].first_sector;
    fat->files[0].first_sector--;

    while(fat->entities[current_sector].eof != eof_flag){
    	fat->entities[current_sector].used = available;
    	current_sector = fat->entities[current_sector].next;
    }
    fat->entities[current_sector].used = available;
    fat->entities[current_sector].eof = not_eof_flag;

    return succeed;
}

void showFATTable (FatTable *fat){
	int i, files_amount = fat->files[0].first_sector;
	int current_sector, memory_used  = 0;

	printf("Mostrando a Tabela FAT de: %s\n", fat->files[0].file_name);
	printf("Quantidade de arquivos gravados: %d\n\n", files_amount);
	for(i=1; i<=files_amount; i++){
		printf("\nNome do arquivo: %s\n", fat->files[i].file_name);
		printf("Setores utilizados: ");
		current_sector = fat->files[i].first_sector;
		while(fat->entities[current_sector].eof != eof_flag){
			printf("%d, ", current_sector);
			memory_used++;
			current_sector = fat->entities[current_sector].next;
		}
		memory_used++;
		printf("%d.", current_sector);
		printf("\nMemoria utilizada: %d bytes\n", memory_used*sector_size);
		memory_used = 0;
	}
}

double computingTime(int *sectors, int n){
    int i;
    double time = 0.0;

    for(i=0; i<(n-1); i++){
        time += (double) (cluster_size*(average_latency + (transfer_time/sectors_per_track)));

        if((sectors[i+1]-sectors[i])>300)
            time += average_seek;
    }
    time += (double) (cluster_size*(average_latency + (transfer_time/sectors_per_track)));

    return time;
}
