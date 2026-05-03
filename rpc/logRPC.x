/*estructura para las operaciones normales*/
struct log_args {
    string usuario<256>;
    string operacion<256>;
};

/*estructura para SENDATTACH que incluye el fichero */
struct log_attach_args {
    string usuario<256>;
    string fichero<256>;
};

/*definición del programa RPC */
program LOG_PROG {
    version LOG_VERS {
        /* registra una operación normal*/
        int LOG_OPERACION(log_args) = 1;
        /* registra un SENDATTACH con fichero*/
        int LOG_SENDATTACH(log_attach_args) = 2;
    } = 1;
} = 0x20000002;