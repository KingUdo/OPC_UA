
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// Die Pigpio Libary ist  für das auslasen der GPIO Pins und ausgabe an diese Notwendig
#include <pigpio.h>

# include "ua_client.h"
# include "ua_config_standard.h"



static void stopHandler(int sign) {
    printf( "received ctrl-c");
    gpioTerminate();

}

int main(void) {

    if (gpioInitialise() < 0)
        {
        //printf(stderr, "pigpio initialisation failes\n");
        return 1;
        }
        gpioSetMode(17, PI_OUTPUT);



    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://192.168.2.20:16664");

    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
         }else{
        printf("Connection");
    }

    UA_Int32 value = 0;
    UA_ReadRequest rReq;
    UA_ReadRequest_init(&rReq);
    rReq.nodesToRead =  UA_Array_new(1, &UA_TYPES[UA_TYPES_READVALUEID]);
    rReq.nodesToReadSize = 1;
    rReq.nodesToRead[0].nodeId = UA_NODEID_STRING_ALLOC(1, "button"); /* assume this node exists */
    rReq.nodesToRead[0].attributeId = UA_ATTRIBUTEID_VALUE; /* return the value attribute */


while(1){

    UA_ReadResponse rResp = UA_Client_Service_read(client, rReq);

    if(rResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD &&
         rResp.resultsSize > 0 && rResp.results[0].hasValue &&
        UA_Variant_isScalar(&rResp.results[0].value) &&
        rResp.results[0].value.type == &UA_TYPES[UA_TYPES_INT32])
                {
                value = *(UA_Int32*)rResp.results[0].value.data;
                printf("the value is: %i\n", value);


                if(value == 1){
                        gpioPWM(17,255);
                        printf("ON\n");
                }else{
                        printf("OFF\n");

                gpioPWM(17,0);
                }

        usleep(100000);
        }

 UA_ReadResponse_deleteMembers(&rResp);

}




    UA_ReadRequest_deleteMembers(&rReq);
//    UA_ReadResponse_deleteMembers(&rResp);

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return (int) UA_STATUSCODE_GOOD;
    gpioTerminate();
}

