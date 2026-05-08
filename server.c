#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#define MAX_USERS 100

typedef struct {
    char nombre[256];
    char ip[16];
    int puerto;
    int conectado; 
} Usuario;

Usuario lista_usuarios[MAX_USERS];
int num_usuarios = 0;
pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;

/*función auxiliar para recibir cadenas terminadas en '\0' */
int recibir_cadena(int socket_cliente, char *buffer, int max_len) {
    char caracter;
    int i = 0;
    /*leemos el socket byte a byte*/
    while (i < max_len - 1 && read(socket_cliente, &caracter, 1) > 0) {
        buffer[i++] = caracter;
        /*si el caracter es fin de cadena, acabamos*/
        if (caracter == '\0') 
            return 0;
    }
    buffer[max_len - 1] = '\0';
    return -1;
}

void *tratar_cliente(void *arg) {
    int socket_cliente = *(int *)arg;
    free(arg); 
    char operacion[256];
    char usuario[256];
    char resultado;
    char puerto[256];

    /*recibimos la operación del cliente*/
    if (recibir_cadena(socket_cliente, operacion, 256) == -1) {
        perror("Error al recibir la operación");
        close(socket_cliente);
        pthread_exit(NULL);
    }
    /*operación de registro*/
    if (strcmp(operacion, "REGISTER") == 0) {

        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&mutex_usuarios);
        int encontrado = -1;

        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0) {
                encontrado = i;
                break;
            }
        }
        /*usuario ya existe*/
        if (encontrado != -1) {
            resultado = 1; 
        /*otro error*/
        } else if (num_usuarios >= MAX_USERS) {
            resultado = 2; 
        /*todo bien*/
        } else {
            /* registramos al usuario y lo dejamos desconectado y sin puerto*/
            strcpy(lista_usuarios[num_usuarios].nombre, usuario);
            lista_usuarios[num_usuarios].conectado = 0;
            lista_usuarios[num_usuarios].ip[0] = '\0';
            lista_usuarios[num_usuarios].puerto = 0;
            num_usuarios++;
            resultado = 0; 
        }

        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);

        if(resultado == 0){
            printf("s> REGISTER %s OK\n", usuario);
        }
        else{
            printf("s> REGISTER %s FAILED\n", usuario);
        }

    /*operacion de unregister*/
    } else if (strcmp(operacion, "UNREGISTER") == 0) {
        /*operación de unregister*/
        if (recibir_cadena(socket_cliente, usuario, 256) == -1) {
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*seccion critica*/
        pthread_mutex_lock(&mutex_usuarios);
        /*vamos a buscar si el usuario existe*/
        int index_usuario = -1;

        for(int i = 0; i < num_usuarios; i++){
            if(strcmp(lista_usuarios[i].nombre, usuario) == 0){
                index_usuario = i;
                break;
            }
        } 
        /* el usuario no existe*/
        if (index_usuario == -1){
            resultado = 1;

        /* para borrar al usuario, movemos todos sus usuarios siguientes una 
        posición para atrás "pisándole" y restamos el numero total de usuarios*/
        } else if (index_usuario != -1){
            for(int i = index_usuario; i< num_usuarios - 1; i++){
                lista_usuarios[i] = lista_usuarios[i+1];
            }
            num_usuarios--;
            resultado = 0;
        /*cualquier otro caso*/
        } else{
            resultado = 2;
        }

        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);
        if(resultado == 0){
            printf("s> UNREGISTER %s TODO BIEN\n", usuario);        
        }
        else{
            printf("s> UNREGISTER %s HA FALLADO\n", usuario);
        }
    
    /*operacion de conectar a un usuario*/
    } else if(strcmp(operacion, "CONNECT") == 0){
        /*recibe el usuario*/
        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*recibimos el puerto que hemos enviado*/
        if(recibir_cadena(socket_cliente, puerto, 256) == -1){
            perror("Error al recibir el puerto");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*seccion critica*/
        pthread_mutex_lock(&mutex_usuarios);
        /*busacmos si el usuario existe o no*/
        int encontrado = -1;
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0) {
                encontrado = i;
                break;
            }
        }   
        /*usuario no encontrado */
        if (encontrado == -1) {
            resultado = 1;
        /*caso de que ya esté conectado el usuario*/
        } else if (lista_usuarios[encontrado].conectado == 1) {
            resultado = 2;
        } else if (encontrado != -1) {
            resultado = 0;
            lista_usuarios[encontrado].conectado = 1;
            /*guardamos el puerto que hemos recibido*/
            lista_usuarios[encontrado].puerto = atoi(puerto);
            /*guardamos la ip que capturamos del socket*/
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            if (getpeername(socket_cliente, (struct sockaddr *)&addr, &addr_len) == -1) {
                perror("Error al obtener la dirección del cliente");
                close(socket_cliente);
                pthread_exit(NULL); 
            } 
            strcpy(lista_usuarios[encontrado].ip, inet_ntoa(addr.sin_addr));

        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);
        if (resultado != 0){
            printf("s> CONNECT %s HA FALLADO\n", usuario);
        }
        else{
            printf("s> CONNECT %s TODO BIEN\n", usuario);
        }

    /*operacion de desconectar a un usuario*/
    } else if(strcmp(operacion, "DISCONNECT") == 0){
        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }
    /*operacion de listar usuarios*/
    } else if(strcmp(operacion, "USERS") == 0){
        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }
    /*operacion de enviar mensaje*/
    } else if(strcmp(operacion, "SEND") == 0){
        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }

    /*operacion de enviar mensaje con archivo*/
    } else if(strcmp(operacion, "SENDATTACH") == 0){

    } 
    
    close(socket_cliente);
    pthread_exit(NULL);
}