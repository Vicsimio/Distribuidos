from enum import Enum
import argparse
import socket
import threading

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = None
    _port = -1
    _listen_thread = None
    _listen_socket = None
    _user_conectado = None

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  register(user) :
        try:
            #conectamos al servidor
            sock = client.connect_server()
            if sock is None:
                print("c> REGISTER FAIL")
                return client.RC.ERROR
            sock.sendall(b"REGISTER\0")
            #enviamos el nombre del usuario
            sock.sendall((user + "\0").encode())
            #recibimos el resultado --> 1byte
            resultado = sock.recv(1)
            sock.close()
            #imprimo segun el resultado
            if resultado == b'\x00':
                print("c> REGISTER OK")
                return client.RC.OK
            elif resultado == b'\x01':
                print("c> USERNAME IN USE")
                return client.RC.USER_ERROR 
            else:
                print("c> REGISTER FAIL")
                return client.RC.ERROR

        except Exception as e:
            print("c> REGISTER FAIL")
            return client.RC.ERROR
    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
        try:
            #conectamos al servidor
            sock = client.connect_server()
            if sock is None:
                print("c> UNREGISTER FAIL")
                return client.RC.ERROR
            #enviamos la operación al servidor con el caracter de fin de cadena
            sock.sendall(b"UNREGISTER\0")
            #enviamos el nombre del usuario
            sock.sendall((user + "\0").encode())
            #recibimos el resultado --> 1byte
            resultado = sock.recv(1)
            sock.close()
            #imprimo segun el resultado
            if resultado == b'\x00':
                print("c> UNREGISTER OK")
                return client.RC.OK
            elif resultado == b'\x01':
                print("c> USER DOES NOT EXIST")
                return client.RC.USER_ERROR 
            else:
                print("c> UNREGISTER FAIL")
                return client.RC.ERROR  
        except Exception as e:
            print("c> UNREGISTER FAIL")
            return client.RC.ERROR


    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user):
        try:
            #buscamos el puerto libre
            listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_sock.bind(("", 0))
            puerto = listen_sock.getsockname()[1]
            listen_sock.listen(10)
            client._listen_socket = listen_sock
            #creamos el hilo para escuchar
            client._listen_thread = threading.Thread(target=client.listen, args=(user,), daemon=True)
            client._listen_thread.start()
            #conectamos al servidor
            sock = client.connect_server()
            if sock is None:
                print("c> CONNECT FAIL")
                return client.RC.ERROR  
            sock.sendall(b"CONNECT\0")
            sock.sendall((user + "\0").encode())
            sock.sendall((str(puerto) + "\0").encode())
            resultado = sock.recv(1)
            sock.close()
            if resultado == b'\x00':
                client._user_conectado = user
                print("c> CONNECT OK")
                return client.RC.OK 
            elif resultado == b'\x01':
                client._listen_socket.close()
                client._listen_socket = None
                print("c> CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif resultado == b'\x02':
                client._listen_socket.close()
                client._listen_socket = None
                print("c> USER ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
                client._listen_socket.close()
                client._listen_socket = None
                print("c> CONNECT FAIL")
                return client.RC.ERROR
        except Exception as e:
            print("c> CONNECT FAIL")
            return client.RC.ERROR
    # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        try:
            # Revisamos que el user esté conectado y no sea None para que no rompa
            if client._user_conectado is None:
                print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                return client.RC.USER_ERROR

            #conectamos al servidor
            sock = client.connect_server()
            if sock is None:
                print("c>CONNECTED USERS FAIL")
                return client.RC.ERROR

            #enviamos la operacion
            sock.sendall(b"USERS\0")

            #enviamos el nombre del usuario conectado
            sock.sendall((client._user_conectado + "\0").encode())
            resultado = sock.recv(1)  #recibimos en un byte el resultado de la operacion

            if resultado == b'\x00':
                num_usuarios = int(client.recv_string(sock))
                print(f"c> CONNECTED USERS ({num_usuarios} users connected) OK")

                #recibimos los nombres de los usuarios conectados
                for _ in range(num_usuarios):
                    usuario = client.recv_string(sock)
                    print(f"\t{usuario}")
                sock.close()
                return client.RC.OK

            elif resultado == b'\x01':
                #usuario no conectado
                print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
                sock.close()
                return client.RC.USER_ERROR

            else:
                print("c> CONNECTED USERS FAIL")
                sock.close()
                return client.RC.ERROR

        except Exception as e:
            print("c> CONNECTED USERS FAIL")
            return client.RC.ERROR

    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        try:
            #conectamos al servidor
            sock = client.connect_server()
            if sock is None:
                print("c> DISCONNECT FAIL")
                return client.RC.ERROR
            sock.sendall(b"DISCONNECT\0")
            sock.sendall((user + "\0").encode())
            resultado = sock.recv(1)
            sock.close()
            if resultado == b'\x00':
                print("c> DISCONNECT OK")
                #Como ha ido bien, limpiamos el usuario conectado
                client._user_conectado = None

                #cerramos el socket de escucha y esperamos a que el hilo termine
                client._listen_socket.close()
                client._listen_thread.join()
                return client.RC.OK

            elif resultado == b'\x01':
                print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR

            elif resultado == b'\x02':
                print("c> DISCONNECT FAIL, USER NOT CONNECTED")
                return client.RC.USER_ERROR

            else:
                print("c> DISCONNECT FAIL")
                return client.RC.ERROR

        except Exception as e:
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR
            
    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  send(user,  message) :               
        #Nos aseguramos de que el cliente esté conectado correctamente
        if client._user_conectado == None:
            print("c> SEND FAIL")
            return client.RC.ERROR

        #Nos aseguramos de que el mensaje tenga un máximo de 255 carácteres útiles
        if len(message.encode()) > 255:
            print("c> SEND FAIL")
            return client.RC.ERROR

        try:
            sock = client.connect_server()
            if sock == None:
                print("c> SEND FAIL")
                return client.RC.ERROR 
            #enviamos la operación
            sock.sendall(b"SEND\0")
            #enviamos el nombre del emisor
            sock.sendall((client._user_conectado + "\0").encode())
            #enviamos el nombre del receptor
            sock.sendall((user + "\0").encode())
            #enviamos mensaje
            sock.sendall((message + "\0").encode())

            #recibimos la respesta del servidor
            respuesta = sock.recv(1)

            #respuesta exitosa
            if respuesta == b'\x00':
                identificador = client.recv_string(sock)
                print("c> SEND OK - MESSAGE " + identificador)
                sock.close()
                return client.RC.OK

            #algún usuario no existe
            elif respuesta == b'\x01':
                print("c> SEND FAIL, USER DOES NOT EXIST")
                sock.close()
                return client.RC.USER_ERROR
            
            #otro error
            else:
                print("c> SEND FAIL")
                sock.close()
                return client.RC.ERROR

        except:
            print("c> SEND FAIL")
            sock.close()
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  file,  message) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False;
        
        client._server = args.s
        client._port = args.p

        return True

    #funcion axuliar para conectar al servidor
    @staticmethod
    def connect_server():
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((client._server, client._port))
            return sock
        except Exception as e:
            return None

    #función auxiliar para recibir strings y evitar repetir el bucle de leer hasta \0
    @staticmethod
    def recv_string(sock):
        #recibimos el numero de usuarios conectados
        num_usuarios = b""
        #leemos byte a byte hasta encontrar el caracter de fin de cadena
        while not num_usuarios.endswith(b"\0"):
            trozo = sock.recv(1)
            num_usuarios += trozo
        return num_usuarios[:-1].decode()
        

    @staticmethod
    def listen(user):
        try:
            while True:
                conexion, direccion = client._listen_socket.accept()
                #leemos la operacion
                operacion = client.recv_string(conexion)

                if operacion == "SEND_MESSAGE":
                    remitente = client.recv_string(conexion)
                    id_mensaje = client.recv_string(conexion)
                    mensaje = client.recv_string(conexion)

                    print(f"\ns> MESSAGE {id_mensaje} FROM {remitente}")
                    print(mensaje)
                    print("END\n")
                    print("c> ", end="", flush=True)
                elif operacion == "SEND_MESS_ACK":
                    id_mensaje = client.recv_string(conexion)
                    print(f"\nc> SEND MESSAGE {id_mensaje} OK")
                    print("c> ", end="", flush=True)
                conexion.close()

        except Exception as e:
            pass

    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    

if __name__=="__main__":
    client.main([])
