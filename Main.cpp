//System includes

//Library includes

//Local includes
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("AVNSignalAnalyser");
    a.setApplicationVersion("0.0.1b");
    a.setOrganizationName("SKA SA");
    a.setOrganizationDomain("ska.ac.za");

    cMainWindow w;
    w.show();

    return a.exec();
}
