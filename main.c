#include "Controller.h"

int main(int argc, char **argv){


    VirtualDisk *disk0 = (VirtualDisk *) installDisk();
    FatTable *disk_fat0 = (FatTable *) startFat();
    double time;
    int flag;
    char option;
    char file_name[address_size];

    do{
    	system("cls");
    	printf("\n1. Gravar um arquivo\n2. Ler um arquivo\n3. Deletar um arquivo\n4. Mostrar a tabela FAT do disco\n5. Sair\n");
    	scanf("%c", &option);
    	getchar();
    	switch(option){
    		case '1':
    			printf("Insira o nome do arquivo a ser gravado: ");
    			gets(file_name);
    			time = writeFile(disk0, disk_fat0, file_name);
    			if(time == writing_failed)
                    printf("O arquivo nao pode ser gravado\n");
    			else if(time == disk_full)
                    printf("O disco esta cheio\n");
                else
                    printf("O arquivo foi gravado com sucesso\nTempo de escrita: %.3lf milisegundos\n", time);
                system("PAUSE");
    		break;
    		case '2':
    			printf("Insira o nome do arquivo a ser lido: ");
    			gets(file_name);
    			time = readFile(disk0, disk_fat0, file_name);
                if(time == missing_file)
                    printf("O arquivo nao foi encontrado ou esta corrompido\n");
    			else
                    printf("Um arquivo com o identificador Output_filename foi gerado\nTempo de leitura: %.3lf milisegundos\n", time);
                system("PAUSE");
    		break;
    		case '3':
    			printf("Insira o nome do arquivo a ser deletado: ");
    			gets(file_name);
    			flag = deleteFile(disk_fat0, file_name);
    			if(flag == missing_file)
                    printf("O arquivo nao foi encontrado ou esta corrompido\n");
                else
                    printf("O arquivo foi deletado com sucesso\n");
                system("PAUSE");
    		break;
    		case '4':
    			showFATTable(disk_fat0);
    			system("PAUSE");
    		break;
    		case '5':
    		break;
    		default:
    			printf("Opcao invalida\n");
    			system("PAUSE");
    		break;
    	}
	} while (option != '5');
	printf("Encerrando...\n");

    uninstallDisk(disk0);
    finishFat(disk_fat0);

    return 0;
}
