import subprocess
import time

SERVIDOR = "127.0.0.1"
PUERTO = "8080"


def ejecutar_prueba(nombre, comandos):
    print("\n" + nombre)

    proceso = subprocess.Popen(
        ["python3", "client.py", "-s", SERVIDOR, "-p", PUERTO],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    salida, errores = proceso.communicate(input=comandos)

    print(salida)

    if errores:
        print("Errores:")
        print(errores)


def crear_cliente():
    proceso = subprocess.Popen(
        ["python3", "client.py", "-s", SERVIDOR, "-p", PUERTO],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    return proceso


def prueba_envio_directo():
    print("\nPrueba 7: envio directo entre dos usuarios conectados")

    cliente1 = crear_cliente()
    cliente2 = crear_cliente()

    # Registramos los usuarios y conectamos al primero
    cliente1.stdin.write("REGISTER directo_a\n")
    cliente1.stdin.write("REGISTER directo_b\n")
    cliente1.stdin.write("CONNECT directo_a\n")
    cliente1.stdin.flush()

    time.sleep(1)

    # Conectamos al segundo usuario
    cliente2.stdin.write("CONNECT directo_b\n")
    cliente2.stdin.flush()

    time.sleep(1)

    # Enviamos un mensaje de directo_a a directo_b
    cliente1.stdin.write("SEND directo_b hola_directo\n")
    cliente1.stdin.flush()

    time.sleep(2)

    # Cerramos los dos clientes
    cliente2.stdin.write("DISCONNECT directo_b\n")
    cliente2.stdin.write("QUIT\n")
    cliente2.stdin.flush()

    cliente1.stdin.write("DISCONNECT directo_a\n")
    cliente1.stdin.write("UNREGISTER directo_a\n")
    cliente1.stdin.write("UNREGISTER directo_b\n")
    cliente1.stdin.write("QUIT\n")
    cliente1.stdin.flush()

    salida1, errores1 = cliente1.communicate()
    salida2, errores2 = cliente2.communicate()

    print("\nSalida cliente directo_a:")
    print(salida1)

    print("\nSalida cliente directo_b:")
    print(salida2)

    if errores1:
        print("Errores cliente directo_a:")
        print(errores1)

    if errores2:
        print("Errores cliente directo_b:")
        print(errores2)


def prueba_mensaje_pendiente():
    print("\nPrueba 8: mensaje pendiente entregado al conectar")

    emisor = crear_cliente()

    # El emisor registra a los dos usuarios y manda un mensaje
    # al receptor cuando todavía no está conectado
    emisor.stdin.write("REGISTER pend_emisor\n")
    emisor.stdin.write("REGISTER pend_receptor\n")
    emisor.stdin.write("CONNECT pend_emisor\n")
    emisor.stdin.write("SEND pend_receptor Hola pendiente\n")
    emisor.stdin.flush()

    time.sleep(2)

    receptor = crear_cliente()

    # Al conectar el receptor, debería recibir el mensaje pendiente
    receptor.stdin.write("CONNECT pend_receptor\n")
    receptor.stdin.flush()

    time.sleep(2)

    # Cerramos receptor
    receptor.stdin.write("DISCONNECT pend_receptor\n")
    receptor.stdin.write("QUIT\n")
    receptor.stdin.flush()

    # Cerramos emisor
    emisor.stdin.write("DISCONNECT pend_emisor\n")
    emisor.stdin.write("UNREGISTER pend_emisor\n")
    emisor.stdin.write("UNREGISTER pend_receptor\n")
    emisor.stdin.write("QUIT\n")
    emisor.stdin.flush()

    salida_emisor, errores_emisor = emisor.communicate()
    salida_receptor, errores_receptor = receptor.communicate()

    print("\nSalida emisor:")
    print(salida_emisor)

    print("\nSalida receptor:")
    print(salida_receptor)

    if errores_emisor:
        print("Errores emisor:")
        print(errores_emisor)

    if errores_receptor:
        print("Errores receptor:")
        print(errores_receptor)


def prueba_varios_pendientes():
    print("\nPrueba 9: varios mensajes pendientes")

    emisor = crear_cliente()

    # El receptor no está conectado, por lo que los mensajes quedan pendientes
    emisor.stdin.write("REGISTER multi_emisor\n")
    emisor.stdin.write("REGISTER multi_receptor\n")
    emisor.stdin.write("CONNECT multi_emisor\n")
    emisor.stdin.write("SEND multi_receptor mensaje1\n")
    emisor.stdin.write("SEND multi_receptor mensaje2\n")
    emisor.stdin.write("SEND multi_receptor mensaje3\n")
    emisor.stdin.flush()

    time.sleep(2)

    receptor = crear_cliente()

    # Al conectar, debería recibir los tres mensajes
    receptor.stdin.write("CONNECT multi_receptor\n")
    receptor.stdin.flush()

    time.sleep(2)

    # Lo desconectamos y conectamos otra vez para comprobar que no se repiten
    receptor.stdin.write("DISCONNECT multi_receptor\n")
    receptor.stdin.write("CONNECT multi_receptor\n")
    receptor.stdin.flush()

    time.sleep(2)

    receptor.stdin.write("DISCONNECT multi_receptor\n")
    receptor.stdin.write("QUIT\n")
    receptor.stdin.flush()

    emisor.stdin.write("DISCONNECT multi_emisor\n")
    emisor.stdin.write("UNREGISTER multi_emisor\n")
    emisor.stdin.write("UNREGISTER multi_receptor\n")
    emisor.stdin.write("QUIT\n")
    emisor.stdin.flush()

    salida_emisor, errores_emisor = emisor.communicate()
    salida_receptor, errores_receptor = receptor.communicate()

    print("\nSalida emisor:")
    print(salida_emisor)

    print("\nSalida receptor:")
    print(salida_receptor)

    if errores_emisor:
        print("Errores emisor:")
        print(errores_emisor)

    if errores_receptor:
        print("Errores receptor:")
        print(errores_receptor)


def prueba_unregister_borra_pendientes():
    print("\nPrueba 10: UNREGISTER borra mensajes pendientes")

    comandos = """REGISTER baja_emisor
REGISTER baja_receptor
CONNECT baja_emisor
SEND baja_receptor mensaje_que_debe_borrarse
UNREGISTER baja_receptor
DISCONNECT baja_emisor
UNREGISTER baja_emisor
REGISTER baja_receptor
CONNECT baja_receptor
DISCONNECT baja_receptor
UNREGISTER baja_receptor
QUIT
"""

    ejecutar_prueba("Prueba 10: UNREGISTER borra pendientes", comandos)


# Prueba 1: ciclo normal de usuario
comandos1 = """REGISTER alumno1
CONNECT alumno1
USERS
DISCONNECT alumno1
UNREGISTER alumno1
QUIT
"""
ejecutar_prueba("Prueba 1: ciclo basico", comandos1)


# Prueba 2: registrar dos veces el mismo usuario
comandos2 = """REGISTER alumno_fijo
REGISTER alumno_fijo
UNREGISTER alumno_fijo
QUIT
"""
ejecutar_prueba("Prueba 2: doble registro", comandos2)


# Prueba 3: enviar mensaje a usuario inexistente
comandos3 = """REGISTER alumno_emisor
CONNECT alumno_emisor
SEND usuario_fantasma Hola estas ahi
DISCONNECT alumno_emisor
UNREGISTER alumno_emisor
QUIT
"""
ejecutar_prueba("Prueba 3: envio a usuario inexistente", comandos3)


# Prueba 4: enviar mensaje a usuario registrado pero desconectado
comandos4 = """REGISTER receptor1
REGISTER emisor1
CONNECT emisor1
SEND receptor1 Hola receptor
DISCONNECT emisor1
UNREGISTER emisor1
UNREGISTER receptor1
QUIT
"""
ejecutar_prueba("Prueba 4: envio a usuario desconectado", comandos4)


# Prueba 5: pedir USERS sin estar conectado
comandos5 = """REGISTER alumno_users
USERS
UNREGISTER alumno_users
QUIT
"""
ejecutar_prueba("Prueba 5: USERS sin estar conectado", comandos5)


# Prueba 6: conectar usuario inexistente
comandos6 = """CONNECT no_existo
QUIT
"""
ejecutar_prueba("Prueba 6: CONNECT usuario inexistente", comandos6)


# Pruebas con dos clientes
prueba_envio_directo()
prueba_mensaje_pendiente()
prueba_varios_pendientes()
prueba_unregister_borra_pendientes()