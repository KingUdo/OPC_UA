###Setup
####Vorraussetzungen
Vorrausgesetzt wird eine Jessie Lite Version auf einem Raspbery Pi, welcher an ein Netzwerk angeschlossen, und über ssh erreichbar ist. Hierfür siehe https://www.raspberrypi.org/downloads/raspbian/ für Tutorials über den Setupprozess, und https://www.raspberrypi.org/documentation/remote-access/ssh/ für die Aktivierung des SSH. Dabei ist es auch noch wichtig, eine statische IP der Pi zu geben. Dies kann am einfachsten durch die Konfiguration des Routers geschehen.

####open62541
Zuerst wird ein Verzeichniss angelegt und dort die neuste Version von open62541 herruntergeladen und extrahiert.
```
ssh pi@192.168.178.62
mkdir open62541
cd open62541
wget https://github.com/open62541/open62541/releases/download/0.2-rc2/open62541-raspberrypi.tar.gz
tar -xvzf open62541-raspberrypi.tar.gz 
rm open62541-raspberrypi.tar.gz
ls
```
Jetzt sollte man die Dateien sehen: 
```
AUTHORS  README.md  examples        open62541.c  server_cert.der
LICENSE  doc        libopen62541.a  open62541.h
```

Um zu testen, ob der simple Setup funktioniert hat, würde ich empfehlen die in `examples` gegebenen Beispiele auszuführen. Dafür muss noch eine zweite SSH Session geöffnet werden.
**erste SSH Session**
```
cd examples
./server
```
**zweite SSH Session**
```
cd ~/open62541/examples
./client
```

Wenn sich nun in beiden Sessions etwas verändert hat, und die Scripte keine Fehler geworfen haben, sollte alles korrekt erstellt sein.
Um die Funktion der OPC-UA Server zu testen, nutzen wir UA-Expert (https://www.unified-automation.com/downloads/opc-ua-clients.html). Hierbei bitte die richtige Version für das Betriebssystem herrunterladen und instalieren.

###Aufbau eines open62541 Servers
Um das ganze so verständlich wie möglich zu machen, sind alle hier beschriebenen Scripte hinterlegt. Dennoch wird nicht bei allen Skripten der kompletten Code beschrieben, da die Skripe aufeinander aufbauen.
Es wird vorgeschlagen, sich alle Skripte via `git clone` herrunterzuladen. Dabei ist dies am besten im Ordner open63542 machen.
```
cd ../
mkdir projekts
cd projekts
git clone https://github.com/KingUdo/OPC_UA.git
```
Als erstes Script wird das Skript Server_Min.c genauer beschrieben, welches den Aufbau eines OPC-UA Servers beschreibt.
```
cd server
cat Server_Min.c
```
Hier sieht man das Grundgerüst eines Servers. Zuerst wir die signal.h importiert, welche einige Standarts beinhaltet. Dannach wird die open62541.h importiert. Dieser Pfad muss gegebenenfalls angepasst werden.
```
#include <signal.h>
#include "../../open62541.h"
```
Danach definieren wir eine Funktion, die sich um die Variable running kümmert. Diese muss auf true gesetzt werden, während der Server läuft. Die Funktion `signalHandler` wird später bei einem Strg+C angesprochen und beendet den Server sachgemäß, um den tcp Port wieder freizugeben.
```
UA_Boolean running = true;
void signalHandler(int sig) {
    running = false;
}
```
Dahinter befindet sich die `main` Funktion. Diese startet zuerst die `signal()` Funktion aus signal.h,  welche Eingaben überwacht. Dabei wird nur bei 'Interrupt Signalen' wie Strg + c `SIGINT` ausgelöst und die Funktion `signalHandler` aktiviert.
Nach dem Aktiviern der `signal` Funktion wird ein Server erstellt. Dabei wird zuert eine Server Config Datei `config` defeniert und mit dem Standartwerten ausgestattet. Danach wird ein ServerNetworkLayer `nl` erstellt, welcher ein TCP Layer ist. Dieser ist wiederrum mit dem `UA_ConnectionConfig_standard` ausgestattet, der auf Port 4840 läuft.
Diese ServerNetworkLayer wird dann der `config` Datei hinzugefügt und zuletzt wird noch ein neuer `UA_Server` pointer mit dem Namen `server` und der `config` Datei erstellt.   
```
int main(int argc, char** argv)
{
    signal(SIGINT, signalHandler); /* catch ctrl-c */

    /* Create a server with one network layer listening on port 4840 */
    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 4840);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    UA_Server *server = UA_Server_new(config);
```
Als zweiten Block sieht man hier das Script, welches den gerade erstellten Server ausführt. 
Hierbei wird die Variable `status` erstellt, welche alle Statusmeldungen vom Server kommend aufnimmt. Der Server wird mit `UA_Server_run(server, &running)` gestartet. In dieser Schleife bleibt der Server bis die variable `running` auf false gesetzt wird. Wenn dies der Fall ist, wird der Server und die NetworkLayer gelöscht und der Wert vom Status zurückgegeben.
```
/* Run the server loop */
    UA_StatusCode status = UA_Server_run(server, &running);
    UA_Server_delete(server);
    nl.deleteMembers(&nl);
    return status;
```

Um nun den programierten Server zum laufen zu bringen, muss dieser noch kompiliert werden. Dies ist mit der Befehlszeile: `gcc -std=c99 Server_Min.c ../../open62541.c -o Server_Min` machbar, wobei hier der Pfad zur `open62541.c` Datei gegebennenfals angepasst werden muss.
Nach erfoglreichen kompilieren kann der Server mit `./Server_min` gestartet werden. Ist dies erfogreich, sollte der Server einen Timestamp und die Adresse, auf welcher er erreichbar ist, angeben.
```
[03/03/2017 13:12:16.286] info/network	TCP network layer listening on opc.tcp://OPC-UA-PI:4840
```
Um jetzt zu überprüfen, ob der Server auch sachgemäß arbeitet, benutzen wir das vorher installierte Programm UaExpert. Nach erstem Öffnen muss ein Zertifikat angelegt werden, was aber keine Probleme darstellen sollte. Danach kann über das Plus Zeichen ein neuer Server hinzugefügt werden. Unter dem Feld 'Add Server' muss dann zuerst ein Name unter Configuration Name vergeben werden. Anschließend kann unter dem Reiter "Advanced" noch die IP Adresse, bzw. der HostName in das Feld 'Endpoint Url' eingetragen werden. Dabei ist zu beachten, dass auch der verwendete Port mit angegeben werden muss.
Nach dem Hinzufügen des Servers erscheint ein Link in dem Feld Project, der Server ist aber noch nicht verbunden. Zum Verbinden muss dieser ausgewählt, und das Steckersymbol in der obern Leiste geklickt werden. Nach einer erfolgreichen Verbindung wird dieses im Log mit einem "Browse succeeded", sowie mit einem eingesteckten Symbol neben dem Servernamen bestätigt. Um nun zu überprüfen, ob auch Nodes (Servervariablen) ausgelesen werden können, öffent man auf der linken Seite in der Mitte den Server und danach den Nodes Server Status. Hierbei ist die Variable 'Current Time' sichtbar, welche angeklickt und in die Mitte gezogen werden kann. Das in die Mitte Ziehen bewirkt die Erstellung einer 'subscription'. Man sollte nun eine sich updatende Zeit sehen können. Um die subscription wieder zu entfernen, muss unter Rechtsklick 'Remove selected Nodes' gewählt werden. 
Jetzt wissen wir, dass unser UPC-UA Server funktioniert und wir die schon vorhandenen Nodes auslesen können. Um eigene Nodes zu erstellen, muss der Servercode etwas angepasst werden.

###Hinzufügen von einfachen Nodes
Eine Node ist eine Variable, welche auf dem Server gespeichert ist und extern von einem Client aufgerufen werden kann. Wie man schon sehen konnte, gibt es im OPC-UA Server schon standartmäßig Nodes, wie 'CurrenTime', 'StartTime' oder 'State'. Um selber Nodes zu setzen, müssen diese im Servercode hinzugefügt werden. Als Beispiel dient heirbei die Datei Server_Node.c, an welcher das Hinzufügen einer Node gezeigt wird.
Der erste Teil der Datei ist äquivalent zu der Vorherigen und erstellt den Server. Die Erstellung der Node findet im Teil nach `/* Add a variable node */` statt. Im ersten Block werden die Parameter der Node definiert. Zuerst wird eine UA_VariableAttributes Variable namens attr erstellt. Danach wird in dieser Variable das Attribut `displayName` auf "Test Var" gesetzt. Im Anschluss wird ein UA_Int32 namens myInteger erstellt, welcher den Wert "2572" beinhaltet. Dieser Wert wird dann noch mit attr.value verlinkt. Die verschiednenen Variablentypen können unter "http://open62541.org/doc/current/types.html" nachgelesen werden. 
```
    UA_VariableAttributes attr;
    UA_VariableAttributes_init(&attr);
    attr.displayName = UA_LOCALIZEDTEXT("en_US", "Test Var");
    UA_Int32 myInteger = 2572;
    UA_Variant_setScalar(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
```

Im zweiten Block wird die NodeID auf "Test.Node" gesetzt, sowie die Position, unter welcher die Node sich befinden soll. Alle hierbei definierten Variablen werden später bei der Erstellung der Node benötigt. Zuletzt wird noch der angezeigte Name definierte. Diesen Wert kann man später beim Durchsuchen der Node sehen.

```
    UA_NodeId newNodeId = UA_NODEID_STRING(1, "Test.Var");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_NodeId variableType = UA_NODEID_NULL; /* take the default variable type */
    UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "Test Node");
```

Im letzte Block wird die definierte Node noch dem Server hinzugefügt. Dabei ist der Syntax für UA_Server_addVariableNode `UA_Server_addVariableNode	(UA_Server *server, UA_Variant *value, const UA_QualifiedName browseName, UA_NodeId nodeId, const UA_NodeId	parentNodeId, const UA_NodeId referenceTypeId)`, dieser kann in größerem Detail unter 'http://open62541.org/doc/0.1/group__server.html#ga9d0151ddb2f02dc46506aac5d10f2524' nachgelesen werden.
```
   UA_Server_addVariableNode(server, newNodeId, parentNodeId, parentReferenceNodeId,browseName, variableType, attr, NULL, NULL);
```

Auch diesen Server kann man mit dem vorher verwendeten Kommando `gcc -std=c99 Server_Node.c ../../open62541.c -o Server_Node` kompelieren. Dieser wird auch mit dem Komando `./Server_Node.c` ausgeführt. 
Man sollte jetzt mit UaExpert auf den Server zugreifen, und dort die Node "Test Value" mit dem Wert 2575 auslesen können.

###Hinzufügen von veränderbaren Nodes
Es ist möglich Nodes zu erzeugen und diese auszulesen. Aber der Wert der Node ist immer der Wert, der am Anfang gesetzt wurde. Wie können jetzt variable Werte, wie zum Beispiel der Wert eines Kopfes (gedrückt, nicht gedrückt), oder eines Thermomethers ausgelesen werden? 
Dafür gibt es zwei Varianten, zwischen welchen, basierend auf den zeitlichen Anvorderungen, gewählt wird.
Einerseits gibt es die Variante, bei der, der Server von selbst in einem bestimmten zeitlichen Abstand den Sensorwert abfragt und diesen in der Node speichert. Wenn nun eine Anfrage auf den Nodewert gestellt wird, wird der gespeicherte Wert zurückgegeben. Diese Methode ist sehr schnell, gibt aber nicht den aktuellen Wert zurück, sonder immer nur einen etwas veralteten. Diese Methode wird bei zeitkrischen Anwendungen verwendet, bei denen der Wert nicht absolut aktuell sein muss.  
Im Gegensatz dazu gibt es die zweite Methode, welche den aktuellesten Wert liefert, aber nicht so schnell ist wie die Erste. Bei dieser Methode wird der Sensorwert abgefragt nachdem einen NodeAnfrage gestellt wurde. So kann der NodeAnfrage der top aktuelle Wert geliefert werden. 

Da die Umsetzung der zweiten Methode einfacher ist, wird diese zuerst erklärt.
Um das Prinzip auch ohne Knöpfe und Thermomether zu verstehen, wird es mit einer Funktion erläutert, die bei jeder Anfrage den zurückgelieferten Wert verändert.  Als Beispielscript dient hierbei Server_NodeVar.c.

Abermals ist dieses Script, mit einigen Veränderungen, auf dem vorherigen Script basierend.
Als erste Veränderung wurde ein logging Element hinzugefügt. Dieses erlaubt es logging Ausgaben an das Terminal zu senden.
```
UA_Logger logger = UA_Log_Stdout
```
Als weitere Veränderung wurde die Funktion `onREad()` hinzugefügt. Diese Funktion wird im späteren Verlauf aufgerufen und gibt im Server Terminal zurück, dass die Node aufgerufen wurde.
```
static void onRead() {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "VALUE READ!!!");
}
```
Als dritte Veränderung wurde die Funktion `onRead()` noch in den `UA_ValueCallback` der vorher erzeugten Node mit eingebunden.
```
    UA_ValueCallback callback = {(void*)7, onRead, NULL};
    UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
```
Wenn nun auch dieser Server mit `gcc -std=c99 Server_NodeVar.c ../../open62541.c -o Server_NodeVar` kompeliert und danach ausgeführt wird, kann man in der Konsole des Serveres sehen, wenn mit UaExpert auf die Node zugegriffen wird. Dafür soll am besten eine Subscription für die Node erzeugt werden (durch Ziehen der Node in die Mitte). Dabei wird alle 200ms der Wert abgefragt, was in der Konsole zu beobachten ist.

Jetzt ist eine Funktion vorhanden, welche bei jedem Lesezugriff ausgeführt wird. Um den Wert, der von der Node zurückgegeben wird, auch noch bei jedem Zugriff zu verändern, sind noch ein paar weitere Veränderungen nötig. Um vorallem Daten dynamisch in die Node einzufügen, ist aber ein anderes Konstrukt notwendig. Hierbei wird ein `UA_DataSource` benötigt. Dieser wird, wie in Server_NodeVar2.c beschrieben, in den Server eingebaut. 
In dieser Version muss die read Funktion angepasst werden. Ihr werden nun der handler, die node ID und ein paar weitere Variablen übergeben. In der Funktion wird zuerst wieder der logger angesprochen, welcher ausgibt, dass ein Lesezugriff stattfindet. Danach wird der Variable ButtonValue ein Wert zugewiesen. Hierbei können jegliche Funktionen mit eingebunden werden, welche physische Messwerte extrahieren. Dem handle wird daraufhin der Wert der Variablen übergeben. Um nun auch die Node zu updaten, wird ein `UA_Variant_setScalarCopy` ausgeführt, welche den Wert, den handler und den Datentyp übermittelt bekommt.
```
static UA_StatusCode
onRead(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "VALUE READ!!!");
    ButtonValue = 1;
    handle = &ButtonValue;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
}
```

Auf diese Art und Weise ist es möglich OPC-UA Anfragen, mit nach der Anfrage gewonnenen Werten, zu beantworten. 
Um das Beispiel zu vollenden, muss noch eine Funktion eingebaut werden, welche der Variable ButtonValue einen Wert gibt und beim Aufruf der `onRead` Funktion, ausgeführt wird. 

Wie bereits oben erwähnt, gibt es noch eine weitere Methode Daten von externen Quellen zu bekommen. Bei dieser, wird in einem bestimmten Abstand der Datenwert abgefragt und in einer Variable gespeichert. Wenn nun eine Nodeanfrage stattfindet, wird diese Variabel der Node übergeben, und der Wert zurückgegeben. Als Beispiel für diesen Aufbau dient Server_NodeVar3.c.

Die Funktion, die immer wieder ausgeführt wird ist in diesem Fall `testCallback`. Die Funktion selber schreibt zuerst "tesstcallback" in den Log. und addiert dann auf den Wert von ButtonValue eine Einhet hinzu. 
```
static void testCallback(UA_Server *server, void *data) {
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "testcallback %i", ButtonValue);
    ButtonValue = ButtonValue + 1;
}
```
Natürlich muss dem Server noch gesagt werden, dass diese immer wieder ausführbare Funktion existiert. Dies wird im unteren Teil des Scriptes gemacht. Dort wird zuert einem `UA_Job` `job` als `UA_JOBTYPE_METHODCALL` definert, wobei als Methode die Funktion `testcallback` angegeben wurde. Hirbei wird als Datenwert ein `NULL` weitergeben. Dieser Wert kann angepast werden.  
Danach wird der UA_Job noch dem Server mit `UA_Server_addRepeatedJob` hinzugefügt. Dabei wird dem Server der Name des Servers, der Name des Jobs und das Zeitintervall übergeben. Das Zeitintervall wird in Millisekunden angegegben.
```
UA_Job job = {.type = UA_JOBTYPE_METHODCALL,
                  .job.methodCall = {.method = testCallback, .data = NULL} };
    UA_Server_addRepeatedJob(server, job, 2000, NULL); // call every 2 sec
```
Auch dieser Server kann mit `gcc -std=c99 Server_NodeVar3.c ../../open62541.c -o Server_NodeVar3` kompiliert werden. Beim Ausführen des Servers sollte man alle zwei Sekunden eine Loginfo mit `testcallback` bekommen. Hierbei kann man beobachten, wie sich der Wert von ButtonValue jede zweite Sekunden erhöht. Dies ist auch mit UaExpert möglich. Nach dem Erstellen einer Subscription kann man dort sehen, wie sich der Wert immer um 1 erhöht. Dabei darf man sich nicht von der Log Info 'VALUE READ!! 2572' irritieren lassen. Hierbei wird der, dem Handeler zugewiesenen Wert ausgelesen. Dieser wurde am Anfang als 2572 definiert. Die `onRead` Funktion verändert in der jetzigen Konfiguration nicht nur den Orginalwert der Node, sonder gibt auch einen anderen Wert bei einer Abfrage zurück. 


###Node Beschreiben
So ergibt sich die Frage, wie man den Orginalwert einer Node verändern kann. 
Das ist einerseits von einem externen Programm, andererseits innerhalb der Funktion möglich.
Auch hierfür ist ein Beispiel vorhanden: im Script Server_NodeVar4.c, welches auf Server_NodeVar2.c basiert, wurde eine Schreibfunktion eingebaut. Um die korrekte Funktion der Schribfunktion zu bestätigen, wurde zuerst die Lesefunktion bearbeitet. In dieser wurden die Definition der Variable `ButtonValue` herausgenommen, genauso wie dem `*handel` nicht mehr der Wert vom `ButtonValue` zugewiesen wird. Jetzt wird automatisch bei einer Anfrage der Wert der Node zurückgegeben.
```
static UA_StatusCode
onRead(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {

    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "VALUE READ!!! %i", *(UA_UInt32*)handle);
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_INT32]);
}
```
Darüber hinaus wurde noch eine Funktion `onWrite` hinzugefügt. Diese ist sehr ähnlich zu der `onRead` Funktion. Die Funktion selber prüft erst, ob der Eingabedatentyp dem vorgegebenen Datentyp von `UA_TYPES_INT32` entspricht und übergibt dann dem handel den Wert von data. Zuletzt wird noch ein InfoLog Event mit "written value" und dem aktuellen Wert des handel ausgegeben. 
```
onWrite(void *handle, const UA_NodeId nodeid,
             const UA_Variant *data, const UA_NumericRange *range) {

    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_UInt32*)handle = *(UA_UInt32*)data->data;
    }
    UA_LOG_INFO(logger, UA_LOGCATEGORY_USERLAND, "written value %i", *(UA_UInt32*)handle);
}
```
Die onWrite Funktion muss noch dem Server übergeben werden. Dies geschieht im `UA_DataSource`. Hierbei wird die Funktion `onWrite` unter`.write` verlinkt.
```
UA_DataSource dateDataSource = (UA_DataSource) {.handle = &myInteger, .read = onRead, .write = onWrite};
```
Getestet werden kann diese Script mit dem UaExpert. Nach dem erfogreichen kompilieren mit `gcc -std=c99 Server_NodeVar4.c ../../open62541.c -o Server_NodeVar4` kann der Server abermals mit `./Server_NodeVar4` gestartet werden. Im UeExpert, sollte nun der Pi Server sichtbar sein. Nach dem Erzeugen einer Subscription der Node ist der aktuelle Wert sichtbar. Dieser Wert kann im UeExpert auch durch Klicken verändert werden. Es sollte nun möglich sein, diesen Wert zu verändern, sodass er auch konstant auf dem veränderten Wert bleibt. Es ist bei einer Veränderung der Node möglich im Terminalfenster die Infonachricht 'written value 123' mit dem korrekten Integer zu beobachten.

##Client
Der zweite große Teil des Open62541 Paketes ist das Readingmodul. Mit dem Paket ist es auch möglich, einen Client aufzubauen, welcher den Wert einen Node ausliest. Um die Funktionen eins Clients zu erklären, ist das Script Client_Min.c im Ordner Clients zu verwenden. Im ersten Teil wird ,äquivalent zum Server, `studio.h` und ` open62541.h`importiert. Danach wird in dem Hauptteil des Scriptes ein `UA_Client` namens `*client` als neuer Client definiert, welcher die Standartwerte besitzt. Um sich mit einem Server verbinden zu können, wird auch noch ein `UA_Client_connect` benötigt. Dieser heißt in dem Fall `retval`. Um zu Überprüfen ob eine verbindung mit dem Server aufgebaut werden kann wird daraufhin noch der Statuscode von `retval` ausgelesen, und nur wenn dieser `UA_STATUSCODE_GOOD` ist, also eine Verbindung aufgebaut werden konnte wird die Funktion weiter ausgeführt.
```
#include <stdio.h>
#include "../../open62541.h"

int main(void) {
    UA_Client *client = UA_Client_new(UA_ClientConfig_standard);
    UA_StatusCode retval = UA_Client_connect(client, "opc.tcp://OPC-UA-PI:4840");
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return (int)retval;
    }
```

Nachdem überprüft wurde, dass eine Verbindung aufgebaut werden konnte, wird ein `UA_Variant` namens `value` definiert und dieses mit ` UA_Variant_init(&value);` initialisiert. Um nun auch eine bestimmte Node auslesen zu könenn, muss noch eine `UA_NodeId` definierte werden. Diese ist in diesem Beispiel die lokale Zeit des Servers. Normalerweise wird eine Node mit der vorher definierten numerischen NodeId angesprochen, aber spezielle Nodes, wie die locale Zeit, der Status, oder der Startzeitpunkt, können mit spezielen Shortcodes ausgelesen werden. (`UA_NS0ID_SERVER_SERVERSTATUS_STATE, UA_NS0ID_SERVER_SERVERSTATUS_STARTTIME `) 
```
    UA_Variant value; 
    UA_Variant_init(&value);

    const UA_NodeId nodeId =
        UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER_SERVERSTATUS_CURRENTTIME);
```
Jetzt ist es möglich die vorher definierten Variablen zu nutzen, um den eigentlichen Wert der Node auszulesen. Dies wird im folgenden Abschnitt getan. Zuerst wird versucht mit einem `UA_Client_readValueAttribute`, welchem alle vorherigen Variablen übergeben wurden, die Node auszulesen. Nach dem Ausleseveruch wird zuerst getestet, ob der Statuscode `UA_STATUSCODE_GOOD` und der ausgelesene Wert dem Datentypus `UA_TYPES_DATETIME` entspricht. Falls dies der Fall ist, wird aus `value` das Datum extrahiert, in einen String verwandelt und in der Konsole ausgegeben. Zuletzt wird noch der String `string_date`, sowie `value` und der client gelöscht. 
```
    retval = UA_Client_readValueAttribute(client, nodeId, &value);
    if(retval == UA_STATUSCODE_GOOD &&
       UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DATETIME])) {
        UA_DateTime raw_date = *(UA_DateTime*)value.data;
        UA_String string_date = UA_DateTime_toString(raw_date);
        printf("string date is: %.*s\n", (int)string_date.length, string_date.data);
        UA_String_deleteMembers(&string_date);
    }

    /* Clean up */
    UA_Variant_deleteMembers(&value);
    UA_Client_delete(client); /* Disconnects the client internally */
    return UA_STATUSCODE_GOOD;
}
```
