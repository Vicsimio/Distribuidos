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
                print("c> CONNECT OK")
                return client.RC.OK 
            elif resultado == b'\x01':
                print("c> CONNECT FAIL, USER DOES NOT EXIST")
                return client.RC.USER_ERROR
            elif resultado == b'\x02':
                print("c> USER  ALREADY CONNECTED")
                return client.RC.USER_ERROR
            else:
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
        #  Write your code here
        return client.RC.ERROR



    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        #  Write your code here
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
        #  Write your code here
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
    
    @staticmethod
    def listen(user):
        return None

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
