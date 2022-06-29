#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include "open62541.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    UA_Client *client;
    UA_StatusCode retval;
    void chgEnable(boolean enable);         //function that set/reset the widgets' enable attribute on window

private slots:
    void on_pushButton_connect_clicked();

    void on_pushButton_connect_2_clicked();

    void on_pushButton_string_read_clicked();

    void on_pushButton_string_write_clicked();

    void on_pushButton_string_read_2_clicked();

    void on_pushButton_add_value_clicked();

    void on_pushButton_string_read_4_clicked();

    void on_pushButton_string_read_5_clicked();

    void on_pushButton_string_read_6_clicked();

    void on_BtnWriteInt_clicked();

    void on_BtnReadReal_clicked();

    void on_BtnWriteReal_clicked();

    void on_BtnReadString_clicked();

    void on_BtnWriteString_clicked();

    void on_BtnReadBool_clicked();

    void on_BtnWriteBool_clicked();

    void on_BtnReadInt_clicked();

    void on_btnConnect_clicked();

    //void on_btnDisconnect_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
