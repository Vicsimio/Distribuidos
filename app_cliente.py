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
