//System includes

//Library includes

//Local includes
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    cMainWindow w;
    w.show();

    return a.exec();
}
