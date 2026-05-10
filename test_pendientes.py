import subprocess
import time

def crear_cliente():
    proceso = subprocess.Popen(
        ["python3", "client.py", "-s", "127.0.0.1", "-p", "8080"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    return proceso



print("Prueba: mensajes pendientes al conectar")
# Creamos el cliente emisor
emisor = crear_cliente()

#El emisor registra a los dos usuarios, se conecta y manda un mensaje al receptor cuando todavía está desconectado
emisor.stdin.write("REGISTER emisor\n")
emisor.stdin.write("REGISTER receptor\n")
emisor.stdin.write("CONNECT emisor\n")
emisor.stdin.write("SEND receptor Hola pendiente\n")
emisor.stdin.flush()
# Esperamos un poco para que el servidor guarde el mensaje pendiente
time.sleep(2)

#reamos el cliente receptor
receptor = crear_cliente()
#El receptor se conecta y debería recibir el mensaje pendiente
receptor.stdin.write("CONNECT receptor\n")
receptor.stdin.flush()
#Esperamos un poco para que llegue el mensaje 
time.sleep(2)

#Cerramos emisor y receptor
receptor.stdin.write("DISCONNECT receptor\n")
receptor.stdin.write("QUIT\n")
receptor.stdin.flush()
emisor.stdin.write("DISCONNECT emisor\n")
emisor.stdin.write("QUIT\n")
emisor.stdin.flush()

# Recogemos las salidas de los dos clientes
salida_emisor, errores_emisor = emisor.communicate()
salida_receptor, errores_receptor = receptor.communicate()

print("\n SALIDA EMISOR:")
print(salida_emisor)

if errores_emisor:
    print("ERRORES EMISOR:")
    print(errores_emisor)

print("\nSALIDA RECEPTOR:")
print(salida_receptor)

if errores_receptor:
    print("ERRORES RECEPTOR:")
    print(errores_receptor)