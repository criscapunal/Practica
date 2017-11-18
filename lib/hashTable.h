#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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


#endif