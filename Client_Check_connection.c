
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
# include "ua_client.h"
# include "ua_config_standard.h"



static void stopHandler(int sign) {
    printf( "received ctrl-c");
}

int main(void) {

printf(" Hier wir dei Verbindung zu 192.168.2.20 aufgebaut. Falls die IP geändert werden sollte, bitte mi$

    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client,"opc.tcp://192.168.2.20:16664");

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
         }else{
        printf("Connection erfolgreich \n");
    }

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
}
