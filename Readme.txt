Para instalar las dependencias necesarias para el servicio web se usa un entorno virtual de Python.

Desde la carpeta del proyecto:

python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt   (instalará Flask)

Para compilar y ejecutar la práctica es necesario tener instalados:

- gcc
- make
- python3
- rpcbind
- libtirpc-dev


Para ejecutar todos los procesos de la práctica completa, el orden es el siguiente:

1. Arrancar rpcbind:
sudo service rpcbind start

2. Compilar los ejecutables en C:
make clean
make

3. Ejecutar el servidor RPC de logs en una terminal:
./log_server

4. Activar el entorno virtual y ejecutar el servicio web en otra terminal:
source venv/bin/activate
python3 servidor_web.py

5. Ejecutar el servidor principal en otra terminal:
./server -p 8080

6. Ejecutar uno o varios clientes en otras terminales:
python3 client.py -s 127.0.0.1 -p 8080

7. Ejecutar la batería de pruebas:
python3 app_cliente.py
8. Para comprobar el servicio web

Para comprobar que el servicio web está arrancado, se abre en el navegador:

http://127.0.0.1:8000/
