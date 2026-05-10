CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -I/usr/include/tirpc -I./rpc
#añadimos la librería al enlazador
LBLIBS = -ltirpc
#rutas de los archivos RPC
RPC_DIR = rpc
RPC_XDR = $(RPC_DIR)/logRPC_xdr.c
RPC_SVC = $(RPC_DIR)/logRPC_svc.c
RPC_CLNT = $(RPC_DIR)/logRPC_clnt.c

all: server log_server

server: server.c $(RPC_CLNT) $(RPC_XDR)
	$(CC) $(CFLAGS) -o server server.c $(RPC_CLNT) $(RPC_XDR) $(LBLIBS)

#compilamos el servidor de log
log_server: $(RPC_DIR)/log_server.c $(RPC_SVC) $(RPC_XDR)
	$(CC) $(CFLAGS) -o log_server $(RPC_DIR)/log_server.c $(RPC_SVC) $(RPC_XDR) $(LBLIBS)
clean:
	rm -f server log_server *.o