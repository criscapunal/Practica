#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <termios.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h> 


#define PORT 3535
#define BACKLOG 32

int clientfd;

void configurar() {
	int r;
	struct sockaddr_in client;
	socklen_t size;

	clientfd = socket(AF_INET,SOCK_STREAM,0);
	if(clientfd == -1){perror("error socket");}

	client.sin_family = AF_INET;
	client.sin_port = htons(PORT);
	client.sin_addr.s_addr = inet_addr("127.0.0.1");
	bzero(client.sin_zero,8);
	
	size = sizeof(struct sockaddr_in);
	r = connect(clientfd,(struct sockaddr *)&client,size);//establesemos conexion con el servidor
	if(r == -1){perror("error connect");}	
}

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

void continuar2() {
	char tecla;
    printf("%s\n", "Presione ENTER para continuar ... ");
    scanf("%c", &tecla);
    getchar();
}

struct dogType *agregar(){
	
    struct dogType *mascota;
    mascota = malloc(sizeof(struct dogType));



    char nombreAux[32];

	printf("Ingrese el nombre: ");
	fflush(stdin);
    scanf("%s", nombreAux);
    for (int i = 0; nombreAux[i]; i++) {
    	nombreAux[i] = toupper(nombreAux[i]);
    }
    strcpy(mascota->nombre, nombreAux);

    printf("Ingrese el tipo: ");
    fflush(stdin);
    scanf("%s", mascota->tipo);

    printf("Ingrese la edad: ");
    fflush(stdin);
    scanf("%i", &mascota->edad);

    printf("Ingrese la raza: ");
    fflush(stdin);
    scanf("%s", mascota->raza);

    printf("Ingrese la estarura: ");
    fflush(stdin);
    scanf("%i", &mascota->estatura);

    printf("Ingrese el peso: ");
    fflush(stdin);
    scanf("%f", &mascota->peso);

    printf("Ingrese el genero: ");
    fflush(stdin);
    scanf(" %c", &mascota->sexo);

    return mascota;
}; 


struct dogType verRegistro(struct dogType auxiliar) {

	//struct dogType auxiliar;
	struct dogType auxiliar2;
	FILE *file;
	FILE *file2;

	//Escribe los datos de la estrucutura solicitada en el archivo Historial.
	file2 = fopen("historial.txt","w");
	fprintf(file2, "%s %s %i %s %i %0.2f %c" , auxiliar.nombre, auxiliar.tipo, auxiliar.edad,
											   auxiliar.raza, auxiliar.estatura, auxiliar.peso,
											   auxiliar.sexo);
	
	fclose(file2);

	system("gedit historial.txt");

	system("clear");
	
	printf("%s\n","Guardando Cambios ...");
	continuar2();

	//Lee el Historial para tomar actualizar datos.
	file2 = fopen("historial.txt","r");
	fscanf(file2, "%s %s %i %s %i %f %c", auxiliar2.nombre, auxiliar2.tipo, &auxiliar2.edad,
										  auxiliar2.raza, &auxiliar2.estatura, &auxiliar2.peso,
										  &auxiliar2.sexo);

	for (int i = 0; auxiliar2.nombre[i]; i++) {
    	auxiliar2.nombre[i] = toupper(auxiliar2.nombre[i]);
    }

	//fclose(file);
	fclose(file2);

	return auxiliar2;
}



void main() {

	configurar();
	int opc, r;

	while (opc != 5) {

		system("clear");

		printf("%s\n", "Digite el númeo de la opcion que desea seleccionar seguido por un enter");
		printf("%s\n", "Opcion 1: Ingresar Registro");
		printf("%s\n", "Opcion 2: Ver Registro");
		printf("%s\n", "Opcion 3: Borrar Registro");
		printf("%s\n", "Opcion 4: Buscar Registro");
		printf("%s\n", "Opcion 5: Salir");

		scanf("%i", &opc);
		fflush(stdin);

		r = send(clientfd, &opc, sizeof(opc), 0);
		if(r != sizeof(int)){perror("error enviando");}

		if (opc == 1) {
			int confirmacion = 0;
			struct dogType *mascota = agregar();
			send(clientfd, mascota, sizeof(struct dogType), 0);
			recv(clientfd, &confirmacion, sizeof(int), MSG_WAITALL);
			if (confirmacion == 1) {
				printf("%s\n", "Proceso Realizado Exitosamente");
			} else {
				printf("%s\n", "Error");
			}
			continuar2();
			
		} else if (opc == 2) {
			long registros, numRegistro;
			int sz;
			int caracter;
			char nombreHistorial[14];
			int confirmacion = 0;
			FILE *file;
			char buffer[256];
			char abrirHistorial[18] = "gedit ";
			char borrarHistorial[18] = "rm ";
			int borrado;

			recv(clientfd, &registros, sizeof(long), MSG_WAITALL);

	     	printf("Numero de Registros Presentes:  %lu\n",registros);
	     	printf("%s", "Escriba el número de registro que desea ver:");
	     	scanf(" %ld",&numRegistro);
	     	fflush(stdin);

			send(clientfd, &numRegistro, sizeof(long), 0);

			recv(clientfd, &borrado, sizeof(int), MSG_WAITALL);
			if (borrado == 1) {

				printf("El registro fue borrado\n");

			} else {
				recv(clientfd, nombreHistorial, sizeof(char)*14, MSG_WAITALL);
				recv(clientfd, &sz, sizeof(int), MSG_WAITALL);
				//printf("%s\n", nombreHistorial);
				file = fopen(nombreHistorial, "w");
	
				for (int i = 0; i < sz; i++) {
					recv(clientfd, &caracter, sizeof(int), MSG_WAITALL);
					fputc(caracter, file);
				}
	
				fclose(file);
				sz = 0;
				
				strncat(abrirHistorial, nombreHistorial, 14);
				//printf("%s\n", abrirHistorial);
				system(abrirHistorial);
	
				file = fopen(nombreHistorial, "r");
	
	
				fseek(file, 0L, SEEK_END);
				sz = ftell(file);
				fseek(file, 0L, SEEK_SET);
	
				send(clientfd, &sz, sizeof(int), 0);
	
				while((caracter = fgetc(file)) != EOF) {
					send(clientfd, &caracter, sizeof(int), 0);
				}

				strncat(borrarHistorial, nombreHistorial, 12);
				system(borrarHistorial);
	
			}
			
			continuar2();

		} else if (opc == 3) {
			long registros, numRegistro;
			int confirmacion = 0;

			recv(clientfd, &registros, sizeof(long), MSG_WAITALL);

	     	printf("Numero de Registros Presentes:  %lu\n",registros);
	     	printf("%s", "Escriba el número de registro que desea borrar:");
	     	scanf(" %ld",&numRegistro);
	     	fflush(stdin);

	     	send(clientfd, &numRegistro, sizeof(long), 0);

	     	system("clear");

	     	recv(clientfd, &confirmacion, sizeof(int), MSG_WAITALL);

			if (confirmacion == 1) {
				printf("%s\n", "Proceso Realizado Exitosamente");
			} else {
				printf("%s\n", "Error");
			}
			continuar2();	     	

		} else if (opc == 4) {
			char nombre[32];
			int confirmacion = 0;;
			struct dogType mascota;
			int contador = 0;
			int pos;

			printf("%s", "Escriba el nombre por el cual desea buscar:");
			scanf("%s",nombre);
			

			for (int i = 0; i < 32; i++) {
				nombre[i] = toupper(nombre[i]);
			}

	     	send(clientfd, &nombre, 33, 0);
			 
			while (true) {
				int x = sizeof(struct dogType);
				int r = 0;

				//  MSG_WAITALL Solicita que se bloquee la operacion hasta que se satisfaga la solictud completa.
				recv(clientfd, &pos, sizeof(int), MSG_WAITALL);
				recv(clientfd, &mascota, sizeof(struct dogType), MSG_WAITALL);

				if (mascota.nombre[0] == '\0') {
					break;
				}
				printf("%s %i","NR: ", pos);
				printf("%s %i %s %s %s %i %s %i %0.2f %c\n", "\nID: ",
													mascota.id, " \n", mascota.nombre, mascota.tipo, 
													mascota.edad, mascota.raza, mascota.estatura,
													mascota.peso, mascota.sexo);
				contador++;
			}

			printf("\nSe encontraron %i", contador);
        	printf("%s\n\n"," registros con el mismo nombre");

			recv(clientfd, &confirmacion, sizeof(int), MSG_WAITALL);

			if (confirmacion == 1) {
				printf("%s\n", "Proceso Realizado Exitosamente");
			} else {
				printf("%s\n", "Error");
			}
			continuar2();
			
		}
	}

}