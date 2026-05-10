#include <stdlib.h>
#include <string.h>
#include "logRPC.h"

/*registtro una operación normal del usuario*/
bool_t
log_operacion_1_svc(log_args arg1, int *result, struct svc_req *rqstp){
    /*imprimimos el usuario y la operacion*/
    printf("%s: %s\n", arg1.usuario, arg1.operacion);
    *result = 0;
    return TRUE;
}
/*registramos una operación sendattach de un usuario*/
bool_t
log_sendattach_1_svc(log_attach_args arg1, int *result, struct svc_req *rqstp){
    /*imprimimos el usuario, la operacion y el fichero*/
    printf("%s SENDATTACH %s\n", arg1.usuario, arg1.fichero);
    *result = 0;
    return TRUE;
}
/*liberamos los resultados que necesita el stub*/
int
log_prog_1_freeresult(SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
    xdr_free(xdr_result, result);
    return 1;
}
