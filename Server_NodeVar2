#include <signal.h>
#include "../../open62541.h"



UA_Boolean running = true;
UA_Logger logger = UA_Log_Stdout;

UA_Int32 ButtonValue = 0;

void signalHandler(int sig) {
    running = false;
}

static UA_StatusCode
onRead(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "VALUE READ!!! %i", *(UA_UInt32*)handle);
    ButtonValue = 1;
    handle = &ButtonValue;

    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
}


int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler); /* catch ctrl-c */

    /* Create a server with one network layer listening on port 4840 */
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);

    /* Add a variable node */
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", "Test Var");
    UA_Int32 myInteger = 2572;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);

    UA_NodeId newNodeId = UA_NODEID_STRING(1, "Test.Var");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "Test Node");


    UA_DataSource dateDataSource = (UA_DataSource) {.handle = &myInteger, .read = onRead, .write = NULL};

    UA_Server_addDataSourceVariableNode(server, newNodeId,
                                        parentNodeId, parentReferenceNodeId,
                                        browseName, UA_NODEID_NULL, attr, dateDataSource, NULL);


    /* Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return status;
}
