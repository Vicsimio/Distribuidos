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
int id_mensajes = 0;
pthread_mutex_t mutex_usuarios = PTHREAD_MUTEX_INITIALIZER;

/*función auxiliar para recibir cadenas terminadas en '\0' */
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
        write(socket_cliente, &resultado, 1);
        if(resultado == 0){
            printf("s> UNREGISTER %s OK\n", usuario);        
        }
        else{
            printf("s> UNREGISTER %s FAIL\n", usuario);
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
            lista_usuarios[encontrado].puerto = atoi(puerto);
            /*guardamos la ip que capturamos del socket*/
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            if (getpeername(socket_cliente, (struct sockaddr *)&addr, &addr_len) == -1) {
                perror("Error al obtener la dirección del cliente");
                pthread_mutex_unlock(&mutex_usuarios);
                close(socket_cliente);
                pthread_exit(NULL); 
            } 
            strcpy(lista_usuarios[encontrado].ip, inet_ntoa(addr.sin_addr));
        }
        pthread_mutex_unlock(&mutex_usuarios);
        write(socket_cliente, &resultado, 1);
        if (resultado != 0){
            printf("s> CONNECT %s FAIL\n", usuario);
        }
        else{
            printf("s> CONNECT %s OK\n", usuario);
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

        /*buscamos si el que pregunta existe, esta conectado y cuenta los usuarios conectados*/
        for (int i = 0; i < num_usuarios; i++) {
            if (strcmp(lista_usuarios[i].nombre, usuario) == 0){
                existe = 1
            } 
            if (lista_usuarios[i].conectado == 1) {
                remitente_conectado = 1;
            }
            if (lista_usuarios[i].conectado == 1) {
                num_conectados++;
            }
        }
        /*si no existe, pasamos 2 como resultado*/
        if (!existe) {
            resultado = 2;
        } 
        else if (!remitente_conectado) {
            resultado = 1;
        } 
        else {
        resultado = 0;
        }

        /*si el que pregunta no está conectad pues no puede pedir la lista*/
        if (remitente_conectado == 0) {
            resultado = 1; 
            pthread_mutex_unlock(&mutex_usuarios);
            write(socket_cliente, &resultado, 1);
            printf("s> USERS %s HA FALLADO\n", usuario);
        } 
        /*si el usuario está conectado preparamos todo para enviarle la lista*/
        else {
            resultado = 0;
            
            /*guardamos una copia de los nombres para poder enviarlos fuera del mutex*/
            char nombres_conectados[MAX_USERS][256];
            int index = 0;
            for (int i = 0; i < num_usuarios; i++) {
                if (lista_usuarios[i].conectado == 1) {
                    strcpy(nombres_conectados[index], lista_usuarios[i].nombre);
                    index++;
                }
            }
            pthread_mutex_unlock(&mutex_usuarios);
            write(socket_cliente, &resultado, 1);
            /*enviamos el número de usuarios conectados como string*/
            char users_connect[10];
            sprintf(users_connect, "%d", num_conectados);
            write(socket_cliente, users_connect, strlen(users_connect) + 1);

            /*enviamos los nombres de los usuarios uno a uno con su \0 */
            for (int i = 0; i < num_conectados; i++) {
                write(socket_cliente, nombres_conectados[i], strlen(nombres_conectados[i]) + 1);
            }
            printf("s> USERS %s OK (%d conectados)\n", usuario, num_conectados);
        }
    /*operacion de enviar mensaje*/
    } else if(strcmp(operacion, "SEND") == 0){
        int remitente_ok = 0;
        int destino_ok = 0;
        char ip_destino[16];
        int puerto_destino = 0;
        struct sockaddr_in addr_destino;
        char receptor[256];
        char mensaje[256];
        char *comando = "SEND MESSAGE\0";
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
        /*remitente no válido*/
        if (remitente_ok == 0) {
            resultado = 1; 
        /*destino no válido o desconectado*/
        } else if (destino_ok == 0) {
            resultado = 2; 
        } else {
            resultado = 0;
        }
        pthread_mutex_unlock(&mutex_usuarios); 
        if (resultado == 0) {
         /*preparamos el id como string antes de enviar nada */
         char id_str[10];
         sprintf(id_str, "%d", id_mensajes);

         sock_envio = socket(AF_INET, SOCK_STREAM, 0);
         addr_destino.sin_family = AF_INET;
         addr_destino.sin_port = htons(puerto_destino);
         addr_destino.sin_addr.s_addr = inet_addr(ip_destino);

         /*enviamos al receptor en orden */
         if (connect(sock_envio, (struct sockaddr *)&addr_destino, sizeof(addr_destino)) == 0) {
             write(sock_envio, comando, strlen(comando) + 1); 
             write(sock_envio, usuario, strlen(usuario) + 1); 
             write(sock_envio, id_str, strlen(id_str) + 1);  
             write(sock_envio, mensaje, strlen(mensaje) + 1); 
             close(sock_envio);
         } else {
             resultado = 2; 
         }
     }

     write(socket_cliente, &resultado, 1);

     /* 3. Enviamos el ID al emisor y sumamos 1 al contador */
     if (resultado == 0) {
         char id_str[10];
         sprintf(id_str, "%d", id_mensajes);
         id_mensajes++;                      
         write(socket_cliente, id_str, strlen(id_str) + 1); 

         printf("s> SEND %s %s %s OK\n", usuario, receptor, mensaje);
     } else {
         printf("s> SEND %s %s %s FAIL\n", usuario, receptor, mensaje);
     }

    /*operacion de enviar mensaje con archivo*/
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
    
    
    }
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