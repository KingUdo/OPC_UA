/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <pigpio.h>

#ifdef UA_NO_AMALGAMATION
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "networklayer_tcp.h"
#else
# include "open62541.h"
#endif



UA_Boolean running = true;
UA_Logger logger = Logger_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;
    gpioTerminate();

}


//################################################BUTTON##########################
static UA_StatusCode
readIntegerB(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

// Let LED BLink
    gpioPWM(17,255);
    usleep( 1000 );
    gpioPWM(17,0);

// Read Digital value of GPIO 22 and set as Node Value
    UA_Int32 G = gpioRead(22);
    handle = &G;

    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node read %.*s",
                nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "read value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}
static UA_StatusCode
writeIntegerB(void *handle, const UA_NodeId nodeid,
             const UA_Variant *data, const UA_NumericRange *range) {

    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_UInt32*)handle = *(UA_UInt32*)data->data;
    }
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32*)handle);
    return UA_STATUSCODE_GOOD;
}


//#########################################LED########################################
static UA_StatusCode
readIntegerP(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

 dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);

// Let LED BLink
    gpioPWM(17,255);
    usleep( 1000 );
    gpioPWM(17,0);

    return UA_STATUSCODE_GOOD;
}


static UA_StatusCode

writeIntegerP(void *handle, const UA_NodeId nodeid,
             const UA_Variant *data, const UA_NumericRange *range) {

    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_UInt32*)handle = *(UA_UInt32*)data->data;
    }
    // we know the nodeid is a string
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32*)handle);
// Switch LED ON

gpioPWM(24,*(UA_UInt32*)handle);

/*
UA_Int32 Val = 0;
if (Val == *(UA_UInt32*)handle ){

gpioPWM(24,0);

}else{

gpioPWM(24,255);

}
*/

 return UA_STATUSCODE_GOOD;
}
//#######################################MAIN#######################################
int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */


    if (gpioInitialise() < 0)
   {
      fprintf(stderr, "pigpio initialisation failed\n");
      return 1;

}

    gpioSetMode(17, PI_OUTPUT);
    gpioSetMode(22, PI_INPUT);
    gpioSetMode(24, PI_OUTPUT);


    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);


    /* add a variable node to the address space */
    UA_Int32 myIntegerB = 32;
    UA_NodeId myIntegerBNodeId = UA_NODEID_STRING(1, "button");
    UA_QualifiedName myIntegerBName = UA_QUALIFIEDNAME(1, "button");
    UA_DataSource dateDataSource = (UA_DataSource) {
        .handle = &myIntegerB, .read = readIntegerB, .write = writeIntegerB};
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.description = UA_LOCALIZEDTEXT("en_US","button_on_off");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","button");

    UA_Server_addDataSourceVariableNode(server, myIntegerBNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerBName, UA_NODEID_NULL, attr, dateDataSource, NULL);

 /* add a variable  node to the address space */

 /* add a variable  node to the address space */

    UA_Int32 myIntegerP = 1;
    UA_NodeId myIntegerPNodeId = UA_NODEID_STRING(1, "LED");
    UA_QualifiedName myIntegerPName = UA_QUALIFIEDNAME(1, "LED");
    UA_DataSource dateDataSourceP = (UA_DataSource) {
        .handle = &myIntegerP, .read = readIntegerP , .write = writeIntegerP};

    UA_VariableAttributes attrP;
    UA_VariableAttributes_init(&attrP);
    attrP.description = UA_LOCALIZEDTEXT("en_US","LED_ON/OFF");
    attrP.displayName = UA_LOCALIZEDTEXT("en_US","LED");

    UA_Server_addDataSourceVariableNode(server, myIntegerPNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerPName, UA_NODEID_NULL, attrP, dateDataSourceP, NULL);

    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);


    return (int)retval;
}
