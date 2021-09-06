#include "Serial.h"

Serial::Serial(QWidget *parent)
    : QMainWindow(parent), dcbParams({})
{
    FillMemory(&dcbParams, sizeof(dcbParams), 0);
    dcbParams.DCBlength = sizeof(dcbParams);
    if (!BuildCommDCB(TEXT("9600,E,7,1"), &dcbParams)) throw std::runtime_error("Failed to build DCB struct");

    dcbParams.XoffChar = 0x13; //ASCII XOFF char
    dcbParams.XonChar = 0x11; //ASCII XON char
    
    ui.setupUi(this);

    refreshPortsList();

    connect(ui.openButton, SIGNAL(clicked()), this, SLOT(openSerialPort()));
    connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(closeSerialPort()));
    connect(ui.refreshPortsButton, SIGNAL(clicked()), this, SLOT(refreshPortsList()));
    connect(ui.sendButton, SIGNAL(clicked()), this, SLOT(sendText()));
    connect(ui.bitrateSpin, SIGNAL(valueChanged(int)), this, SLOT(setBaudrate(int)));
    connect(ui.stopBitSpin, SIGNAL(valueChanged(int)), this, SLOT(setStopBits(int)));
    connect(ui.bitSpin, SIGNAL(valueChanged(int)), this, SLOT(setBits(int)));
    connect(ui.characterControlCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setParity(int)));
    connect(ui.flowCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setControlFlow(int)));
    connect(ui.terminatorCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setTerminator(int)));
    connect(ui.terminatorEdit, SIGNAL(textChanged(QString)), this, SLOT(setCustomTerminator(QString)));
    connect(ui.transmitTextEdit, SIGNAL(textChanged(QString)), this, SLOT(setTransmitedText(QString)));
    connect(ui.pingButton, SIGNAL(clicked()), this, SLOT(pingPort()));
}

void Serial::openSerialPort()
{
    if (!ui.portsCombo->count()) throw std::runtime_error("No port selected!");
    fileHandle = CreateFileA(
        ui.portsCombo->currentText().toStdString().c_str(),									
        GENERIC_READ | GENERIC_WRITE,				
        NULL,										
        NULL,										
        OPEN_EXISTING,								
        FILE_ATTRIBUTE_NORMAL,						
        NULL										
    );
    if (fileHandle == INVALID_HANDLE_VALUE) throw std::runtime_error("Unable to open port!");
    PurgeComm(fileHandle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    if (!SetCommState(fileHandle, &dcbParams)) throw std::runtime_error("Unable to set params!");

    COMMTIMEOUTS timeout{};

    memset((void*)&timeout, 0, sizeof(timeout));
    timeout.ReadIntervalTimeout = 500;				
    timeout.ReadTotalTimeoutMultiplier = 1;			
    timeout.ReadTotalTimeoutConstant = 5000;		
    SetCommTimeouts(fileHandle, &timeout);
    PurgeComm(fileHandle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    ui.closeButton->setEnabled(true);
    ui.pingButton->setEnabled(true);
    if (ui.transmitRadio->isChecked()) {
        ui.sendButton->setEnabled(true);
        ui.transmitTextEdit->setEnabled(true);
        changeInputAvailability(false);
    }
    else {
        DWORD numberofBytesRead = -1;
        std::string out{};
        char byte{};
        int terminatorLength = terminator.length();
        if (terminatorLength) {
            int terminatorCharsDetected{};
            do {
                if (!ReadFile(fileHandle, &byte, 1, &numberofBytesRead, nullptr)) {
                    ClearCommError(fileHandle, 0, 0);
                    throw std::runtime_error("Failed to read data");
                }
                if (byte == terminator[terminatorCharsDetected]) terminatorCharsDetected++;
                if (terminatorCharsDetected == terminatorLength) break;
                out += byte;
            } while (numberofBytesRead);
            if (terminatorCharsDetected == 2) out.pop_back();
        }
        else {
            do {
                if (!ReadFile(fileHandle, &byte, 1, &numberofBytesRead, nullptr)) {
                    ClearCommError(fileHandle, 0, 0);
                    throw std::runtime_error("Failed to read data");
                }
                out += byte;
            } while (numberofBytesRead);
        }
        ui.receiveTextBox->setText(out.c_str());
        closeSerialPort();
    }
}

void Serial::changeInputAvailability(bool state)
{
    ui.openButton->setEnabled(state);
    ui.portsCombo->setEnabled(state);
    ui.bitrateSpin->setEnabled(state);
    ui.stopBitSpin->setEnabled(state);
    ui.bitSpin->setEnabled(state);
    ui.characterControlCombo->setEnabled(state);
    ui.flowCombo->setEnabled(state);
    ui.terminatorCombo->setEnabled(state);
    if (state && ui.terminatorCombo->currentIndex() == 4) ui.terminatorEdit->setEnabled(state);
    else ui.terminatorEdit->setEnabled(false);
}

void Serial::pingPort()
{
    auto t1 = std::chrono::high_resolution_clock::now();
    DWORD bytesWritten, bytesRead;
    uint8_t bytes[8]{};
    WriteFile(fileHandle, bytes, 8, &bytesWritten, nullptr);
    ReadFile(fileHandle, bytes, 8, &bytesRead, nullptr);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time = t2 - t1;
    ui.rtdLabel->setText("Round trip delay: " + QString::number(time.count()) + "ms");

}

void Serial::closeSerialPort()
{
    ui.closeButton->setEnabled(false);
    ui.pingButton->setEnabled(false);
    if (ui.transmitRadio->isChecked()) {
        ui.sendButton->setEnabled(false);
        ui.transmitTextEdit->setEnabled(false);
    }
    changeInputAvailability(true);
    PurgeComm(fileHandle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
    CloseHandle(fileHandle);
}

void Serial::refreshPortsList()
{
    ui.portsCombo->clear();
    auto ports = QSerialPortInfo::availablePorts();
    for (auto portInfo : ports)
    {
        ui.portsCombo->addItem(portInfo.portName());
    }
}

void Serial::setBaudrate(int value)
{
    dcbParams.BaudRate = value;
}

void Serial::setStopBits(int value)
{
    switch (value) {
        case 1: dcbParams.StopBits = ONESTOPBIT; break;
        case 2: dcbParams.StopBits = TWOSTOPBITS; break;
    } 
}

void Serial::setParity(int value)
{
    switch (value) {
        case 0: dcbParams.Parity = EVENPARITY; break;
        case 1: dcbParams.Parity = PARITY_NONE; break;
        case 2: dcbParams.Parity = ODDPARITY; break;
    }
}

void Serial::setBits(int value)
{
    dcbParams.ByteSize = value;
}

void Serial::setControlFlow(int value)
{
    switch (value) {
    case 0: {
        dcbParams.fOutxCtsFlow = dcbParams.fDsrSensitivity = false;
        dcbParams.fRtsControl = RTS_CONTROL_DISABLE;
        dcbParams.fDtrControl = DTR_CONTROL_DISABLE;
        }; break;
    case 1: {
        dcbParams.fDtrControl = DTR_CONTROL_HANDSHAKE;
        dcbParams.fDsrSensitivity = true;
        dcbParams.fOutxCtsFlow = false;
        dcbParams.fRtsControl = RTS_CONTROL_DISABLE;
    }; break;
    case 2: {
        dcbParams.fOutxCtsFlow = true;
        dcbParams.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcbParams.fDtrControl = DTR_CONTROL_DISABLE;
        dcbParams.fDsrSensitivity = false;
    }
    }
}

void Serial::setTerminator(int value)
{
    if (value != 4) ui.terminatorEdit->setEnabled(false);
    else ui.terminatorEdit->setEnabled(true);
    switch (value) {
        case 0: terminator.clear(); break;
        case 1: terminator = '\r'; break;
        case 2: terminator = '\n'; break;
        case 3: terminator = "\r\n"; break;
        case 4: terminator = ui.terminatorEdit->text(); break;
    }
}

void Serial::setCustomTerminator(QString value)
{
    terminator = value;
}

void Serial::setTransmitedText(QString txt)
{
    transmitedText = txt.toStdString() + terminator.toStdString();
}

void Serial::sendText()
{
    auto msg = transmitedText.c_str();
    DWORD dataSize{};
    int i = WriteFile(fileHandle, msg, transmitedText.size(), &dataSize, nullptr);
    if (!i) throw std::runtime_error("Writing to port failed");
}
