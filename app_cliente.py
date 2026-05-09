import subprocess

def ejecutar_prueba(nombre_prueba, comandos):
    print(f"\n{'='*50}")
    print(f"iniciando {nombre_prueba}")
    print(f"{'='*50}")
    
    # Lanza el cliente conectándose al servidor local en el puerto 8080
    proceso = subprocess.Popen(
        ["python3", "client.py", "-s", "127.0.0.1", "-p", "8080"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    # Inyecta los comandos como si los escribieras en la terminal
    salida, errores = proceso.communicate(input=comandos)
    
    print(salida)
    if errores:
        print(f"ERRORES REPORTADOS:\n{errores}")


#prueba 1: resgistro, conexión, consulta de usuarios, desconexión y baja
comandos_basicos = """REGISTER alumno1
CONNECT alumno1
USERS
DISCONNECT alumno1
UNREGISTER alumno1
QUIT
"""
ejecutar_prueba("Prueba 1: Ciclo de vida básico (Registro, Conexión, Usuarios, Desconexión, Baja)", comandos_basicos)

#prueba 2: registrar un usuario ya existente 
comandos_error_registro = """REGISTER alumno_fijo
REGISTER alumno_fijo
QUIT
"""
ejecutar_prueba("Prueba 2: Control de errores (Doble registro)", comandos_error_registro)

#prueba 3: enviar un mensaje a alguien que no existe
comandos_error_envio = """REGISTER alumno_emisor
CONNECT alumno_emisor
SEND usuario_fantasma Hola, estas ahi?
DISCONNECT alumno_emisor
QUIT
"""
ejecutar_prueba("Prueba 3: Control de errores (Envío a usuario inexistente)", comandos_error_envio)

# prueba 4: enviar mensaje a usuario registrado pero desconectado
comandos_send_pendiente = """REGISTER receptor1
REGISTER emisor1
CONNECT emisor1
SEND receptor1 Hola receptor, estas desconectado
DISCONNECT emisor1
QUIT
"""
ejecutar_prueba("Prueba 4: Envío a usuario registrado pero desconectado", comandos_send_pendiente)

# prueba 5: pedir USERS sin estar conectado
comandos_users_sin_conectar = """REGISTER alumno_users
USERS
QUIT
"""
ejecutar_prueba("Prueba 5: USERS sin estar conectado", comandos_users_sin_conectar)

# prueba 6: conectar usuario inexistente
comandos_connect_inexistente = """CONNECT no_existo
QUIT
"""
ejecutar_prueba("Prueba 6: CONNECT de usuario inexistente", comandos_connect_inexistente)