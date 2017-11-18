#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h> 
#include <time.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define BACKLOG 32
#define PORT 3535
#define MAX_PROCESOS 32


// Declaración de Constantes

struct Nodo *hashTable[3000];

char nombreArchivo[32] = "../etc/dataDogs.dat";
char nombreArchivo2[32] = "../etc/dataDogs2.dat";
char archivoHistorial[32] = "../etc/historial.txt";
char archivoLog[32] = "../etc/serverDogs.log";
char nombreBuscar[32];
long cantidadMascotasTotal;
int condicionActualizar = 0;

//Variables Manejo de Concurrencia
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *semaforo;
int pipefd[2], r;
pid_t pid;
char buffer;

int serverfd;

long indiceID = 10000000;

void log1(char ip[32], int option, char reg[32]) {
	FILE *fd = fopen(archivoLog, "a");
	time_t tiempo = time(0);
	struct tm *tlocal = localtime(&tiempo);
	char date[128];
	strftime(date, 128, "%Y/%m/%d - %H:%M:%S", tlocal);
	char option_char[32];

	switch(option){
	  case 1:
		strcpy(option_char, "inserción");
		break;
	  case 2:
		strcpy(option_char, "lectura");
		break;
	  case 3:
		strcpy(option_char, "borrado");
		break;
	  case 4:
		strcpy(option_char, "búsqueda");
		break;
	  case 5:
		strcpy(option_char, "salida");
		break;
	  default:
		strcpy(option_char, "opción invalida");
		break;
	}

	char description[128];
	strcpy(description, "| ");
	strncat(description, date, 32);
	strncat(description, " | ", 32);
	strncat(description, ip, 32);
	strncat(description, " | ", 32);
	strncat(description, option_char, 32);
	strncat(description, " | ", 32);
	strncat(description, reg, 32);
	strncat(description, " | \n\n", 32);
  	int sek = fseek(fd, 0, SEEK_END);
    //int wr = fwrite (description, sizeof(char[128]), 1, fd);

    fprintf(fd, "%s" , description);
    fclose(fd);
}



//............ Practica Uno ..........................................//

//Estructura Identificador.
struct identificador {
	char nombre[32];
	long posicion;
};

//Estructura DogType.
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

// Cantidad de mascotas en el archivo dogType
long cantidadMascotasArchivo(char archivo[32]){
	FILE *file;
	long cant;

	file = fopen(archivo, "r");
	fseek (file, 0, SEEK_END); 
	cant = ftell(file) / (sizeof(struct dogType));
	fclose(file);
	return cant;
}

// Cantidad de mascotas en el archivo Identificador
long cantidadMascotasArchivo2(char archivo[32]){
	FILE *file;
	long cant;

	file = fopen(archivo, "r");
	fseek (file, 0, SEEK_END); 
	cant = ftell(file) / (sizeof(struct identificador));
	fclose(file);
	return cant;
}

//               ................Hash..............                     //
//........................................................................

//Nodo de la Hash
struct Nodo {
	int posicion;
	char borrado;
	struct Nodo *next; 
};

//Generador de posicion en Hash
int getHash(char nombre[32]){

	int index = 0; 
	int i = 0; 
	int indexHash = 0;
    int tamNombre = strlen(nombre);
    char auxNombre[tamNombre+1];

    for ( i=0; nombre[i]; i++) {
       auxNombre[i] = toupper(nombre[i]);
    }

    auxNombre[i]= '\0'; 

    for (i = 0;i < 32;i++) {
		if (auxNombre[i]=='\0') {
	    	break;
		}
        index += (( (int) auxNombre[i])*(i+1));
    }

    indexHash = index%3000;

    if (indexHash < 0) {
		indexHash = indexHash*(-1);
    }

    return indexHash;
}

//Ingresa datos nuevos en Hash
  int ingresarHash(struct identificador id){

  	struct Nodo *nuevo = malloc(sizeof(struct Nodo));
    nuevo->posicion = id.posicion;

    //Se asigna posicion en el Hash
    int index = getHash(id.nombre);

    if (hashTable[index] == NULL) { // EL PROBLEMA ESTA AQUÍ
        nuevo->next = NULL;
        hashTable[index] = nuevo;
    } else {
        nuevo->next = hashTable[index];
        hashTable[index] = nuevo;
    }

    return index;
}


//Busqueda de estructuras con "nombre" igual en Hash
void buscarHash(char nombre[32], int clientfd) {
        
        int index;
		int contador = 0;
		int pos;
		struct dogType auxiliar;

        FILE *file;            

        //Busco posicion de "nombres" en Hash
        index = getHash(nombre);

        if (hashTable[index] != NULL) {
            struct Nodo *nodo = hashTable[index];
			file = fopen(nombreArchivo,"r");
			
			
            while(nodo != NULL) {

            	fseek (file, nodo->posicion, SEEK_SET);
				fread(&auxiliar, sizeof(struct dogType), 1, file); 

				pos = (((nodo->posicion)/100)+1);
                
               	if ((strcmp(nombre, auxiliar.nombre) == 0) && (nodo->borrado != 'b')) {
               		
					contador++;
					send(clientfd, &pos, sizeof(int), 0);					
					send(clientfd, &auxiliar, sizeof(struct dogType), 0);					

				}
				nodo = nodo-> next;
			}
			auxiliar.nombre[0] = '\0';

			send(clientfd, &pos, sizeof(int), 0);
			send(clientfd, &auxiliar, sizeof(struct dogType), 0);

			fclose(file);  
		} 
}


//.........................................................................

//Espera ENTER de confirmacion
void continuar() {
    char tecla;
    printf("%s\n", "Proceso Realizado Exitosamente\nPresione ENTER para continuar ... ");
    scanf("%c", &tecla);
    getchar();
}

//Espera ENTER de confirmacion
void continuar2() {
	char tecla;
    printf("%s\n", "Presione ENTER para continuar ... ");
    scanf("%c", &tecla);
    fflush(stdin);
    getchar();
}


// Ingresa la nueva estructura en el archivo
void ingresarEnArchivo(char ingresarEnArchivo[32], void *mascota) {
	struct dogType *mascotaIngresar;
	struct identificador id;
	mascotaIngresar = mascota;
	FILE *file;
	FILE *file2;

	//Guarda la estructura con el puntero ubicado en el final del archivo.
	file = fopen(ingresarEnArchivo,"a");
	fwrite(mascotaIngresar, sizeof(struct dogType), 1, file);
	fclose(file);
}

// Registro de datos
void crearRegistro(void *x) {
	struct dogType *nuevo;
	nuevo = x;

	indiceID++;

	nuevo->id = indiceID;
    //Guarda la estructura en el archivo.
    ingresarEnArchivo(nombreArchivo, nuevo);

    //Guarda la estructura en las Hash.
    struct identificador id;
    strcpy(id.nombre, nuevo->nombre);
    id.posicion = (cantidadMascotasArchivo(nombreArchivo)-1)*sizeof(struct dogType);                
    ingresarHash(id);
    //printf("%s\n", "muerome");
}



void verHistorial(int clientfd, long numRegistro){

	struct dogType auxiliar;
	FILE *file;
	int id;
	int borrado;
	//Lee estrutura de archivo y guarda en auxiliar.
	file = fopen(nombreArchivo,"r+");
	fseek (file,(numRegistro-1)*sizeof(struct dogType), SEEK_SET);
	fread(&auxiliar, sizeof(struct dogType), 1, file); 
	fclose(file);

	if (auxiliar.nombre[0] == '\0') {
		borrado = 1;
		send(clientfd, &borrado, sizeof(int), 0);		
	} else {
		borrado = 0;
		send(clientfd, &borrado, sizeof(int), 0);	
		id  = auxiliar.id;
		char nombreHistorial[21] = "../etc/";
		char nombreHistorial1[14];
		sprintf(nombreHistorial1, "%d", id);
		strncat(nombreHistorial1, ".txt", 4);
		strncat(nombreHistorial, nombreHistorial1, 14);
	
		FILE *archivo;
		int sz;
		int caracter;
		
		archivo = fopen(nombreHistorial,"r");
	
		if (archivo == NULL) {
			archivo = fopen(nombreHistorial,"w+");
			fprintf(archivo, "%s %s %i %s %i %0.2f %c" , auxiliar.nombre, auxiliar.tipo, auxiliar.edad,auxiliar.raza, auxiliar.estatura, auxiliar.peso,auxiliar.sexo);
		}
	
	
		fseek(archivo, 0L, SEEK_END);
		sz = ftell(archivo);
		fseek(archivo, 0L, SEEK_SET);
		//printf("%i\n", sz);
		send(clientfd, nombreHistorial1, sizeof(char)*14, 0);
		send(clientfd, &sz, sizeof(int), 0);
	
		while((caracter = fgetc(archivo)) != EOF) {
			send(clientfd, &caracter, sizeof(int), 0);
		}
	
		fclose(archivo);
		sz = 0;
		
		archivo = fopen(nombreHistorial,"w");
	
		recv(clientfd, &sz, sizeof(int), 0);
		fseek(archivo, 0L, SEEK_SET);
		//printf("%i\n", sz);
		for (int i = 0; i < sz; i++) {
			recv(clientfd, &caracter, sizeof(int), MSG_WAITALL);
			fputc(caracter, archivo);
		}
		
		fclose(archivo);
		archivo = fopen(nombreHistorial,"r");

		fscanf(archivo, "%s %s %i %s %i %f %c", auxiliar.nombre, auxiliar.tipo, &auxiliar.edad,auxiliar.raza, &auxiliar.estatura, &auxiliar.peso,&auxiliar.sexo);
		
		fclose(archivo);

		file = fopen(nombreArchivo,"r+");
		
		pthread_mutex_lock(&mutex);
		//Sobreescribe las estructura en el archivo.
		fseek (file,(numRegistro-1)*sizeof(struct dogType), SEEK_SET);
		fwrite(&auxiliar, sizeof(struct dogType), 1, file);
		fclose(file);
		pthread_mutex_unlock(&mutex);
	}
	

}


//Permite ver el historial de una estructura especifica.
void verRegistro(int clientfd, long numeroRegistro) {

	struct dogType auxiliar;
	FILE *file;

	//recv(clientfd, &auxiliar, sizeof(struct dogType), 0);

	//Lee estrutura de archivo y guarda en auxiliar.
	file = fopen(nombreArchivo,"r+");
	fseek (file,(numeroRegistro-1)*sizeof(struct dogType), SEEK_SET);
	fread(&auxiliar, sizeof(struct dogType), 1, file); 

	send(clientfd, &auxiliar, sizeof(struct dogType), 0);
	//printf("%s\n", "enviado");
	fclose(file);

	if (auxiliar.nombre[0] == '\0') {

	} else {
		file = fopen(nombreArchivo,"r+");

		recv(clientfd, &auxiliar, sizeof(struct dogType), MSG_WAITALL);
		//printf("%s\n", "recibido");

		pthread_mutex_lock(&mutex);
		//Sobreescribe las estructura en el archivo.
		fseek (file,(numeroRegistro-1)*sizeof(struct dogType), SEEK_SET);
		fwrite(&auxiliar, sizeof(struct dogType), 1, file);
		fclose(file);
		pthread_mutex_unlock(&mutex);
	}

	int confirmacion = 1;
	send(clientfd, &confirmacion, sizeof(int), 0);
}


struct dogType vaciarEstructura(long numeroRegistro) {

	FILE *file;
	struct dogType auxiliar = {0, '\0', '\0', 0, '\0', 0, 0, '\0'};
	struct dogType auxiliar2;

	file = fopen(nombreArchivo, "r+");
	fseek (file,(numeroRegistro-1)*sizeof(struct dogType), SEEK_SET);
	fread(&auxiliar2, sizeof(struct dogType), 1, file);

	fseek (file,(numeroRegistro-1)*sizeof(struct dogType), SEEK_SET);
	fwrite(&auxiliar, sizeof(struct dogType), 1, file);
	fclose(file);
	return auxiliar2;
}

void asignarBorradoHash(struct dogType mascota, long numeroRegistro) {

    int index = getHash(mascota.nombre);
    struct Nodo *nodo = hashTable[index];

    if (nodo->posicion == (numeroRegistro-1)*sizeof(struct dogType)) {
        nodo->borrado = 'b';

    //Recorre y busca el nodo para asignar borrado.
    } else {
        while(nodo != NULL) {
            if (nodo->posicion == (numeroRegistro-1)*sizeof(struct dogType)) {
                nodo->borrado = 'b';
            }
            nodo = nodo-> next;
        }
    }
}


//Borra registro del arvhivo y la Hash.
void actualizar(){

	FILE *file;
	FILE *file2;
	struct dogType *mascota = malloc(sizeof(struct dogType));
	long cantidadMascotas = cantidadMascotasArchivo(nombreArchivo);

	//printf("%s\n", "estoy actualizando");

	file = fopen(nombreArchivo, "r");
	file2 = fopen(nombreArchivo2, "w+");

	for (int i = 0; i < cantidadMascotas; ++i) {
		fread(mascota, sizeof(struct dogType), 1, file);

		if (mascota->nombre[0] == '\0') {
			continue;

		//Copia el resto de estructuras al archivo temporal.
		} else { 
			fwrite(mascota, sizeof(struct dogType), 1, file2);
		}

	}
	free(mascota);
	fclose(file);
	fclose(file2);
	system("mv ../etc/dataDogs2.dat ../etc/dataDogs.dat");
	//system("mv dataDogs2.dat dataDogs.dat");
}



//.................................................................................................//

void menu(int clientfd, struct sockaddr_in client, char ip[32]) {
	//system("clear");
	printf("%s\n", "Esperando cliente...");

	int opc = 0;

	recv(clientfd,&opc, sizeof(int), MSG_WAITALL);

	if (opc == 1) {
		log1(ip, opc, " ... ");
		struct dogType *nuevo;
		nuevo = malloc(sizeof(struct dogType));

		if(nuevo == NULL){
			perror("error en malloc");
			exit(-1);	
		}

		recv(clientfd, nuevo, sizeof(struct dogType), MSG_WAITALL);

     	r = read(pipefd[0], &buffer, 1);

     	crearRegistro(nuevo);

     	r = write(pipefd[1], "T", 1);

     	free(nuevo);

     	int confirmacion = 1;
     	send(clientfd, &confirmacion, sizeof(int), 0);

     	cantidadMascotasTotal++;  	     

     	menu(clientfd, client, ip);

     	system("clear");
	    
	} else if (opc == 2) {
		int numeroReg;
		char numreg[32];
		send(clientfd, &cantidadMascotasTotal, sizeof(long), 0);

		recv(clientfd, &numeroReg, sizeof(long), MSG_WAITALL);
		verHistorial(clientfd, numeroReg);
		sprintf(numreg, "%i", numeroReg);
		log1(ip, opc, numreg);

		menu(clientfd, client, ip);
	} else if (opc == 3) {
		long numRegistro;
		int confirmacion = 1;
		char numreg[32];
		char nombreHistorial[12];
		char borrarHistorial[25] = "rm ../etc/"; 
		int id;

		struct dogType mascota;

		send(clientfd, &cantidadMascotasTotal, sizeof(long), 0);

		recv(clientfd, &numRegistro, sizeof(long), MSG_WAITALL);

		mascota = vaciarEstructura(numRegistro);
	    asignarBorradoHash(mascota, numRegistro);

	    system("clear");
		cantidadMascotasTotal--;
		
		id  = mascota.id;
		sprintf(nombreHistorial, "%d", id);
		strncat(nombreHistorial, ".txt", 4);
		strncat(borrarHistorial, nombreHistorial, 12);

		system(borrarHistorial);   
		
	   	sprintf(numreg, "%lu", numRegistro);
		log1(ip, opc, numreg);

		send(clientfd, &confirmacion, sizeof(int), 0);		

	    menu(clientfd, client, ip);
	} else if (opc == 4) {
		int confirmacion = 1;
		char nombre[32];
		recv(clientfd, &nombre, 33, MSG_WAITALL);

		buscarHash(nombre, clientfd);

		log1(ip, opc, nombre);

		send(clientfd, &confirmacion, sizeof(int), 0);

		menu(clientfd, client, ip);

	} 
}

void* recibirCliente(void *cliente){
	int clientfd;
	struct sockaddr_in client;
	socklen_t sizecli;
	char ip[32];

	clientfd = accept(serverfd,(struct sockaddr *)&client,&sizecli);
	if(clientfd == -1){perror("error accept");}

	strcpy(ip, inet_ntoa(client.sin_addr));
	//printf("%s\n", ip);
	menu(clientfd, client, ip);

	close(clientfd);

}

void prepararServer(){	
	int z;
		
	pthread_t hilo[BACKLOG];
	struct sockaddr_in server;
	socklen_t size = sizeof(struct sockaddr);

	serverfd = socket(AF_INET,SOCK_STREAM,0);
	if(serverfd == -1){perror("error socket");}

	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;
	bzero(server.sin_zero,8);

	z = bind(serverfd,(struct sockaddr *)&server,size);
	if(z == -1){perror("error bind");}
	
	z = listen(serverfd,BACKLOG);
	if(z == -1){perror("error listen");}

	semaforo = sem_open("semaforo_name", O_CREAT, 0700, MAX_PROCESOS);

	r = pipe(pipefd);
	r = write(pipefd[1], "T", 1);

	for(int i = 0; i < BACKLOG; i++){
		int r = pthread_create(&hilo[i], NULL, recibirCliente, (void*)&i);
		if(r != 0){
			perror("error en el hilo ");
		}
	}

	printf("%s\n", "Esperando cliente...");

	for(int i = 0; i < BACKLOG; i++){
		int r =  pthread_join(hilo[i], NULL);
		if(r != 0){
			perror("error en el hilo ");
		}
	}	

	close(serverfd);
}

void signalHandler( int signum ) {

	printf("%s\n", "Por favor espere un momento mientras se actualizan los datos");
	printf("%s\n", "Esto puede tardar unos segundos ...");
	actualizar();
	system("clear");
	//printf("%s\n", "Esperando cliente...");
	exit(signum);  
}

int main(){

	FILE *fileHash2;
	
	//struct Nodo *hashTable[sizeHash];
	struct identificador auxiliar;
	int opcionMenu = 0;
	int numRegistro;

	cantidadMascotasTotal = cantidadMascotasArchivo(nombreArchivo);

	printf("%s\n", "Cargando Datos ... ");
	for (int i = 0; i < 3000; ++i) {
		hashTable[i] = NULL;
	}

	fileHash2 = fopen(nombreArchivo,"r");
	
	for (int j = 0; j < 10000000; j++) {
		struct dogType *mascota = malloc(sizeof(struct dogType));
		struct identificador *id = malloc(sizeof(struct identificador));
		fread(mascota, sizeof(struct dogType), 1, fileHash2); 
		strcpy(id->nombre, mascota->nombre);
		id->posicion = j*sizeof(struct dogType);
		ingresarHash(*id);
		free(id);
		free(mascota);
	}

	fclose(fileHash2);
	system("clear");

	signal(SIGINT, signalHandler);

	prepararServer();

	sem_close(semaforo);
	sem_unlink("semaforo_name");
	pthread_mutex_destroy(&mutex);

	return 0;
}