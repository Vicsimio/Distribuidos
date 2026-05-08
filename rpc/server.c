#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "logRPC.h"
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
int recibir_cadena(int socket_cliente, char *buffer) {
    char caracter;
    int i = 0;
    /*leemos el socket byte a byte*/
    while (read(socket_cliente, &caracter, 1) > 0) {
        buffer[i++] = caracter;
        /*si el caracter es fin de cadena, acabamos*/
        if (caracter == '\0') 
            return 0;
    }
    return -1;
}

void *tratar_cliente(void *arg) {
    int socket_cliente = *(int *)arg;
    free(arg); 
    char operacion[256];
    char usuario[256];
    char resultado;

    /*recibimos la operación del cliente*/
    if (recibir_cadena(socket_cliente, operacion)) {
        perror("Error al recibir la operación");
        pthread_exit(NULL);
    }
    /*operación de registro*/
    if (strcmp(operacion, "REGISTER") == 0) {

        recibir_cadena(socket_cliente, usuario);
        pthread_mutex_lock(&mutex_lista);
        int encontrado = -1;
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(usuarios[i].nombre, usuario) == 0) {
                encontrado = i;
                break;
            }
        }
        if (encontrado != -1) {
            resultado = 2; 
        } else if (num_usuarios >= MAX_USERS) {
            resultado = 1; 
        } else {
            strcpy(usuarios[num_usuarios].nombre, usuario);
            usuarios[num_usuarios].conectado = 0;
            num_usuarios++;
            resultado = 0; 
        }

        pthread_mutex_unlock(&mutex_lista);
        write(socket_cliente, &resultado, 1);

    } else if (strcmp(operacion, "UNREGISTER") == 0) {
        /*operación de unregister*/
        if (recibir_cadena(socket_cliente, usuario)) {
            perror("Error al recibir el nombre del usuario");
            pthread_exit(NULL);
        }
        /*seccion critica*/
        pthread_mutex_lock(&mutex_usuarios);
        /*vamos a buscar si el usuario existe*/
        int index_usuario = -1;
    } else if 

    close(socket_cliente);
    pthread_exit(NULL);
}

/*registtro una operación normal del usuario*/
bool_t
log_operacion1_svc(log_args arg1, int *result, struct svc_req *rqstp){
    /*imprimimos el usuario y la operacion*/
    printf("%s: %s\n", arg1.usuario, arg1.operacion);
    *result = 0;
    return TRUE;
}
/*registramos una operación sendattach de un usuario*/
bool_t
log_sendattach1_svc(log_attach_args arg1, int *result, struct svc_req *rqstp){
    /*imprimimos el usuario, la operacion y el fichero*/
    printf("%s SENDATTACH %s\n", arg1.usuario, arg1.fichero);
    *result = 0;
    return TRUE;
}
/*liberamos los resultados que necesita el stub*/
int
log_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    xdr_free(xdr_result, result);
    return 1;
}
