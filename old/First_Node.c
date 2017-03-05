
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
# include "ua_types.h"
# include "ua_server.h"
# include "ua_config_standard.h"
# include "networklayer_tcp.h"

UA_Boolean running = true;
UA_Logger logger = Logger_Stdout;

static void stopHandler(int sign) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = false;

}


//################################################Test Node B##########################
static UA_StatusCode
readIntegerB(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

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



//#######################################MAIN#######################################
int main(int argc, char** argv) {
    signal(SIGINT, stopHandler); /* catches ctrl-c */


    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16664);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);


    /* add a variable node to the address space */
    UA_Int32 myIntegerB = 32;
    UA_NodeId myIntegerBNodeId = UA_NODEID_STRING(1, "B");
    UA_QualifiedName myIntegerBName = UA_QUALIFIEDNAME(1, "B");
    UA_DataSource dateDataSource = (UA_DataSource) {
        .handle = &myIntegerB, .read = readIntegerB, .write = writeIntegerB};
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.description = UA_LOCALIZEDTEXT("en_US","Test Node B");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","B");

    UA_Server_addDataSourceVariableNode(server, myIntegerBNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerBName, UA_NODEID_NULL, attr, dateDataSource, NULL);



    UA_StatusCode retval = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);


    return (int)retval;
}
