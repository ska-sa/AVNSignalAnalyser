#-------------------------------------------------
#
# Project created by QtCreator 2015-07-03T10:48:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AVNSignalAnalyser
TEMPLATE = app


SOURCES +=\
        MainWindow.cpp \
    Main.cpp \
    NetworkGroupBox.cpp \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingTCPSocket.cpp \
    AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.cpp \
    QwtLinePlotWidget.cpp \
    QwtWaterfallPlotWidget.cpp \
    PlotsWidget.cpp \
    QwtLinePlotPicker.cpp


HEADERS  += MainWindow.h \
    NetworkGroupBox.h \
    AVNUtilLibs/DataStructures/ThreadSafeCircularBuffer/ThreadSafeCircularBuffer.h \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingTCPSocket.h \
    AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.h \
    QwtLinePlotWidget.h \
    QwtWaterfallPlotWidget.h \
    PlotsWidget.h \
    QwtLinePlotPicker.h


FORMS    += MainWindow.ui \
    NetworkGroupBox.ui \
    QwtWaterfallPlotWidget.ui \
    QwtLinePlotWidget.ui \
    PlotsWidget.ui

RESOURCES += \
    Images.qrc

unix{
    #X11 (required for osgEarth rendering)
    LIBS += -lX11

    #Qwt Ubuntu:
    #DEFINES    += QT_DLL QWT_DLL
    #LIBS += -lqwt
    #INCLUDEPATH += /usr/include/qwt

    #Qwt Gentoo:
    DEFINES    += QT_DLL QWT_DLL
    LIBS += -lqwt6
    INCLUDEPATH += /usr/include/qwt6

    #Boost
    LIBS += -lboost_system -lboost_thread -lboost_chrono
}

win32{
    #Qwt
    DEFINES    += QT_DLL QWT_DLL
    LIBS += -LC://DevLibs//qwt-6.1.0//lib
    CONFIG(debug, debug|release) {
        LIBS += -lqwtd
    } else {
        LIBS += -lqwt
    }
    INCLUDEPATH +=  C://DevLibs//qwt-6.1.0//src

    #Boost
    CONFIG(debug, debug|release) {
        LIBS += -LC:\\DevLibs\\boost\\boost_1_55_0\\lib64-msvc-10.0 -lboost_system-vc100-mt-gd-1_55 -lboost_thread-vc100-mt-gd-1_55 -lboost_chrono-vc100-mt-gd-1_55
    } else {
        LIBS += -LC:\\DevLibs\\boost\\boost_1_55_0\\lib64-msvc-10.0 -lboost_system-vc100-mt-1_55 -lboost_thread-vc100-mt-1_55 -lboost_chrono-vc100-mt-1_55
    }
    INCLUDEPATH += C:\\DevLibs\\boost\\boost_1_55_0

    #Winsock
    LIBS += Ws2_32.lib


    #Set application executable icon:
    RC_FILE = AVNSignalAnalyser_win32.rc

}
