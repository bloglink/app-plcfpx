/*******************************************************************************
 * Copyright [2018] <青岛艾普智能仪器有限公司>
 * All rights reserved.
 *
 * version:     0.1
 * author:      zhaonanlin
 * brief:       松下伺服PLC调试助手
*******************************************************************************/
#ifndef APPPLCFPX_H
#define APPPLCFPX_H

#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QLabel>
#include <QLayout>
#include <QWidget>
#include <QSettings>
#include <QDateTime>
#include <QListView>
#include <QLineEdit>
#include <QComboBox>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QApplication>
#include <QTextBrowser>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#ifndef __linux__
#include <qt_windows.h>
#endif

class AppPlcfpx : public QMainWindow
{
    Q_OBJECT

public:
    explicit AppPlcfpx(QWidget *parent = 0);
    ~AppPlcfpx();
private slots:
    void initUI();
    void initSkin();
    void initTitle();
    void initParam();
    void initLayout();
    void initDevPort();
    void initDevMode();
    void initDevTurn();
    void initDevSpeed();
    void initDevTorque();
    void initDevButton();
    int getTest();
    int setMode();
    int setTurn();
    int setData();
    int setTest();
    int setStop();
    int waitRet();
    int writeMsg(QByteArray msg);
    int initTask();
    int taskThread();
private:
    QTextBrowser *text;
    QHBoxLayout *btnLayout;
    QComboBox *boxDevPort;
    QComboBox *boxDevMode;
    QComboBox *boxDevTurn;
    QLineEdit *torque;
    QLineEdit *speed;
    QSerialPort *com;
    QTimer *timer;
    QByteArray tmpByte;
    int timeOut;
    int timeRep;
    int currMap;
    int isStart;
    int currTest;
    QString lastError;
};

#endif // APPPLCFPX_H
