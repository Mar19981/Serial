#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Serial.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <Windows.h>
#include <chrono>

class Serial : public QMainWindow
{
    Q_OBJECT

public:
    Serial(QWidget *parent = Q_NULLPTR);

private:
    Ui::SerialClass ui;
    QSerialPort serialPort;
    DCB dcbParams;
    QString terminator;
    std::string transmitedText;
    HANDLE fileHandle;
    void changeInputAvailability(bool);
private slots:
    void openSerialPort();
    void pingPort();
    void closeSerialPort();
    void refreshPortsList();
    void setBaudrate(int);
    void setStopBits(int);
    void setParity(int);
    void setBits(int);
    void setControlFlow(int);
    void setTerminator(int);
    void setCustomTerminator(QString);
    void setTransmitedText(QString);
    void sendText();
};
