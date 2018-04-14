/*******************************************************************************
 * Copyright [2018] <青岛艾普智能仪器有限公司>
 * All rights reserved.
 *
 * version:     0.1
 * author:      zhaonanlin
 * brief:       松下伺服PLC调试助手
*******************************************************************************/
#include "appplcfpx.h"

typedef int (AppPlcfpx::*pClass)(void);
QMap<int, pClass> taskMap;

AppPlcfpx::AppPlcfpx(QWidget *parent)
    : QMainWindow(parent)
{
    initUI();
    initParam();
}

AppPlcfpx::~AppPlcfpx()
{
}

void AppPlcfpx::initUI()
{
    initSkin();
    initTitle();
    initLayout();
    initDevPort();
    initDevMode();
    initDevTurn();
    initDevSpeed();
    initDevTorque();
    initDevButton();
}

void AppPlcfpx::initSkin()
{
    QFile file;
    QString qss;
    file.setFileName(":/qss_black.css");
    file.open(QFile::ReadOnly);
    qss = QLatin1String(file.readAll());
    qApp->setStyleSheet(qss);
}

void AppPlcfpx::initTitle()
{
    char s_month[5];
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    int month, day, year;

    sscanf((__DATE__), "%s %d %d", s_month, &day, &year);
    month = (strstr(month_names, s_month)-month_names)/3+1;

    QDate dt;
    dt.setDate(year, month, day);
    static const QTime tt = QTime::fromString(__TIME__, "hh:mm:ss");

    QDateTime t(dt, tt);
    QString verNumb = QString("V-0.1.%1").arg(t.toString("yyMMdd-hhmm"));

    this->setWindowTitle(tr("松下伺服PLC调试助手%1").arg(verNumb));
}

void AppPlcfpx::initParam()
{
    com = NULL;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(taskThread()));
    timer->start(10);

    taskMap[0] = &AppPlcfpx::getTest;
    taskMap[1] = &AppPlcfpx::setMode;
    taskMap[2] = &AppPlcfpx::waitRet;
    taskMap[3] = &AppPlcfpx::setTurn;
    taskMap[4] = &AppPlcfpx::waitRet;
    taskMap[5] = &AppPlcfpx::setData;
    taskMap[6] = &AppPlcfpx::waitRet;
    taskMap[7] = &AppPlcfpx::setTest;
    taskMap[8] = &AppPlcfpx::waitRet;

    currMap = 0;
    isStart = 0;
    timeOut = 0;
    timeRep = 0;
}

void AppPlcfpx::initLayout()
{
    QVBoxLayout *layout = new QVBoxLayout;

    text = new QTextBrowser(this);
    layout->addWidget(text);

    btnLayout = new QHBoxLayout;
    layout->addLayout(btnLayout);

    layout->addWidget(new QLabel(tr("松下伺服PLC调试助手 by link"), this));

    QFrame *frame = new QFrame(this);
    frame->setLayout(layout);
    this->setCentralWidget(frame);
    this->resize(800, 600);
}

void AppPlcfpx::initDevPort()
{
    QStringList com;
#ifndef __linux__
    QString path = "HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM";
    QSettings *settings = new QSettings(path, QSettings::NativeFormat);
    QStringList key = settings->allKeys();
    HKEY hKey;
    int ret = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                             0, KEY_READ, &hKey);
    if (ret != 0) {
        qDebug() << "Cannot open regedit!";
    } else {
        for (int i=0; i < key.size(); i++) {
            wchar_t name[256];
            DWORD ikey = sizeof(name);
            char kvalue[256];
            DWORD t = sizeof(kvalue);
            DWORD type;
            QString tmpName;
            ret = ::RegEnumValue(hKey, i, LPWSTR(name), &ikey, 0, &type,
                                 reinterpret_cast<BYTE*>(kvalue), &t);
            if (ret == 0) {
                for (int j = 0; j < static_cast<int>(t); j++) {
                    if (kvalue[j] != 0x00) {
                        tmpName.append(kvalue[j]);
                    }
                }
                com << tmpName;
            }
        }
        RegCloseKey(hKey);
    }
#else
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        com << info.portName();
    }
#endif
    com.sort();
    for (int i=0; i < com.size(); i++) {
        if (com.at(i).size() > 4) {
            com.move(i, com.size()-1);
        }
    }
    btnLayout->addWidget(new QLabel(tr("串口:"), this));
    boxDevPort = new QComboBox(this);
    boxDevPort->addItems(com);
    boxDevPort->setView(new QListView);
    boxDevPort->setMinimumSize(77, 35);
    btnLayout->addWidget(boxDevPort);
    btnLayout->addStretch();
}

void AppPlcfpx::initDevMode()
{
    btnLayout->addWidget(new QLabel(tr("模式:"), this));

    QStringList com;
    com << tr("速度模式") << tr("转矩模式");

    boxDevMode = new QComboBox(this);
    boxDevMode->addItems(com);
    boxDevMode->setView(new QListView);
    boxDevMode->setMinimumSize(97, 35);
    btnLayout->addWidget(boxDevMode);
}

void AppPlcfpx::initDevTurn()
{
    btnLayout->addWidget(new QLabel(tr("转向:"), this));

    QStringList com;
    com << tr("顺时针") << tr("逆时针");

    boxDevTurn = new QComboBox(this);
    boxDevTurn->addItems(com);
    boxDevTurn->setView(new QListView);
    boxDevTurn->setMinimumSize(77, 35);
    btnLayout->addWidget(boxDevTurn);
}

void AppPlcfpx::initDevSpeed()
{
    btnLayout->addWidget(new QLabel(tr("转度:"), this));

    speed = new QLineEdit(this);
    speed->setText("1000");
    speed->setFixedHeight(35);
    btnLayout->addWidget(speed);
}

void AppPlcfpx::initDevTorque()
{
    btnLayout->addWidget(new QLabel(tr("转矩:"), this));

    torque = new QLineEdit(this);
    torque->setText("0.5");
    torque->setFixedHeight(35);
    btnLayout->addWidget(torque);
}

void AppPlcfpx::initDevButton()
{
    QPushButton *btnCurr = new QPushButton(this);
    btnCurr->setText(tr("启动"));
    btnCurr->setMinimumSize(66, 35);
    connect(btnCurr, SIGNAL(clicked(bool)), this, SLOT(initTask()));
    btnLayout->addWidget(btnCurr);

    QPushButton *btnCurrStop = new QPushButton(this);
    btnCurrStop->setText(tr("停止"));
    btnCurrStop->setMinimumSize(66, 35);
    connect(btnCurrStop, SIGNAL(clicked(bool)), this, SLOT(setStop()));
    btnLayout->addWidget(btnCurrStop);
}

int AppPlcfpx::getTest()
{
    if (isStart == QMessageBox::Yes) {
        isStart = QMessageBox::No;
        return QMessageBox::Apply;
    } else {
        return QMessageBox::Reset;
    }
}

int AppPlcfpx::setMode()
{ // 目标站号0x01,指命代码WCS,触点代码R,触点编号0x0001,触点数据0/1
    QByteArray msg = "%01#WCSR0001";
    msg.append(QString::number(boxDevMode->currentIndex()));
    msg.append("**\r");
    return writeMsg(msg);
}

int AppPlcfpx::setTurn()
{ // 目标站号0x01,指命代码WCS,触点代码R,触点编号0x0003,触点数据0/1
    QByteArray msg = "%01#WCSR0003";
    msg.append(QString::number(boxDevTurn->currentIndex()));
    msg.append("**\r");
    return writeMsg(msg);
}

int AppPlcfpx::setData()
{ // 目标站号0x01,指命代码WD,数据代码D,起始数据编码00100,结束数据编码00101
    int s = (boxDevMode->currentIndex() == 0) ? speed->text().toInt()*2 : 0;
    int t = (boxDevMode->currentIndex() == 1) ? torque->text().toDouble()*2500 : 0;
    QByteArray msg = "%01#WDD0010000101";
    msg.append(QString("%1").arg(s%256, 2, 16, QChar('0')).toUpper().toUtf8());
    msg.append(QString("%1").arg(s/256, 2, 16, QChar('0')).toUpper().toUtf8());
    msg.append(QString("%1").arg(t%256, 2, 16, QChar('0')).toUpper().toUtf8());
    msg.append(QString("%1").arg(t/256, 2, 16, QChar('0')).toUpper().toUtf8());
    msg.append("**\r");  // 设置转速时转矩设置为0
    return writeMsg(msg);
}

int AppPlcfpx::setTest()
{  // 目标站号0x01,指命代码WCS,触点代码R,触点编号0x0000,触点数据0/1
    return writeMsg("%01#WCSR00001**\r");
}

int AppPlcfpx::setStop()
{  // 目标站号0x01,指命代码WCS,触点代码R,触点编号0x0000,触点数据0/1
    return writeMsg("%01#WCSR00000**\r");
}

int AppPlcfpx::waitRet()
{
    if (com == NULL || !com->isOpen()) {
        lastError = tr("串口未打开");
        return QMessageBox::Abort;
    }
    int ret = QMessageBox::Retry;;
    if (com->bytesAvailable() > 0) {
        tmpByte.append(com->readAll());
        if (tmpByte.endsWith("\r") && tmpByte.contains("$")) {
            ret = QMessageBox::Apply;
            QString tstring = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            text->insertPlainText(tr("[%1] ").arg(tstring));
            text->insertPlainText("com recv: ");
            text->insertPlainText(tmpByte);
            qDebug() << "com recv:" << tmpByte;
            tmpByte.clear();
        }
    }
    return ret;
}

int AppPlcfpx::writeMsg(QByteArray msg)
{
    tmpByte.clear();
    if (com == NULL || !com->isOpen()) {
        lastError = tr("串口未打开");
        return QMessageBox::Abort;
    }
    if (com->bytesAvailable() > 0) {
        com->readAll();
    }
    if (com->write(msg) != msg.size()) {
        lastError = tr("串口写入失败");
        return QMessageBox::Abort;
    }
    QString tstring = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    text->insertPlainText(tr("[%1] ").arg(tstring));
    text->insertPlainText("com send: ");
    text->insertPlainText(msg);
    qDebug() << "com send:" << msg;
    return QMessageBox::Apply;
}

int AppPlcfpx::initTask()
{
    text->clear();
    if (com != NULL && com->isOpen()) {
        com->close();
    }
    com = new QSerialPort(boxDevPort->currentText(), this);
    if (com->open(QIODevice::ReadWrite)) {
        com->setBaudRate(9600);
        com->setDataBits(QSerialPort::Data8);
        com->setParity(QSerialPort::OddParity);
        com->setStopBits(QSerialPort::OneStop);
        com->setFlowControl(QSerialPort::NoFlowControl);
        com->setDataTerminalReady(true);
        com->setRequestToSend(false);
        isStart = QMessageBox::Yes;
    } else {
        qDebug() << "com open error";
    }
    return QMessageBox::Apply;
}



int AppPlcfpx::taskThread()
{
    int ret = QMessageBox::Abort;
    if (taskMap.keys().contains(currMap))
        ret = (this->*taskMap[currMap])();
    switch (ret) {
    case QMessageBox::Apply:
        currMap = (taskMap.keys().contains(currMap+1)) ? (currMap+1) : 0;
        timeOut = 0;
        break;
    case QMessageBox::Retry:
        timeOut++;
        if (timeOut%30 == 0) {  // 300ms无正确应答,重复发送
            currMap = (currMap <= 0) ? 0 : (currMap-1);
            timeRep++;
            if (timeRep >= 3) {  // 重复3次后无应答,发出警告并退出
                ret = QMessageBox::Abort;
                lastError = tr("通信无应答");
            }
        }
        break;
    default:
        break;
    }
    if (ret == QMessageBox::Abort) {
        QMessageBox::StandardButtons button = QMessageBox::Abort | QMessageBox::Retry;
        button = QMessageBox::warning(this, tr("警告"), lastError, button);
        if (button == QMessageBox::Abort) {
            currMap = 0;
        }
        timeOut = 0;
        timeRep = 0;
    }
    return ret;
}
