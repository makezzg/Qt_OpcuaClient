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

    ui->nsiInt->setText(QString::number(4));
    ui->IdStringInt->setText(QString("|var|PAC120-PXX01-3X-XX-XX.Application.GVL.mi_testINT"));
    ui->lineEdit_URL->setText(QString("opc.tcp://169.254.116.116:4840"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chgEnable(boolean enable)
{
    ui->nsiInt->setDisabled(enable);
    ui->IdStringInt->setDisabled(enable);
    ui->ValueInt->setDisabled(enable);
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
    int nsi = ui->nsiInt->text().toInt();   //namespaceIndex -> nsi
    UA_Variant *val = UA_Variant_new();

    QString str = ui->IdStringInt->text();   //identifier.string -> str
    char *isting;
    QByteArray ba = str.toLatin1();
    isting = ba.data();
    print("\nReading the value of node (%d, \"%s\"):\n",nsi, isting);

    retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(nsi, isting), val);
    if(retval == 0 && UA_Variant_isScalar(val) &&
       val->type == &UA_TYPES[UA_TYPES_INT16]) {    //UA_TYPES_INT32 -> INT16, codesys use int16 for INT variable
            value = *(UA_Int16*)val->data;          //use int16 to instead of int32
            ui->ValueInt->setText(QString::number(value));
            print("the value is: %i\n", value);
    }else{
        printf("read error\n");ui->ValueInt->setText("");
    }

    fflush(stdout);
    UA_Variant_delete(val);
}



void MainWindow::on_BtnWriteInt_clicked()                   //INT16 value -> codesys int variable
{
    int value = ui->ValueInt->text().toInt();         //Value

    /* Write node attribute (using the highlevel API) */
    int nsi = ui->nsiInt->text().toInt();       //NameSpaceIndex
    UA_Variant *myVariant = UA_Variant_new();
    UA_Variant_setScalarCopy(myVariant, &value, &UA_TYPES[UA_TYPES_INT16]);
    QString str = ui->IdStringInt->text();
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

