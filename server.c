#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "rpc/logRPC.h"

#define MAX_USERS 100
#define MAX_MESSAGES 1000

/*estructura para usuarios*/
typedef struct {
    char nombre[256];
    char ip[16];
    int puerto;
    int conectado; 
    unsigned int ultimo_id;
} Usuario;

/*estructura para mensajes pendientes*/
typedef struct {
    char remitente[256];
    char receptor[256];
    char mensaje[256];
    unsigned int id;
} MensajePendiente;

Usuario lista_usuarios[MAX_USERS];
int num_usuarios = 0;
MensajePendiente mensajes_pendientes[MAX_MESSAGES];
int num_mensajes_pendientes = 0;

pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mensajes = PTHREAD_MUTEX_INITIALIZER;

/*funciones auxiliares*/
/*envia una cadena terminada en '\0' */
int enviar_cadena(int socket, char *cadena) {
    return write(socket, cadena, strlen(cadena) + 1);
}

/* recibe cadenas terminadas en '\0' */
int recibir_cadena(int socket_cliente, char *buffer, int max_len) {
    char caracter;
    int i = 0;
    /*leemos el socket byte a byte*/
    while (i < max_len && read(socket_cliente, &caracter, 1) > 0) {
        buffer[i++] = caracter;
        /*si el caracter es fin de cadena, acabamos*/
        if (caracter == '\0') 
            return 0;
    }
    buffer[max_len - 1] = '\0';
    return -1;
}

/*guarda un mensaje en la lista de pendientes */
int guardar_mensaje_pendiente(char *remitente, char *receptor, char *mensaje, unsigned int id) {
    if (num_mensajes_pendientes >= MAX_MESSAGES) {
        return -1;
    }
    strcpy(mensajes_pendientes[num_mensajes_pendientes].remitente, remitente);
    strcpy(mensajes_pendientes[num_mensajes_pendientes].receptor, receptor);
    strcpy(mensajes_pendientes[num_mensajes_pendientes].mensaje, mensaje);
    mensajes_pendientes[num_mensajes_pendientes].id = id;

    num_mensajes_pendientes++;
    return 0;
}

/*envia un mensaje al cliente receptor */
int enviar_mensaje_cliente(char *ip, int puerto, char *remitente, unsigned int id, char *mensaje) {
    int socket_envio;
    struct sockaddr_in dir_cliente;
    char id_str[20];
    sprintf(id_str, "%u", id);

    socket_envio = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_envio < 0) {
        return -1;
    }

    memset(&dir_cliente, 0, sizeof(dir_cliente));
    dir_cliente.sin_family = AF_INET;
    dir_cliente.sin_port = htons(puerto);
    dir_cliente.sin_addr.s_addr = inet_addr(ip);

    if (connect(socket_envio, (struct sockaddr *)&dir_cliente, sizeof(dir_cliente)) < 0) {
        close(socket_envio);
        return -1;
    }

    enviar_cadena(socket_envio, "SEND MESSAGE");
    enviar_cadena(socket_envio, remitente);
    enviar_cadena(socket_envio, id_str);
    enviar_cadena(socket_envio, mensaje);

    close(socket_envio);
    return 0;
}

/*envia al remitente el ackowledge indicando que su mensaje se ha entregado*/
int enviar_ack_cliente(char *ip, int puerto, unsigned int id) {
    int socket_envio;
    struct sockaddr_in dir_cliente;
    char id_str[20];

    sprintf(id_str, "%u", id);

    socket_envio = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_envio < 0) {
        return -1;
    }

    memset(&dir_cliente, 0, sizeof(dir_cliente));
    dir_cliente.sin_family = AF_INET;
    dir_cliente.sin_port = htons(puerto);
    dir_cliente.sin_addr.s_addr = inet_addr(ip);

    if (connect(socket_envio, (struct sockaddr *)&dir_cliente, sizeof(dir_cliente)) < 0) {
        close(socket_envio);
        return -1;
    }

    enviar_cadena(socket_envio, "SEND MESS ACK");
    sprintf(id_str, "%u", id);
    enviar_cadena(socket_envio, id_str);

    close(socket_envio);
    return 0;
}


void *tratar_cliente(void *arg) {
    int socket_cliente = *(int *)arg;
    free(arg); 
    char operacion[256];
    char usuario[256];
    char resultado;
    char puerto[256];
    /*guardar el resultado*/
    int res_rpc;

    /*creamos el cliente RPC para el servidor de logs*/
    CLIENT *clnt_log;
    clnt_log = clnt_create("localhost", LOG_PROG, LOG_VERS, "tcp");
    if (clnt_log == NULL) {
        printf("Aviso: No se pudo conectar al servidor de logs RPC.\n");    
    }

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
            lista_usuarios[num_usuarios].ultimo_id = 0;
            num_usuarios++;
            resultado = 0;
            if (clnt_log != NULL) {
                log_args args_log;
                enum clnt_stat status;

                /*rellenamos los datos*/
                args_log.usuario = usuario;
                args_log.operacion = "REGISTER";
                /*llamamos a la función remota*/
                status = log_operacion_1(args_log, &res_rpc, clnt_log);
                if (status != RPC_SUCCESS) printf("Error enviando log al servidor RPC.\n");
            }
        }

        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);

        if(resultado == 0){
            printf("s> REGISTER %s OK\n", usuario);
        }
        else{
            printf("s> REGISTER %s FAIL\n", usuario);
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
        resultado = 2;
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
        } 

        pthread_mutex_unlock(&mutex_usuarios);
        /* si el usuario se ha borrado correctamente, borramos sus mensajes pendientes */
        if (resultado == 0) {
            pthread_mutex_lock(&mutex_mensajes);

            for (int i = 0; i < num_mensajes_pendientes; i++) {
                if (strcmp(mensajes_pendientes[i].receptor, usuario) == 0) {
                    for (int j = i; j < num_mensajes_pendientes - 1; j++) {
                        mensajes_pendientes[j] = mensajes_pendientes[j + 1];
                    }
                num_mensajes_pendientes--;
                i--;
            }
        }

            pthread_mutex_unlock(&mutex_mensajes);
            if (clnt_log != NULL) {
                log_args args_log;
                enum clnt_stat status;
                args_log.usuario = usuario; 
                args_log.operacion = "UNREGISTER"; 

                status = log_operacion_1(args_log, &res_rpc, clnt_log);
            }
        }
        /*Imprimimos según el resultado*/
        write(socket_cliente, &resultado, 1);
        if(resultado == 0){
            printf("s> UNREGISTER %s OK\n", usuario);        
        }
        else{
            printf("s> UNREGISTER %s FAIL\n", usuario);
        }
    
    /*operacion de conectar a un usuario*/
    } else if(strcmp(operacion, "CONNECT") == 0){
        char ip_usuario[16];
        int puerto_usuario = 0;
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
        /*obtenemos la IP del cliente del socket*/
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        if (getpeername(socket_cliente, (struct sockaddr *)&addr, &addr_len) == -1) {
            perror("Error al obtener la direccion del cliente");
            resultado = 3;
            write(socket_cliente, &resultado, 1);
            printf("s> CONNECT %s FAIL\n", usuario);
            close(socket_cliente);
            pthread_exit(NULL);
        }

        strcpy(ip_usuario, inet_ntoa(addr.sin_addr));
        puerto_usuario = atoi(puerto);

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
        resultado = 3;
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
            lista_usuarios[encontrado].puerto = puerto_usuario;
            strcpy(lista_usuarios[encontrado].ip, ip_usuario);
        }
        pthread_mutex_unlock(&mutex_usuarios);

        /*respondemos al cliente*/
        write(socket_cliente, &resultado, 1);
        if (resultado != 0){
            printf("s> CONNECT %s FAIL\n", usuario);
        }
        else{
            printf("s> CONNECT %s OK\n", usuario);
            if (clnt_log != NULL) {
                log_args args_log;
                /*rellenamos los datos*/
                args_log.usuario = usuario; 
                args_log.operacion = "CONNECT"; 
                /*llamamos a la función remota*/
                log_operacion_1(args_log, &res_rpc, clnt_log);
            }
            /* Si el usuario tenia mensajes pendientes se los enviamos ahora uno a uno*/
            int seguir = 1;

            while (seguir) {
                MensajePendiente mensaje_pendiente;
                int encontrado_mensaje = -1;

                /*buscamos un mensaje pendiente para este usuario*/
                pthread_mutex_lock(&mutex_mensajes);

                for (int i = 0; i < num_mensajes_pendientes; i++) {
                    if (strcmp(mensajes_pendientes[i].receptor, usuario) == 0) {
                        mensaje_pendiente = mensajes_pendientes[i];
                        encontrado_mensaje = i;
                        break;
                    }
                }

                pthread_mutex_unlock(&mutex_mensajes);

                /*si no hay mensajes pendientes, terminamos*/
                if (encontrado_mensaje == -1) {
                    seguir = 0;
                }

                else {
                    /*intentamos enviar el mensaje pendiente al usuarioque se acaba de conectar */
                    if (enviar_mensaje_cliente(ip_usuario, puerto_usuario,mensaje_pendiente.remitente, mensaje_pendiente.id, mensaje_pendiente.mensaje) == 0) {

                        printf("s> SEND MESSAGE %u FROM %s TO %s\n", mensaje_pendiente.id, mensaje_pendiente.remitente, mensaje_pendiente.receptor);

                        /*si el mensaje se ha entregado bien lo borramos de la lista de pendientes*/
                        pthread_mutex_lock(&mutex_mensajes);

                        for (int i = 0; i < num_mensajes_pendientes; i++) {
                            if (mensajes_pendientes[i].id == mensaje_pendiente.id &&
                                strcmp(mensajes_pendientes[i].remitente, mensaje_pendiente.remitente) == 0 &&
                                strcmp(mensajes_pendientes[i].receptor, mensaje_pendiente.receptor) == 0) {

                                for (int j = i; j < num_mensajes_pendientes - 1; j++) {
                                    mensajes_pendientes[j] = mensajes_pendientes[j + 1];
                                }

                                num_mensajes_pendientes--;
                                break;
                            }
                        }

                        pthread_mutex_unlock(&mutex_mensajes);

                        /*Ahora buscamos si el remitente sigue conectado. Si lo esta, le enviamos el ACK*/
                        pthread_mutex_lock(&mutex_usuarios);

                        int remitente_conectado = 0;
                        char ip_remitente[16];
                        int puerto_remitente = 0;

                        for (int i = 0; i < num_usuarios; i++) {
                            if (strcmp(lista_usuarios[i].nombre, mensaje_pendiente.remitente) == 0 &&
                                lista_usuarios[i].conectado == 1) {

                                remitente_conectado = 1;
                                strcpy(ip_remitente, lista_usuarios[i].ip);
                                puerto_remitente = lista_usuarios[i].puerto;
                                break;
                            }
                        }

                        pthread_mutex_unlock(&mutex_usuarios);

                        if (remitente_conectado == 1) {
                            enviar_ack_cliente(ip_remitente, puerto_remitente, mensaje_pendiente.id);
                        }
                    }

                    else {
                        /*Si falla el envio, dejamos el mensaje pendiente y marcamos al usuario como desconectado*/
                        pthread_mutex_lock(&mutex_usuarios);

                        for (int i = 0; i < num_usuarios; i++) {
                            if (strcmp(lista_usuarios[i].nombre, usuario) == 0) {
                                lista_usuarios[i].conectado = 0;
                                lista_usuarios[i].ip[0] = '\0';
                                lista_usuarios[i].puerto = 0;
                                break;
                            }
                        }

                        pthread_mutex_unlock(&mutex_usuarios);

                        seguir = 0;
                    }
                }
            }
        }
    

    /*operacion de desconectar a un usuario*/
    } else if(strcmp(operacion, "DISCONNECT") == 0){
        if(recibir_cadena(socket_cliente, usuario, 256) == -1){
            perror("Error al recibir el nombre del usuario");
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
        resultado = 3;
        /*usuario no encontrado */
        if (encontrado == -1) 
            resultado = 1;
        /*usuario no conectado  por lo que no se puede desconectar*/
        else if (lista_usuarios[encontrado].conectado == 0) 
            resultado = 2;
        /*usuario encontrado y conectado*/
        else if (encontrado != -1) {
            resultado = 0;
            /*desconectamos al usuario*/
            lista_usuarios[encontrado].conectado = 0;
            /*borramos la ip y el puerto del usuario*/
            lista_usuarios[encontrado].ip[0] = '\0';
            lista_usuarios[encontrado].puerto = 0;
        }
        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);
        if (resultado != 0){
            printf("s> DISCONNECT %s FAIL\n", usuario);
        }else{
            printf("s> DISCONNECT %s OK\n", usuario);
            if (clnt_log != NULL) {
                log_args args_log;
                args_log.usuario = usuario; 
                args_log.operacion = "DISCONNECT"; 
                
                /*llamamos a la función remota*/
                log_operacion_1(args_log, &res_rpc, clnt_log);
            }
        }

    /*operacion de listar usuarios*/
    } else if(strcmp(operacion, "USERS") == 0){
        /*recibimos el nombre del usuario conectado*/
        if (recibir_cadena(socket_cliente, usuario, 256) == -1) {
            perror("Error al recibir el nombre del usuario");
            close(socket_cliente);
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&mutex_usuarios);
        
        int existe = 0;
        int remitente_conectado = 0;
        int num_conectados = 0;
        /*guardamos una copia de los nombres para poder enviarlos fuera del mutex*/
        char nombres_conectados[MAX_USERS][256];

        /*buscamos si el que pregunta existe, esta conectado y cuenta los usuarios conectados*/
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0){
                existe = 1; 
                if (lista_usuarios[i].conectado == 1) {
                    remitente_conectado = 1;
                }
            }
            if (lista_usuarios[i].conectado == 1) {
                strcpy(nombres_conectados[num_conectados], lista_usuarios[i].nombre);
                num_conectados++;
            }
        }
        /*si no existe, pasamos 2 como resultado e indicamos que ha fallado*/
        if (existe == 0) {
        resultado = 2;
        pthread_mutex_unlock(&mutex_usuarios);

        write(socket_cliente, &resultado, 1);
        printf("s> CONNECTEDUSERS FAIL\n");
        }
        /*si existe pero no está conectado pasamos 1*/
        else if (remitente_conectado == 0) {
            resultado = 1;
            pthread_mutex_unlock(&mutex_usuarios);
            write(socket_cliente, &resultado, 1);
            printf("s> CONNECTEDUSERS FAIL\n");
        }
        /*si todo ha ido bien el código de respuesta es 0*/ 
        else {
            resultado = 0;
            pthread_mutex_unlock(&mutex_usuarios);
            write(socket_cliente, &resultado, 1);
            /*creamos una cadena donde guardaremos el número de usuarios conectados*/
            char num_str[10];
            /*convertimos num_conectados a cadena y enviamos*/
            sprintf(num_str, "%d", num_conectados);
            write(socket_cliente, num_str, strlen(num_str) + 1);

        /*enviamos los nombres de los usuarios conectados*/
            for (int i = 0; i < num_conectados; i++) {
                write(socket_cliente, nombres_conectados[i], strlen(nombres_conectados[i]) + 1);
            }

            printf("s> CONNECTEDUSERS OK\n");
            if (clnt_log != NULL) {
                log_args args_log;
                args_log.usuario = usuario; 
                args_log.operacion = "USERS";
                /*llamamos a la función remota*/
                log_operacion_1(args_log, &res_rpc, clnt_log);
            }
        }
    /*operacion de enviar mensaje*/
    } else if(strcmp(operacion, "SEND") == 0){
        char receptor[256];
        char mensaje[256];
        char ip_receptor[16];
        char ip_remitente[16];
        char id_str[20];

        int indice_remitente = -1;
        int indice_receptor = -1;
        int receptor_conectado = 0;
        int puerto_receptor = 0;
        int remitente_conectado = 0;
        int puerto_remitente = 0;

        unsigned int id_mensaje = 0;
        

        /*recibimos el nombre del usuario emisor*/
        if (recibir_cadena(socket_cliente, usuario, 256) == -1) {
            perror("Error al recibir el nombre del usuario emisor");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*recibimos el nombre del usuario receptor*/
        if (recibir_cadena(socket_cliente, receptor, 256) == -1) {
            perror("Error al recibir el nombre del usuario receptor");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*recibimos el mensaje*/
        if (recibir_cadena(socket_cliente, mensaje, 256) == -1) {
            perror("Error al recibir el mensaje");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&mutex_usuarios);
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0) {
                indice_remitente = i;
                if (lista_usuarios[i].conectado == 1) {
                    remitente_conectado = 1;
                    strcpy(ip_remitente, lista_usuarios[i].ip);
                    puerto_remitente = lista_usuarios[i].puerto;
                }
            }

            if (strcmp(lista_usuarios[i].nombre, receptor) == 0) {
                indice_receptor = i;
                if (lista_usuarios[i].conectado == 1) {
                    receptor_conectado = 1;
                    strcpy(ip_receptor, lista_usuarios[i].ip);
                    puerto_receptor = lista_usuarios[i].puerto;
                }
            }
        }
        
        /*si alguno de los dos usuarios no existe, devolvemos error 1*/
        if (indice_remitente == -1 || indice_receptor == -1) {
            resultado = 1;
            pthread_mutex_unlock(&mutex_usuarios);

            write(socket_cliente, &resultado, 1);
            printf("s> SEND MESSAGE FROM %s TO %s FAIL\n", usuario, receptor);
        }

        else {
            /*calculamos el siguiente id del remitente*/
            lista_usuarios[indice_remitente].ultimo_id++;

            /*si hace overflow y vuelve a 0, el siguiente id vuelve a ser 1*/
            if (lista_usuarios[indice_remitente].ultimo_id == 0) {
                lista_usuarios[indice_remitente].ultimo_id = 1;
            }

            id_mensaje = lista_usuarios[indice_remitente].ultimo_id;
            pthread_mutex_unlock(&mutex_usuarios);

            resultado = 0;
            sprintf(id_str, "%u", id_mensaje);

            /*respondemos al cliente que envio el mensaje*/
            write(socket_cliente, &resultado, 1);
            enviar_cadena(socket_cliente, id_str);
            if (clnt_log != NULL) {
                log_args args_log;
                args_log.usuario = usuario; 
                args_log.operacion = "SEND";
                log_operacion_1(args_log, &res_rpc, clnt_log);
            }

            /*si el receptor esta conectado intentamos enviarle el mensaje*/
            if (receptor_conectado == 1) {

                if (enviar_mensaje_cliente(ip_receptor, puerto_receptor, usuario, id_mensaje, mensaje) == 0) {

                    printf("s> SEND MESSAGE %u FROM %s TO %s\n", id_mensaje, usuario, receptor);

                    /*si el remitente esta conectado le enviamos el ACK*/
                    if (remitente_conectado == 1) {
                        enviar_ack_cliente(ip_remitente, puerto_remitente, id_mensaje);
                    }
                }
            
                else {
                    /*si falla el envio, guardamos el mensaje como pendiente*/
                    pthread_mutex_lock(&mutex_mensajes);
                    guardar_mensaje_pendiente(usuario, receptor, mensaje, id_mensaje);
                    pthread_mutex_unlock(&mutex_mensajes);

                    /*marcamos al receptor como desconectado*/
                    pthread_mutex_lock(&mutex_usuarios);

                    for (int i = 0; i < num_usuarios; i++) {
                        if (strcmp(lista_usuarios[i].nombre, receptor) == 0) {
                            lista_usuarios[i].conectado = 0;
                            lista_usuarios[i].ip[0] = '\0';
                            lista_usuarios[i].puerto = 0;
                            break;
                        }
                    }

                pthread_mutex_unlock(&mutex_usuarios);

                printf("s> MESSAGE %u FROM %s TO %s STORED\n", id_mensaje, usuario, receptor);
                }
            }

            else {
                /*si el receptor no esta conectado, guardamos el mensaje*/
                pthread_mutex_lock(&mutex_mensajes);
                guardar_mensaje_pendiente(usuario, receptor, mensaje, id_mensaje);
                pthread_mutex_unlock(&mutex_mensajes);

                printf("s> MESSAGE %u FROM %s TO %s STORED\n", id_mensaje, usuario, receptor);
            }
        }
    
    /*operacion de enviar mensaje con archivo*/
    #if 0                                           
    } else if(strcmp(operacion, "SENDATTACH") == 0){
        char receptor[256];
        char mensaje[256];
        char archivo[256];
        int remitente_ok = 0;
        int destino_ok = 0;
        char ip_destino[16];
        int puerto_destino = 0;
        struct sockaddr_in addr_destino;
        char *comando = "ATTACH\0";
        int sock_envio = 0;
        /*recibimos el nombre del usuario emisor*/
         if (recibir_cadena(socket_cliente, usuario, 256) == -1) {
            perror("Error al recibir el nombre del usuario emisor");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        
        /*recibimos el nombre del usuario receptor*/
        if (recibir_cadena(socket_cliente, receptor, 256) == -1) {
            perror("Error al recibir el nombre del usuario receptor");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*recibimos el archivo*/
        if (recibir_cadena(socket_cliente, archivo, 256) == -1) {
            perror("Error al recibir el nombre del archivo");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        /*recibimos el mensaje*/
        if (recibir_cadena(socket_cliente, mensaje, 256) == -1) {
            perror("Error al recibir el mensaje");
            close(socket_cliente);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&mutex_usuarios);
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0 && lista_usuarios[i].conectado == 1) {
                remitente_ok = 1;
            }
            if (strcmp(lista_usuarios[i].nombre, receptor) == 0 && lista_usuarios[i].conectado == 1) {
                destino_ok = 1;
                strcpy(ip_destino, lista_usuarios[i].ip);
                puerto_destino = lista_usuarios[i].puerto;
            }
        }
        if (remitente_ok == 0) {
            resultado = 1;
        } else if (destino_ok == 0) {
            resultado = 2;
        } else {
            resultado = 0;
        }
        pthread_mutex_unlock(&mutex_usuarios); 

        /* Conexión al receptor */
        if (resultado == 0) {
            sock_envio = socket(AF_INET, SOCK_STREAM, 0);
            addr_destino.sin_family = AF_INET;
            addr_destino.sin_port = htons(puerto_destino);
            addr_destino.sin_addr.s_addr = inet_addr(ip_destino);

            if (connect(sock_envio, (struct sockaddr *)&addr_destino, sizeof(addr_destino)) == 0) {
                write(sock_envio, comando, strlen(comando) + 1); // Avisamos que es un ATTACH
                write(sock_envio, usuario, strlen(usuario) + 1); // Quién envía
                write(sock_envio, archivo, strlen(archivo) + 1); // El nombre del archivo
                write(sock_envio, mensaje, strlen(mensaje) + 1); // El mensaje
                close(sock_envio);
            } else {
                resultado = 2; // Falló la conexión con el receptor
            }
        }

        /* Respuesta al emisor */
        write(socket_cliente, &resultado, 1);
        if (resultado == 0) {
            char id_str[10];
            sprintf(id_str, "%d", id_mensajes);
            id_mensajes++;                      
            write(socket_cliente, id_str, strlen(id_str) + 1); 
            
            printf("s> SENDATTACH %s %s %s %s OK\n", usuario, receptor, archivo, mensaje);
        } else {
            printf("s> SENDATTACH %s %s %s %s FAIL\n", usuario, receptor, archivo, mensaje);
        }
        
    #endif
    }
    if (clnt_log != NULL) clnt_destroy(clnt_log);   
    close(socket_cliente);
    pthread_exit(NULL);
}
int main(int argc, char *argv[]) {
    int puerto = 0;
    /*creamos el socket principal del servidor */
    int server_socket;
    struct sockaddr_in server_addr;
    
    /*comprobamos los argumentos*/
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Uso: %s -p <puerto>\n", argv[0]);
        exit(1);
    }
    puerto = atoi(argv[2]);
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creando el socket");
        exit(1);
    }

    /* Opciones para poder reutilizar el puerto rápido si cerramos el servidor */
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; /* Escucha en todas las interfaces (0.0.0.0) */
    server_addr.sin_port = htons(puerto);

    /*bindeamos el socket al puerto */
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_socket);
        exit(1);
    }

    /*escuchamos a maximo 10 clientes en cola*/
    if (listen(server_socket, 10) < 0) {
        perror("Error en listen");
        close(server_socket);
        exit(1);
    }

    printf("s> init server 127.0.0.1:%d\n", puerto);

    /*atendemos las conexiones */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        /*reservamos memoria para el socket del cliente para pasarlo al hilo */
        int *client_socket = malloc(sizeof(int));
        
        *client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (*client_socket < 0) {
            perror("Error en accept");
            free(client_socket);
            continue;
        }

        /*creamos un hilo para que trate a este cliente mientras el main vuelve a hacer accept */
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, tratar_cliente, client_socket) != 0) {
            perror("Error creando hilo");
            free(client_socket);
        } else {
            /* Detach para que libere recursos automáticamente al terminar*/
            pthread_detach(thread_id); 
        }
    }

    close(server_socket);
    return 0;
}