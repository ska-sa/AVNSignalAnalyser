#-------------------------------------------------
#
# Project created by QtCreator 2015-07-03T10:48:57
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AVNSignalAnalyser
TEMPLATE = app

DEFINES += USE_BOOST_TIME=1 #Use boost libs for timestamp derivation

SOURCES +=\
        MainWindow.cpp \
    Main.cpp \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingTCPSocket.cpp \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingUDPSocket.cpp \
    AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.cpp \
    PlotsWidget.cpp \
    AVNDataTypes/SpectrometerDataStream/SpectrometerHeader.cpp \
    AVNGUILibs/QwtPlotting/BasicQwtLinePlotWidget.cpp \
    AVNGUILibs/QwtPlotting/FramedQwtLinePlotWidget.cpp \
    AVNGUILibs/QwtPlotting/ScrollingQwtLinePlotWidget.cpp \
    AVNGUILibs/QwtPlotting/BandPowerQwtLinePlotWidget.cpp \
    AVNGUILibs/QwtPlotting/CursorCentredQwtPlotMagnifier.cpp \
    AVNGUILibs/QwtPlotting/AnimatedQwtPlotZoomer.cpp \
    AVNAppLibs/SocketStreamers/SocketReceiverBase.cpp \
    AVNAppLibs/SocketStreamers/UDPReceiver/UDPReceiver.cpp \
    NetworkConnectionWidget.cpp \
    AVNDataTypes/SpectrometerDataStream/SpectrometerDataStreamInterpreter.cpp \
    RoachAcquistionControlWidget.cpp \
    AVNUtilLibs/Timestamp/Timestamp.cpp \
    AVNDataTypes/SpectrometerDataStream/SpectrometerDefinitions.cpp \
    KATCPClient.cpp \
    AboutDialog.cpp \
    AVNGUILibs/QwtPlotting/WaterfallQwtPlotWidget.cpp \
    AVNGUILibs/QwtPlotting/WaterfallPlotSpectromgramData.cpp \
    AVNGUILibs/QwtPlotting/WallTimeQwtScaleDraw.cpp \
    AVNGUILibs/QwtPlotting/QwtPlotDistancePicker.cpp \
    AVNGUILibs/QwtPlotting/QwtPlotWidgetBase.cpp \
    AVNGUILibs/QwtPlotting/QwtPlotPositionPicker.cpp

HEADERS  += MainWindow.h \
    AVNUtilLibs/DataStructures/ThreadSafeCircularBuffer/ThreadSafeCircularBuffer.h \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingTCPSocket.h \
    AVNUtilLibs/Sockets/InterruptibleBlockingSockets/InterruptibleBlockingUDPSocket.h \
    AVNAppLibs/SocketStreamers/TCPReceiver/TCPReceiver.h \
    PlotsWidget.h \
    AVNDataTypes/SpectrometerDataStream/SpectrometerDefinitions.h \
    AVNDataTypes/SpectrometerDataStream/SpectrometerHeader.h \
    AVNGUILibs/QwtPlotting/BasicQwtLinePlotWidget.h \
    AVNGUILibs/QwtPlotting/FramedQwtLinePlotWidget.h \
    AVNGUILibs/QwtPlotting/ScrollingQwtLinePlotWidget.h \
    AVNGUILibs/QwtPlotting/BandPowerQwtLinePlotWidget.h \
    AVNUtilLibs/Timestamp/Timestamp.h \
    AVNGUILibs/QwtPlotting/CursorCentredQwtPlotMagnifier.h \
    AVNGUILibs/QwtPlotting/AnimatedQwtPlotZoomer.h \
    AVNAppLibs/SocketStreamers/SocketReceiverBase.h \
    AVNAppLibs/SocketStreamers/UDPReceiver/UDPReceiver.h \
    NetworkConnectionWidget.h \
    AVNDataTypes/SpectrometerDataStream/SpectrometerDataStreamInterpreter.h \
    RoachAcquistionControlWidget.h \
    KATCPClient.h \
    AboutDialog.h \
    AVNGUILibs/QwtPlotting/WaterfallQwtPlotWidget.h \
    AVNGUILibs/QwtPlotting/WaterfallPlotSpectromgramData.h \
    AVNGUILibs/QwtPlotting/WallTimeQwtScaleDraw.h \
    AVNGUILibs/QwtPlotting/QwtPlotDistancePicker.h \
    AVNGUILibs/QwtPlotting/QwtPlotPositionPicker.h \
    AVNGUILibs/QwtPlotting/QwtPlotWidgetBase.h


FORMS    += MainWindow.ui \
    PlotsWidget.ui \
    NetworkConnectionWidget.ui \
    RoachAcquistionControlWidget.ui \
    AboutDialog.ui \
    AVNGUILibs/QwtPlotting/QwtPlotWidgetBase.ui

RESOURCES += \
    Images.qrc

unix{
    message(Detected Linux / Unix OS.)

    #Use Qt and Qwt from shared libs (For LGPL compliance)
    DEFINES    += QT_DLL QWT_DLL

    #Distro specific library includes (caters for varying library names and paths)
    #Currently this is Qwt linking and includes
    SYSTEM_VERSION = $$system( cat /proc/version)
    message( "System version string: $$SYSTEM_VERSION" )

    SYSTEM_GENTOO = $$system( cat /proc/version | grep -o Gentoo )
    contains( SYSTEM_GENTOO, Gentoo ) {
        message( "Detected Gentoo Distribution." )
        LIBS += -lqwt6
        INCLUDEPATH += /usr/include/qwt6
    }

    SYSTEM_UBUNTU = $$system( cat /proc/version | grep -o Ubuntu )
    contains( SYSTEM_UBUNTU, Ubuntu ) {
        message( "Detected Ubuntu Distribution." )
        LIBS += -lqwt
        INCLUDEPATH += /usr/include/qwt
    }

    SYSTEM_DEBIAN = $$system( cat /proc/version | grep -o Debian )
    contains( SYSTEM_DEBIAN, Debian ) {
        message( "Detected Debian Distribution." )
        LIBS += -lqwt
        INCLUDEPATH += /usr/include/qwt
    }

    #Boost seems to be ubiquitous across distros
    LIBS += -lboost_system -lboost_thread -lboost_chrono -lboost_date_time
}

win32{
    message(Detected Windows OS.)

    #Use Qt and Qwt from shared libs (For LGPL compliance)
    DEFINES    += QT_DLL QWT_DLL

    #Qwt
    LIBS += -LC://DevLibs//qwt-6.1.2//lib
    CONFIG(debug, debug|release) {
        LIBS += -lqwtd
    } else {
        LIBS += -lqwt
    }
    INCLUDEPATH +=  C://DevLibs//qwt-6.1.2//src

    #Boost
    CONFIG(debug, debug|release) {
        LIBS += -LC:\\DevLibs\\boost_1_59_0\\stage\\lib libboost_system-vc120-mt-gd-1_59.lib libboost_thread-vc120-mt-gd-1_59.lib libboost_chrono-vc120-mt-gd-1_59.lib libboost_date_time-vc120-mt-gd-1_59.lib
    } else {
        LIBS += -LC:\\DevLibs\\boost_1_59_0\\stage\\lib libboost_system-vc120-mt-1_59.lib libboost_thread-vc120-mt-1_59.lib libboost_chrono-vc120-mt-1_59.lib libboost_date_time-vc120-mt-1_59.lib
    }
    INCLUDEPATH += C:\\DevLibs\\boost_1_59_0

    #Winsock
    LIBS += Ws2_32.lib

    #Set application executable icon with this file:
    RC_FILE = AVNSignalAnalyser_win32.rc

}
