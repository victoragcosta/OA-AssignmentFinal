#include "Controller.h"

VirtualDisk* installDisk(){
    VirtualDisk *drive = (VirtualDisk *) malloc(sizeof(VirtualDisk)); //Aloca o disco
    if(drive == NULL)
        return damaged_disk;

    drive->disk_cylinders = (Cylinder *) malloc(tracks_per_surface*sizeof(Cylinder)); //Aloca todos os cilindros do disco
    if(drive->disk_cylinders == NULL)
        return damaged_disk;

    return drive;
}

void uninstallDisk(VirtualDisk *disk){ //Libera toda a memória do disco
    free(disk->disk_cylinders);
    free(disk);
}

FatTable* start_Fat(){ //Aloca e inicializa a FAT
    int i;
    FatTable *fat = (FatTable *) malloc(sizeof(FatTable));
    if(fat == NULL)
        return corrupted_fat;

    fat->files = (FatFiles *) malloc(sizeof(FatFiles));
    strcpy(fat->files[0].file_name, disk_name);
    fat->files[0].first_sector = 0; //The first space will hold info about the disk, such as name and amount of files written

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

void finish_Fat(FatTable *fat){ //Libera a memória da FAT
    free(fat->entities);
    free(fat->files);
    free(fat);
}

unsigned char* createBuffer(char *file_name, int *buffer_size){
    int file_size = 0, set_ending = 0; //Counts the length of the file
    unsigned char *buffer;
    char current_char;
    FILE *input_file;

    input_file = fopen(file_name, "r"); //Tenta abrir o arquivo
    if(input_file == ptr_missing_file)
        return ptr_missing_file;

    while(!feof(input_file)){ //Conta a quantidade de caracteres do arquivo
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
    buffer = (unsigned char*) malloc(file_size*sizeof(unsigned char)); //Cria um buffer do tamanho do arquivo
    if(buffer == ptr_disk_full)
        return ptr_disk_full;
    fread(buffer, file_size, sizeof(unsigned char*), input_file); //Le tudo para o buffer
    buffer[file_size-set_ending-1] = '\0';
    fclose(input_file);

    return buffer;
}

int findCluster(FatTable *fat){ //Localiza um cluster vazio
    int i, j;
    
    for(i=0; i<sectors_amount; i+=cluster_size){
        if(fat->entities[i].used == available){
            fat->entities[i].used = unavailable;
            return i;
        }
    }
    return disk_full;
}

int* allocateClusters(FatTable *fat, int quantity){ //Aloca a quantidade de clusters necessaria para guardar o arquivo e retorna quais clusters serao usados
    int *clusters_allocated = (int *) malloc(quantity*sizeof(int));
    int i; //Iterator

    for(i=0; i<quantity; i++){
        clusters_allocated[i] = findCluster(fat);
        if(clusters_allocated[i]==disk_full){
            free(clusters_allocated);
            return ptr_disk_full;
        }
    }
    return clusters_allocated;
}

int* seekCluster(int cluster_id){ //Localiza um cluster dentro do disco, em termos de coordenadas de cilindro, trilha e setor
    int *cluster_address = (int *) malloc(3*sizeof(int));
    cluster_address[0] = (int) floor(cluster_id/sectors_per_cylinder); //Cilindro p[0]
    cluster_address[1] = (int) floor((cluster_id-(cluster_address[0]*sectors_per_cylinder))/sectors_per_track); //Trilha p[1]
    cluster_address[2] = (int) cluster_id - (cluster_address[0]*sectors_per_cylinder)+(cluster_address[1]*sectors_per_track); //Setor p[2]
    return cluster_address;
}


float writeFile(VirtualDisk *drive, FatTable *fat, char *file_name){
    unsigned char *buffer;
    int buffer_size, buffer_position = 0;
    int i, l, k, j; //Iterators
    float write_time = 0; ///Falta implementar a parte de calculo dos tempos
    int *clusters_allocated, clusters_needed; //Clusters access controllers
    int previous_cluster; //Holds the latest written cluster
    int *p; //Coordinates (cil, track, sector) of a cluster

    //Retrieve file and build buffer
    buffer = createBuffer(file_name, &buffer_size);
    if(buffer == ptr_missing_file){
        return writing_failed;
    }
    //Retrieve information from the disk FAT Table

    clusters_needed = (int) ceil(buffer_size/cluster_memory);
    clusters_allocated = allocateClusters(fat, clusters_needed);
    write_time = computingTime(clusters_allocated, clusters_needed);
    if(clusters_allocated == ptr_disk_full)
        return disk_full;

    //Write file in FAT Table

    fat->files[0].first_sector++; //Incrementa a quantidade de arquivos na fat
    fat->files = (FatFiles *) realloc(fat->files,(fat->files[0].first_sector+1)*sizeof(FatFiles));
    strcpy(fat->files[fat->files[0].first_sector].file_name, file_name); // Grava o nome do arquivo na fat
    fat->files[fat->files[0].first_sector].first_sector = clusters_allocated[0]; //Grava o primeiro setor do arquivo

    //Write file in disk

    for(i=0; i<clusters_needed; i++){
    	if(i!= 0)
    		fat->entities[previous_cluster].next = clusters_allocated[i];

        previous_cluster = clusters_allocated[i];

        for(l=0; l<cluster_size; l++){
            p = (int *)seekCluster(clusters_allocated[i]+l);
            for(k=buffer_position , j=0; ; k++ , j++){
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

float readFile(VirtualDisk *drive, FatTable *fat, char *file_name){
    unsigned char *cluster_buffer = (unsigned char *) malloc(sector_size*sizeof(unsigned char));
    int i, current_sector, files_amount = fat->files[0].first_sector; ///Falta implementar a parte de calculo dos tempos
    float  read_time = 0;
    int *p; //Coordinates of a cluster
    FILE *output_file;
    char output_name[address_size];

    //Search FAT Table for file

    for(i=1; i<=files_amount; i++){
        if(strcmp(file_name, fat->files[i].file_name)==0)
            break;
    }
    if(strcmp(file_name, fat->files[i].file_name)!=0 || files_amount == 0)
        return missing_file;

    //Opens the output file

    current_sector = fat->files[i].first_sector;
    strcpy(output_name, "Output_");
    strcat(output_name, file_name);
    output_file = fopen(output_name, "w");

    //Reading the disk

    while (fat->entities[current_sector].eof != eof_flag){
        p = seekCluster(current_sector);
        strcpy(cluster_buffer, drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes);
        fprintf(output_file, "%s", cluster_buffer);
        current_sector = fat->entities[current_sector].next;
        free(p);
    }
    p = seekCluster(current_sector);
    strcpy(cluster_buffer, drive->disk_cylinders[p[0]].cylinder_tracks[p[1]].track_sectors[p[2]].sector_bytes);
    fprintf(output_file, "%s", cluster_buffer);
    cluster_buffer = (unsigned char *) memset(cluster_buffer, '\0', sector_size);
    free(p);

    free(cluster_buffer);
    fclose(output_file);

    return read_time;
}

int deleteFile(FatTable *fat, char *file_name){
	int i, files_amount = fat->files[0].first_sector;
	int current_sector;

	//Procura pelo arquivo - pesquisa linear

	for(i=1; i<=files_amount; i++){
        if(strcmp(file_name, fat->files[i].file_name)==0)
            break;
    }
    if(strcmp(file_name, fat->files[i].file_name)!=0)
        return missing_file;

    //Copia a ultima celula para a celula excluida e diminui a quantidade de arquivos presentes

    current_sector = fat->files[i].first_sector;
    strcpy(fat->files[i].file_name, fat->files[fat->files[0].first_sector].file_name);
    fat->files[i].first_sector = fat->files[fat->files[0].first_sector].first_sector;
    fat->files[0].first_sector--;

    //Acessa as informações dos clusters na FAT e seta-os para disponiveis e remove o EOF

    while(fat->entities[current_sector].eof != eof_flag){
    	fat->entities[current_sector].used = available;
    	current_sector = fat->entities[current_sector].next;
    }
    fat->entities[current_sector].used = available;
    fat->entities[current_sector].eof = not_eof_flag;
}

void showFATTable (FatTable *fat){ //Mostra toda a tabela FAT
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

float computingTime(int *sectors, int n){// Recebe o tamanho e o array de setores no qual o arquivo esta contido
    int i;  //control variables
    float tempo=0.0;
    
    for(i=0; i<(n-1); i++){
        tempo += (float) average_latency + (transfer_time/sectors_per_track);
        
        if((sectors[i+1]-sectors[i])>300)
            tempo += average_seek;
    }
    tempo += (float) average_latency + (transfer_time/sectors_per_track); //Tempo adicional para leitura do ultimo setor
    
    return tempo;
}
