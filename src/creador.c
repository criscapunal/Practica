#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include <unistd.h>
#include <ctype.h>

//Declaracion de Constantes
const int numeroMascotas=10000000;								
char archivo_nombres[100] = "../etc/nombresMascotas.txt";
char archivo_data[32] = "../etc/dataDogs.dat";

//Estructura Principal
struct dogType {
	int id;
	char nombre[32];
	char tipo[32];
	int edad;
	char raza[16];
	int estatura;
	float peso;
	char sexo;
};

//Nombres Aleatorios
char *nombreAleatorio (char nombres[1717][32]){					
	char *nombre = malloc(32);
	int random = rand() %1717;                            
	strcpy(nombre, nombres[random]); 				
	return nombre;
}

//Tipos Aleatorios
char *tipoAleatorio(char tipos[10][32]) {
	int random2 = rand() %10;
	char *tipo = malloc(32);
	strcpy(tipo, tipos[random2]);
	return tipo;
}

//Razas Aleatorios
char *razaAleatorio(char razas[2][16]) {
	int random3 = rand() %2;
	char *raza = malloc(16);
	strcpy(raza, razas[random3]);
	return raza;
}

//Sexo Aleatorio
char sexoAleatorio() {
	int random4 = rand() %2;
	if (random4 == 0) {
		return 'M';	
	} else if (random4 == 1) {
		return 'H';
	} else {
		return 'E';
	}
}

//Creador Genearal de Estructura Aleatoria
struct dogType creador(char nombres[1717][32], char tipos[10][32], char razas[2][16], int id) {
	struct dogType auxiliar;

	auxiliar.id = id;

	char *nombre = nombreAleatorio(nombres);
	strcpy(auxiliar.nombre, nombre);
	free(nombre);

	char *tipo = tipoAleatorio(tipos);
	strcpy(auxiliar.tipo, tipo);
	free(tipo);

	char *raza = razaAleatorio(razas);
	strcpy(auxiliar.raza, raza);
	free(raza);

	auxiliar.edad = 1+(rand() % 14);
	auxiliar.estatura = 1+(rand() % 99);
	auxiliar.peso = (1+(rand() % 400))/10.0;
	auxiliar.sexo = sexoAleatorio();

	return auxiliar;	
}


int main() {
	//Inicializa rand
	srand(time(NULL));						
	
	FILE *file;
	FILE *id;
	FILE *dataDog;					
	char buffer_nombres [1717][32];	
	char tipos[10][32]= {"Perro", "Gato", "Conejo", "Tortuga", "Hamster", "Loro", "Pez", "Perico", "Pato", "Pollo"};
	char razas[2][16]= {"Puro", "Criollo"};					
	int i = 0;
	int j = 0;
	int k = 0;

	//Leer archivo y guardar nombres de mascotas en arreglo.
	file = fopen(archivo_nombres,"r");
	while(feof(file) == 0){
		fscanf(file, "%s" , buffer_nombres[i]);	   
		i++;
	}
	fclose(file);											
	
	//Convertir a mayusculas
	for (i = 0; i < 1717; i++) {
		for (j = 0; j < 32; j++) {
			buffer_nombres[i][j] = toupper(buffer_nombres[i][j]);
		}
	}

	printf("%s\n","Por favor espere un momento mientras se generan los registros...");
			
	dataDog = fopen(archivo_data,"a+");
	if(dataDog == NULL){
    	perror("Error Archivo dataDog");
    	exit(-1);
    }  

    //Genera un números especifico de mascotas aleatorias
	for (k = 0; k < numeroMascotas; k++) {
		
		struct dogType *auxiliar = malloc(sizeof(struct dogType));
		*auxiliar = creador(buffer_nombres, tipos, razas, k+1);		
		fwrite (auxiliar, sizeof(struct dogType), 1, dataDog);
		free(auxiliar);	
		
	}
	fclose(dataDog);

}