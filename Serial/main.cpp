#include "Serial.h"
#include <QtWidgets/QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Serial w;
    try {
        w.show();
        return a.exec();
    }
    catch (std::exception err) {
        QMessageBox::warning(&w, "Error", err.what());
        return a.exec();
    }
}
