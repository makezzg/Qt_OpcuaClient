#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "open62541.h"
#include <QDebug>


char display_str[2000];

#define print(format, ...)                                      \
    do                                                          \
    {                                                           \
        printf(format, ##__VA_ARGS__);                          \
    } while (0)

//memset(display_str, 0, sizeof(display_str));            \
//sprintf(display_str, format, ##__VA_ARGS__);            \
//ui->textEdit->append(display_str);                      \

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->radioButton_string->setChecked(true);
    ui->radioButton_byte->setChecked(true);
    ui->lineEdit_namespace->setText(QString::number(4));
    ui->lineEdit_id_string->setText(QString("|var|PAC120-PXX01-3X-XX-XX.Application.GVL.mi_testINT"));
    ui->lineEdit_URL->setText(QString("opc.tcp://169.254.116.116:4840"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chgEnable(boolean enable)
{
    ui->lineEdit_namespace->setDisabled(enable);
    ui->lineEdit_id_string->setDisabled(enable);
    ui->lineEdit_value->setDisabled(enable);
    ui->BtnReadInt->setDisabled(enable);
    ui->BtnWriteInt->setDisabled(enable);
    ui->nsiBool->setDisabled(enable);
    ui->nsiReal->setDisabled(enable);
    ui->nsiString->setDisabled(enable);
    ui->IdStringBool->setDisabled(enable);
    ui->IdStringReal->setDisabled(enable);
    ui->IdStringString->setDisabled(enable);
    ui->ValueBool->setDisabled(enable);
    ui->ValueReal->setDisabled(enable);
    ui->ValueString->setDisabled(enable);
    ui->BtnReadBool->setDisabled(enable);
    ui->BtnReadReal->setDisabled(enable);
    ui->BtnReadString->setDisabled(enable);
    ui->BtnWriteBool->setDisabled(enable);
    ui->BtnWriteReal->setDisabled(enable);
    ui->BtnWriteString->setDisabled(enable);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS
static void
handler_TheAnswerChanged(UA_Client *client, UA_UInt32 subId, void *subContext,
                         UA_UInt32 monId, void *monContext, UA_DataValue *value) {
    printf("The Answer has changed!\n");
}
#endif

static UA_StatusCode
nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) {
    if(isInverse)
        return UA_STATUSCODE_GOOD;
    UA_NodeId *parent = (UA_NodeId *)handle;
    printf("%d, %d --- %d ---> NodeId %d, %d\n",
           parent->namespaceIndex, parent->identifier.numeric,
           referenceTypeId.identifier.numeric, childId.namespaceIndex,
           childId.identifier.numeric);
    return UA_STATUSCODE_GOOD;
}



void MainWindow::on_pushButton_connect_clicked()
{
    /* 获取URL */
    /* QString -> char * */
    QString str = ui->lineEdit_URL->text();
    //QString str = ui->comboBox->currentText();
    //define a fixed url -> to opcua server on the PAC120
    //str = "opc.tcp://169.254.116.116:4840";
    qDebug() << "get the url:" << str ;     //debug info to get the input url
    char *url;
    QByteArray ba = str.toLatin1();
    url = ba.data();

    UA_Client *client = UA_Client_new(UA_ClientConfig_default);

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, url,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return;
    }
    print("%i endpoints found\n", (int)endpointArraySize); // show how many endpoints found


    for(size_t i=0;i<endpointArraySize;i++) {
        print("URL of endpoint %i is %.*s\n", (int)i,  //print each endpoint's url
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    /* Connect to a server */
    /* anonymous connect would be: retval = UA_Client_connect(client, "opc.tcp://localhost:4840"); */
    //retval = UA_Client_connect_username(client, url, "user1", "password");
    retval = UA_Client_connect(client, url);    //connect to server without authority verify
    if(retval != UA_STATUSCODE_GOOD) {
        qDebug()<<"connection failed, connection will be closed!";
        UA_Client_delete(client);
        return;
    }else{
        qDebug()<<"connection success";     //debug to show the status of ua connection, 0->good
    }

    /* Browse some objects */
    print("Browsing nodes in objects folder:\n");  //3 Browsing nodes in objects folder:
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    print("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");   //4 NAMESPACE NODEID           BROWSE NAME      DISPLAY NAME
    for(size_t i = 0; i < bResp.resultsSize; ++i) {
        for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
                print("%-9d %-16d %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                       ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
                       ref->browseName.name.data, (int)ref->displayName.text.length,
                       ref->displayName.text.data);
            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                print("%-9d %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
                       (int)ref->nodeId.nodeId.identifier.string.length,
                       ref->nodeId.nodeId.identifier.string.data,
                       (int)ref->browseName.name.length, ref->browseName.name.data,
                       (int)ref->displayName.text.length, ref->displayName.text.data);
            }
            /* TODO: distinguish further types */
        }
    }
    UA_BrowseRequest_deleteMembers(&bReq);
    UA_BrowseResponse_deleteMembers(&bResp);

    /* Same thing, this time using the node iterator... */
    UA_NodeId *parent = UA_NodeId_new();
    *parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_Client_forEachChildNodeCall(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   nodeIter, (void *) parent);
    UA_NodeId_delete(parent);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Create a subscription */    
    qDebug()<<"doing subscription";     //logic into subscription
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(client, request,
                                                                            NULL, NULL, NULL);

    UA_UInt32 subId = response.subscriptionId;
    if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        print("Create subscription succeeded, id %u\n", subId);

    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(UA_NODEID_STRING(4, "|var|PAC120-PXX01-3X-XX-XX.Application.GVL.mi_testINT"));

    UA_MonitoredItemCreateResult monResponse =
    UA_Client_MonitoredItems_createDataChange(client, response.subscriptionId,
                                              UA_TIMESTAMPSTORETURN_BOTH,
                                              monRequest, NULL, handler_TheAnswerChanged, NULL);
    if(monResponse.statusCode == UA_STATUSCODE_GOOD)
        print("Monitoring 'abcde', id %u\n", monResponse.monitoredItemId);


    /* The first publish request should return the initial value of the variable */
    UA_Client_runAsync(client, 1000);
#endif

    /* Read attribute */
    UA_Int32 value = 0;
    print("\nReading the value of node (1, \"the.answer\"):\n");
    UA_Variant *val = UA_Variant_new();
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(4, "|var|PAC120-PXX01-3X-XX-XX.Application.GVL.mi_testINT"), val);
    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) &&
       val->type == &UA_TYPES[UA_TYPES_INT32]) {
            value = *(UA_Int32*)val->data;
            print("the value is: %i\n", value);
    }
    UA_Variant_delete(val);

    /* Write node attribute */
    value++;
    print("\nWriting a value of node (1, \"the.answer\"):\n");
    UA_WriteRequest wReq;
    UA_WriteRequest_init(&wReq);
    wReq.nodesToWrite = UA_WriteValue_new();
    wReq.nodesToWriteSize = 1;
    wReq.nodesToWrite[0].nodeId = UA_NODEID_STRING_ALLOC(1, "abced");
    wReq.nodesToWrite[0].attributeId = UA_ATTRIBUTEID_VALUE;
    wReq.nodesToWrite[0].value.hasValue = true;
    wReq.nodesToWrite[0].value.value.type = &UA_TYPES[UA_TYPES_INT32];
    wReq.nodesToWrite[0].value.value.storageType = UA_VARIANT_DATA_NODELETE; /* do not free the integer on deletion */
    wReq.nodesToWrite[0].value.value.data = &value;
    UA_WriteResponse wResp = UA_Client_Service_write(client, wReq);
    if(wResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
            print("the new value is: %i\n", value);
    UA_WriteRequest_deleteMembers(&wReq);
    UA_WriteResponse_deleteMembers(&wResp);

    /* Write node attribute (using the highlevel API) */
    value++;
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
    UA_Client_writeValueAttribute(client, UA_NODEID_STRING(1, "the.answer"), myVariant);
    UA_Variant_delete(myVariant);

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Take another look at the.answer */
    UA_Client_runAsync(client, 100);
    /* Delete the subscription */
    if(UA_Client_Subscriptions_deleteSingle(client, subId) == UA_STATUSCODE_GOOD)
        print("Subscription removed\n");
#endif

#ifdef UA_ENABLE_METHODCALLS
    /* Call a remote method */
    UA_Variant input;
    UA_String argString = UA_STRING("Hello Server");
    UA_Variant_init(&input);
    UA_Variant_setScalarCopy(&input, &argString, &UA_TYPES[UA_TYPES_STRING]);
    size_t outputSize;
    UA_Variant *output;
    retval = UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(1, 62541), 1, &input, &outputSize, &output);
    if(retval == UA_STATUSCODE_GOOD) {
        print("Method call was successful, and %lu returned values available.\n",
               (unsigned long)outputSize);
        UA_Array_delete(output, outputSize, &UA_TYPES[UA_TYPES_VARIANT]);
    } else {
        print("Method call was unsuccessful, and %x returned values available.\n", retval);
    }
    UA_Variant_deleteMembers(&input);
#endif

#ifdef UA_ENABLE_NODEMANAGEMENT
    /* Add new nodes*/
    /* New ReferenceType */
//    UA_NodeId ref_id;
//    UA_ReferenceTypeAttributes ref_attr = UA_ReferenceTypeAttributes_default;
//    ref_attr.displayName = UA_LOCALIZEDTEXT("en-US", "NewReference");
//    ref_attr.description = UA_LOCALIZEDTEXT("en-US", "References something that might or might not exist");
//    ref_attr.inverseName = UA_LOCALIZEDTEXT("en-US", "IsNewlyReferencedBy");
//    retval = UA_Client_addReferenceTypeNode(client,
//                                            UA_NODEID_NUMERIC(1, 12133),
//                                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
//                                            UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
//                                            UA_QUALIFIEDNAME(1, "NewReference"),
//                                            ref_attr, &ref_id);
//    if(retval == UA_STATUSCODE_GOOD )
//        print("Created 'NewReference' with numeric NodeID %u\n", ref_id.identifier.numeric);

//    /* New ObjectType */
//    UA_NodeId objt_id;
//    UA_ObjectTypeAttributes objt_attr = UA_ObjectTypeAttributes_default;
//    objt_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewObjectType");
//    objt_attr.description = UA_LOCALIZEDTEXT("en-US", "Put innovative description here");
//    retval = UA_Client_addObjectTypeNode(client,
//                                         UA_NODEID_NUMERIC(1, 12134),
//                                         UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
//                                         UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
//                                         UA_QUALIFIEDNAME(1, "NewObjectType"),
//                                         objt_attr, &objt_id);
//    if(retval == UA_STATUSCODE_GOOD)
//        print("Created 'NewObjectType' with numeric NodeID %u\n", objt_id.identifier.numeric);

    /* New Object */
    UA_NodeId obj_id;
    UA_ObjectAttributes obj_attr = UA_ObjectAttributes_default;
    obj_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewGreatNode");
    obj_attr.description = UA_LOCALIZEDTEXT("de-DE", "Hier koennte Ihre Webung stehen!");
    retval = UA_Client_addObjectNode(client,
                                     UA_NODEID_NUMERIC(1, 0),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                     UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                     UA_QUALIFIEDNAME(1, "TheGreatNode"),
                                     UA_NODEID_NUMERIC(1, 12134),
                                     obj_attr, &obj_id);
    if(retval == UA_STATUSCODE_GOOD )
        print("Created 'NewObject' with numeric NodeID %u\n", obj_id.identifier.numeric);

    /* New Integer Variable */
    UA_NodeId var_id;
    UA_VariableAttributes var_attr = UA_VariableAttributes_default;
    var_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewVariableNode");
    var_attr.description =
        UA_LOCALIZEDTEXT("en-US", "This integer is just amazing - it has digits and everything.");
    UA_Int32 int_value = 1234;
    /* This does not copy the value */
    UA_Variant_setScalar(&var_attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
    var_attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    retval = UA_Client_addVariableNode(client,
                                       UA_NODEID_NUMERIC(1, 0), // Assign new/random NodeID
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(0, "VariableNode"),
                                       UA_NODEID_NULL, // no variable type
                                       var_attr, &var_id);
    if(retval == UA_STATUSCODE_GOOD )
        print("Created 'NewVariable' with numeric NodeID %u\n", var_id.identifier.numeric);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);

    fflush(stdout);

    return;
}

void MainWindow::on_pushButton_connect_2_clicked()
{
    /* 获取URL */
    /* QString -> char * */
    QString str = ui->lineEdit_URL->text();
    //QString str = ui->comboBox->currentText();
    //str = "opc.tcp://169.254.116.116:4840";
    qDebug()<<"connecting to server:"<<str;
    char *url;
    QByteArray ba = str.toLatin1();
    url = ba.data();    //url converted from var 'str' defined above

    client = UA_Client_new(UA_ClientConfig_default);    //create a new ua client

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, url,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return;
    }
    print("%i endpoints found\n", (int)endpointArraySize); //1 1 endpoints found


    for(size_t i=0;i<endpointArraySize;i++) {
        print("URL of endpoint %i is %.*s\n", (int)i,  //2 URL of endpoint 0 is opc.tcp://localhost:4840
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);




    /* Connect to a server */
    /* anonymous connect would be: retval = UA_Client_connect(client, "opc.tcp://localhost:4840"); */
    //retval = UA_Client_connect_username(client, url, "user1", "password");
    retval = UA_Client_connect(client, url);    //connect to server without authority verify
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Client_delete(client);
        return;
    }else{
        print("Server connected!\n");
    }

//    /* Browse some objects */
//    print("Browsing nodes in objects folder:\n");  //3 Browsing nodes in objects folder:
//    UA_BrowseRequest bReq;
//    UA_BrowseRequest_init(&bReq);
//    bReq.requestedMaxReferencesPerNode = 0;
//    bReq.nodesToBrowse = UA_BrowseDescription_new();
//    bReq.nodesToBrowseSize = 1;
//    bReq.nodesToBrowse[0].nodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER); /* browse objects folder */
//    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL; /* return everything */
//    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);//单个节点的研究下

//    printf("bResp.resultsSize = %d\n", bResp.resultsSize);//--
//    fflush(stdout);
//    print("%-9s %-16s %-16s %-16s\n", "NAMESPACE", "NODEID", "BROWSE NAME", "DISPLAY NAME");   //4 NAMESPACE NODEID           BROWSE NAME      DISPLAY NAME
//    for(size_t i = 0; i < bResp.resultsSize; ++i) {
////        printf("bResp.results[i].referencesSize = %d\n", bResp.results[i].referencesSize);//--
//        for(size_t j = 0; j < bResp.results[i].referencesSize; ++j) {
//            UA_ReferenceDescription *ref = &(bResp.results[i].references[j]);
//            if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_NUMERIC) {
//                print("%-9d n%-15d %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
//                       ref->nodeId.nodeId.identifier.numeric, (int)ref->browseName.name.length,
//                       ref->browseName.name.data, (int)ref->displayName.text.length,
//                       ref->displayName.text.data);
////                if(999 == ref->nodeId.nodeId.identifier.numeric)
////                    printf("%d\n", *bResp.results[i].continuationPoint.data[j]);
//            } else if(ref->nodeId.nodeId.identifierType == UA_NODEIDTYPE_STRING) {
//                print("%-9d %-16.*s %-16.*s %-16.*s\n", ref->nodeId.nodeId.namespaceIndex,
//                       (int)ref->nodeId.nodeId.identifier.string.length,
//                       ref->nodeId.nodeId.identifier.string.data,
//                       (int)ref->browseName.name.length, ref->browseName.name.data,
//                       (int)ref->displayName.text.length, ref->displayName.text.data);
//            }
//            /* TODO: distinguish further types */
//        }
//    }
//    UA_BrowseRequest_deleteMembers(&bReq);
//    UA_BrowseResponse_deleteMembers(&bResp);



    fflush(stdout);
}

void MainWindow::on_pushButton_string_read_clicked()        //INT16 value <- codesys INT variable
{
    UA_Int16 value = 0;

    /* Read attribute */
    int nsi = ui->lineEdit_namespace->text().toInt();   //namespaceIndex -> nsi
    UA_Variant *val = UA_Variant_new();

    QString str = ui->lineEdit_id_string->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
    if(retval == 0 && UA_Variant_isScalar(val) &&
       val->type == &UA_TYPES[UA_TYPES_INT16]) {    //UA_TYPES_INT32 -> INT16, codesys use int16 for INT variable
            value = *(UA_Int16*)val->data;          //use int16 to instead of int32
            ui->lineEdit_value->setText(QString::number(value));
            print("the value is: %i\n", value);
    }else{
        printf("read error\n");ui->lineEdit_value->setText("");
    }

//    if(ui->radioButton_string->isChecked())
//    {

//    }else if(ui->radioButton_int->isChecked())
//    {
//        int inum = ui->lineEdit_id_string->text().toInt();
//        print("\nReading the value of node (%d, %d):\n",nsi, inum);

//        retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(nsi, inum), val);
//        if(retval == 0 && UA_Variant_isScalar(val) &&
//           val->type == &UA_TYPES[UA_TYPES_INT16]) {    //UA_TYPES_INT32 -> INT16, codesys use int16 for INT variable
//                value = *(UA_Int16*)val->data;          //use int16 to instead of int32
//                ui->lineEdit_value->setText(QString::number(value));
//                print("the value is: %i\n", value);
//        }else{printf("read error\n");ui->lineEdit_value->setText("");}
//    }



    fflush(stdout);
    UA_Variant_delete(val);
}

void MainWindow::on_pushButton_string_write_clicked()       //INT16 value -> codesys INT variable
{
    int nsi = ui->lineEdit_namespace->text().toInt();       //NameSpaceIndex


    /* Write node attribute (using the highlevel API) */
    int value = ui->lineEdit_value->text().toInt();         //Value

    UA_Variant *myVariant = UA_Variant_new();
    if(ui->radioButton_int32->isChecked()) {
        UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT32]);
    }else if(ui->radioButton_byte->isChecked()) {
        UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_BYTE]);
    }
    if(ui->radioButton_string->isChecked())
    {
        QString str = ui->lineEdit_id_string->text();
        char *isting;
        QByteArray ba = str.toLatin1();
        isting = ba.data();

        retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsi, isting), myVariant);
        if(retval == UA_STATUSCODE_GOOD){printf("1write success\n");}else{printf("1write failed\n");}
    }
    else
    {
        int inum = ui->lineEdit_id_string->text().toInt();

        retval = UA_Client_writeValueAttribute(client, UA_NODEID_NUMERIC(nsi, inum), myVariant);
        if(retval == UA_STATUSCODE_GOOD){printf("write success\n");}else{printf("write failed\n");}

    }
    UA_Variant_delete(myVariant);

    fflush(stdout);
}

void MainWindow::on_pushButton_string_read_2_clicked()
{
    /* Same thing, this time using the node iterator... */
    UA_NodeId *parent = UA_NodeId_new();
    *parent = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_Client_forEachChildNodeCall(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                   nodeIter, (void *) parent);
    UA_NodeId_delete(parent);

    fflush(stdout);
}

void MainWindow::on_pushButton_add_value_clicked()
{
    /* New Integer Variable */
    UA_NodeId var_id;
    UA_VariableAttributes var_attr = UA_VariableAttributes_default;
    var_attr.displayName = UA_LOCALIZEDTEXT("en-US", "TheNewVariableNode");
    var_attr.description =
        UA_LOCALIZEDTEXT("en-US", "This integer is just amazing - it has digits and everything.");
    UA_Int32 int_value = 1234;
    /* This does not copy the value */
    UA_Variant_setScalar(&var_attr.value, &int_value, &UA_TYPES[UA_TYPES_INT32]);
    var_attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    retval = UA_Client_addVariableNode(client,
                                       UA_NODEID_NUMERIC(1, 0), // Assign new/random NodeID
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                       UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                       UA_QUALIFIEDNAME(0, "VariableNode"),
                                       UA_NODEID_NULL, // no variable type
                                       var_attr, &var_id);
    if(retval == UA_STATUSCODE_GOOD )
        print("Created 'NewVariable' with numeric NodeID %u\n", var_id.identifier.numeric);

    fflush(stdout);
}

void MainWindow::on_pushButton_string_read_4_clicked()
{
    int nsi = ui->lineEdit_namespace->text().toInt();

    /* Read attribute */
    UA_Int32 value = 0;

    UA_Variant *val = UA_Variant_new();

    if(ui->radioButton_string->isChecked())
    {
        QString str = ui->lineEdit_id_string->text();
        char *isting;
        QByteArray ba = str.toLatin1();
        isting = ba.data();
        print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

        retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
        if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val))
        {
            if(ui->radioButton_int32->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_INT32]) {
                    value = *(UA_Int32*)val->data;
                }
            }
            else if(ui->radioButton_byte->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_BYTE]) {
                    value = *(UA_Byte*)val->data;
                }
            }
            else if(ui->radioButton_bool->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
                    value = *(UA_Boolean*)val->data;
                }
            }
                ui->lineEdit_value->setText(QString::number(value));
                print("the value is: %i\n", value);

        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }else if(ui->radioButton_int->isChecked())
    {
        int inum = ui->lineEdit_id_string->text().toInt();
        print("\nReading the value of node (%d, %d):\n",nsi, inum);

        retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(nsi, inum), val);
        if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val))
        {
            if(ui->radioButton_int32->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_INT32]) {
                    value = *(UA_Int32*)val->data;
                }
            }
            else if(ui->radioButton_byte->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_BYTE]) {
                    value = *(UA_Byte*)val->data;
                }
            }
            else if(ui->radioButton_bool->isChecked()) {
                if(val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
                    value = *(UA_Boolean*)val->data;
                }
            }
                ui->lineEdit_value->setText(QString::number(value));
                print("the value is: %i\n", value);

        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }



    fflush(stdout);
    UA_Variant_delete(val);
}

void MainWindow::on_pushButton_string_read_5_clicked()
{
    int nsi = ui->lineEdit_namespace->text().toInt();

    /* Read attribute */
    UA_Int32 value = 0;

    UA_Variant *val = UA_Variant_new();

    if(ui->radioButton_string->isChecked())
    {
        QString str = ui->lineEdit_id_string->text();
        char *isting;
        QByteArray ba = str.toLatin1();
        isting = ba.data();
        print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

        retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
        if(retval == UA_STATUSCODE_GOOD)
        {
//            if(ui->radioButton_int32->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_INT32]) {
//                    value = *(UA_Int32*)val->data;
//                }
//            }
//            else if(ui->radioButton_byte->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_BYTE]) {
//                    value = *(UA_Byte*)val->data;
//                }
//            }
//            else if(ui->radioButton_bool->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
//                    value = *(UA_Boolean*)val->data;
//                }
//            }
                ui->lineEdit_value->setText(QString::number(value));
                print("the value is: %i\n", value);

                printf("val->arrayLength = %d\n", val->arrayLength);    //数组的大小

                UA_UInt16 *datas = (UA_UInt16 *)val->data;
                for(int i = 0; i < 100; i++)
                {
                    printf("%d\t%#x\n", i, *(datas+i));
                }
//                UA_Double *datas = (UA_Double *)val->data;
//                for(int i = 0; i < 100; i++)
//                {
//                    printf("%d\t%f\n", i, *(datas+i));
//                }
//                        UA_Double *datas = (UA_Double *)val->data;
//                        qDebug()<<"datas"<<*(datas+2)<<endl;               //数组的内容，这个是读的第3个数据


        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }else if(ui->radioButton_int->isChecked())
    {
        int inum = ui->lineEdit_id_string->text().toInt();
        print("\nReading the value of node (%d, %d):\n",nsi, inum);

        retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(nsi, inum), val);
        if(retval == UA_STATUSCODE_GOOD)
        {
//            if(ui->radioButton_int32->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_INT32]) {
//                    value = *(UA_Int32*)val->data;
//                }
//            }
//            else if(ui->radioButton_byte->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_BYTE]) {
//                    value = *(UA_Byte*)val->data;
//                }
//            }
//            else if(ui->radioButton_bool->isChecked()) {
//                if(val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
//                    value = *(UA_Boolean*)val->data;
//                }
//            }
//                ui->lineEdit_value->setText(QString::number(value));
//                print("the value is: %i\n", value);

            printf("val->arrayLength = %d\n", val->arrayLength);    //数组的大小
//            qDebug()<<"arrayLength"<<val->arrayLength<<endl;    //数组的大小
//                    UA_Double *datas = (UA_Double *)val->data;
////                    qDebug()<<"datas"<<*(datas+2)<<endl;               //数组的内容，这个是读的第3个数据
//                    for(int i = 0; i < 65536; i++)
//                    {
//                        printf("%d\t%f\n", i, *(datas+i));
//                    }
            UA_UInt16 *datas = (UA_UInt16 *)val->data;
            for(int i = 0; i < 100; i++)
            {
                printf("%d\t%#x\n", i, *(datas+i));
            }


        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }



    fflush(stdout);
    UA_Variant_delete(val);
}

void MainWindow::on_pushButton_string_read_6_clicked()
{
    int nsi = ui->lineEdit_namespace->text().toInt();

    /* Read attribute */
    UA_Int32 value = 0;

    UA_Variant *val = UA_Variant_new();

    if(ui->radioButton_string->isChecked())
    {
        QString str = ui->lineEdit_id_string->text();
        char *isting;
        QByteArray ba = str.toLatin1();
        isting = ba.data();
        print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

//        retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
        UA_UInt32 *arrayDimsRead;
        size_t len;
        retval = UA_Client_readArrayDimensionsAttribute(client, UA_NODEID_STRING(nsi, isting), &len, &arrayDimsRead);
        if(retval == UA_STATUSCODE_GOOD)
        {

            printf("len = %d\n", len);
            printf("arrayDimsRead = %f\n", arrayDimsRead[1]);

//                ui->lineEdit_value->setText(QString::number(value));
//                print("the value is: %i\n", value);

//                printf("val->arrayLength = %d\n", val->arrayLength);    //数组的大小

//                UA_Double *datas = (UA_Double *)val->data;
//                for(int i = 0; i < 100; i++)
//                {
//                    printf("%d\t%f\n", i, *(datas+i));
//                }
////                        UA_Double *datas = (UA_Double *)val->data;
////                        qDebug()<<"datas"<<*(datas+2)<<endl;               //数组的内容，这个是读的第3个数据


        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }else if(ui->radioButton_int->isChecked())
    {
        int inum = ui->lineEdit_id_string->text().toInt();
        print("\nReading the value of node (%d, %d):\n",nsi, inum);

        retval = UA_Client_readValueAttribute(client, UA_NODEID_NUMERIC(nsi, inum), val);
        if(retval == UA_STATUSCODE_GOOD)
        {


            printf("val->arrayLength = %d\n", val->arrayLength);    //数组的大小
//            qDebug()<<"arrayLength"<<val->arrayLength<<endl;    //数组的大小
                    UA_Double *datas = (UA_Double *)val->data;
//                    qDebug()<<"datas"<<*(datas+2)<<endl;               //数组的内容，这个是读的第3个数据
                    for(int i = 0; i < 65536; i++)
                    {
                        printf("%d\t%f\n", i, *(datas+i));
                    }


        }else{printf("read error\n");ui->lineEdit_value->setText("");}
    }



    fflush(stdout);
    UA_Variant_delete(val);
}


//new implementation by zhzg===============@June. 2022


void MainWindow::on_btnConnect_clicked()                //create the connection to defined server
{
    //create a new client
    client = UA_Client_new(UA_ClientConfig_default);    //create a new ua client
    /* get url */
    /* QString -> char * */
    QString str = ui->lineEdit_URL->text();
    //QString str = ui->comboBox->currentText();
    //str = "opc.tcp://169.254.116.116:4840";
    qDebug()<<"Finding UA server @:"<<str;
    char *url;
    QByteArray ba = str.toLatin1();
    url = ba.data();    //url converted from var 'str' defined above

    /* Listing endpoints */
    UA_EndpointDescription* endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, url, &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
        return;
    }
    print("%i endpoints found\n", (int)endpointArraySize); //endpoints found
    //list the endpoints
    for(size_t i=0;i<endpointArraySize;i++) {
        print("URL of endpoint %i is %.*s\n", (int)i,  //2 URL of endpoint 0 is opc.tcp://localhost:4840
               (int)endpointArray[i].endpointUrl.length,
               endpointArray[i].endpointUrl.data);
    }
    UA_Array_delete(endpointArray,endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);

    //check the 'connect btn' and execute the relevant logic (connect/disconnect)
    if (ui->btnConnect->text()=="Connect"){             //do connect
        printf("Connecting...\n");
        /* Connect to server */
        /* anonymous connect would be: retval = UA_Client_connect(client, "opc.tcp://localhost:4840"); */
        //retval = UA_Client_connect_username(client, url, "user1", "password");
        retval = UA_Client_connect(client, url);    //connect to server without authority verify
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return;
        }else{
            print("Connected!\n");
            ui->lineEdit_URL->setDisabled(true);
            ui->btnConnect->setText("Disconnect");
            chgEnable(false);
        }
    }else if(ui->btnConnect->text()=="Disconnect"){     //do disconnect
        printf("Disconnecting...\n");
        retval = UA_Client_disconnect(client);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_delete(client);
            return;
        }else{
            print("Disconnected!\n");
            ui->lineEdit_URL->setDisabled(false);
            ui->btnConnect->setText("Connect");
            chgEnable(true);
        }
    };
    fflush(stdout);
}


void MainWindow::on_BtnReadInt_clicked()                   //INT16 value <- codesys int variable
{
    UA_Int16 value = 0;

    /* Read attribute */
    int nsi = ui->lineEdit_namespace->text().toInt();   //namespaceIndex -> nsi
    UA_Variant *val = UA_Variant_new();

    QString str = ui->lineEdit_id_string->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
    if(retval == 0 && UA_Variant_isScalar(val) &&
       val->type == &UA_TYPES[UA_TYPES_INT16]) {    //UA_TYPES_INT32 -> INT16, codesys use int16 for INT variable
            value = *(UA_Int16*)val->data;          //use int16 to instead of int32
            ui->lineEdit_value->setText(QString::number(value));
            print("the value is: %i\n", value);
    }else{
        printf("read error\n");ui->lineEdit_value->setText("");
    }

    fflush(stdout);
    UA_Variant_delete(val);
}



void MainWindow::on_BtnWriteInt_clicked()                   //INT16 value -> codesys int variable
{
    int value = ui->lineEdit_value->text().toInt();         //Value

    /* Write node attribute (using the highlevel API) */
    int nsi = ui->lineEdit_namespace->text().toInt();       //NameSpaceIndex
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT16]);
    QString str = ui->lineEdit_id_string->text();
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    printf("Input value is: %d\n",value);
    retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsi, isting), myVariant);
    if(retval == UA_STATUSCODE_GOOD){
        printf("write success\n");
    }else{
        printf("write failed\n");
    }
    UA_Variant_delete(myVariant);

    fflush(stdout);
}


void MainWindow::on_BtnReadReal_clicked()               //Float value <- codesys REAL variable
{

    UA_Float value = 0;

    /* Read attribute */
    int nsi = ui->nsiReal->text().toInt();   //namespaceIndex -> nsi
    UA_Variant *val = UA_Variant_new();
    QString str = ui->IdStringReal->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_FLOAT]) {    //use UA_TYPES_FLOAT while codesys use REAL variable
            value = *(UA_Float*)val->data;
            ui->ValueReal->setText(QString::number(value));
            print("the value is: %f\n", value);
    }else{
        printf("read error\n");ui->ValueReal->setText("");
    }

    fflush(stdout);
    UA_Variant_delete(val);
}


void MainWindow::on_BtnWriteReal_clicked()      //Float value -> Codesys REAL variable
{

    float value = ui->ValueReal->text().toFloat();         //convert input string value to float value

    /* Write node attribute (using the highlevel API) */
    int nsi = ui->nsiReal->text().toInt();       //NameSpaceIndex
    printf("Input value is: %f\n",value);
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_FLOAT]);
    QString str = ui->IdStringReal->text();     //Identifier string
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();

    retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsi, isting), myVariant);
    if(retval == UA_STATUSCODE_GOOD){
        printf("write success\n");
    }else{
        printf("write failed\n");
    }
    UA_Variant_delete(myVariant);

    fflush(stdout);
}


void MainWindow::on_BtnReadString_clicked()     //string value <- codesys STRING variable
{
    int nsi = ui->nsiString->text().toInt();   //namespaceIndex -> nsi
    /* Read attribute */
    UA_String value;
    UA_Variant *val = UA_Variant_new();
    QString str = ui->IdStringString->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_STRING]) {    //use UA_TYPES_FLOAT while codesys use REAL variable
            value = *(UA_String*)val->data;
            QByteArray text;
            for (int i=0; i<int(value.length); i++){
                text[i]=value.data[i];
            }
            ui->ValueString->setText(text.data());
            print("the value is: %s\n", text.data());
    }else{
        printf("read error\n");ui->ValueString->setText("");
    }

    fflush(stdout);
    UA_Variant_delete(val);

}


void MainWindow::on_BtnWriteString_clicked()     //string value -> codesys STRING variable
{
    int nsi = ui->nsiString->text().toInt();       //NameSpaceIndex
    QString str = ui->IdStringString->text();     //Identifier string
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();

    /* Write node attribute (using the highlevel API) */
    QString input = ui->ValueString->text();    //get the string value from input widget
    QByteArray text = input.toLatin1();         //convert input string value in to a character array
    UA_String value;                            //define a UA_String variable for communicating
    value = UA_String_fromChars(text);          //use exsit function to init the 'UA_String' type variable


    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_STRING]);

    retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsi, isting), myVariant);
    if(retval == UA_STATUSCODE_GOOD){
        printf("write success\n");
    }else{
        printf("write failed\n");
    }
    UA_Variant_delete(myVariant);

    fflush(stdout);
}


void MainWindow::on_BtnReadBool_clicked()       //Boolean value <- codesys bool variable
{
    UA_Boolean value = 0;
    int nsi = ui->nsiBool->text().toInt();   //namespaceIndex -> nsi
    /* Read attribute */
    UA_Variant *val = UA_Variant_new();
    QString str = ui->IdStringBool->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);
    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);

    if(retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val) && val->type == &UA_TYPES[UA_TYPES_BOOLEAN]) {
            value = *(UA_Boolean*)val->data;
            ui->ValueBool->setText(QString::number(value));
            print("the value is: %d\n", value);
    }else{
        printf("read error\n");ui->ValueBool->setText("");
    }

    fflush(stdout);
    UA_Variant_delete(val);
}


void MainWindow::on_BtnWriteBool_clicked()       //Boolean value -> codesys bool variable
{
    bool value = ui->ValueBool->text().toFloat();         //convert input string value to float value

    /* Write node attribute (using the highlevel API) */
    int nsi = ui->nsiBool->text().toInt();       //NameSpaceIndex
    printf("Input value is: %d\n",value);
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_BOOLEAN]);
    QString str = ui->IdStringBool->text();     //Identifier string
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();

    retval = UA_Client_writeValueAttribute(client, UA_NODEID_STRING(nsi, isting), myVariant);
    if(retval == UA_STATUSCODE_GOOD){
        printf("write success\n");
    }else{
        printf("write failed\n");
    }
    UA_Variant_delete(myVariant);

    fflush(stdout);
}


//void MainWindow::on_btnDisconnect_clicked()
//{

//}

