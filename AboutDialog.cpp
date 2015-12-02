//********************************************
//## Commensal Radar Project ##
//
//Filename:
//  AboutDialog.h
//
//Description:
//  This class displays an about dialog for the ARDView app.
//
//Author:
//	Craig Tong
//	Radar Remote Sensing Group
//	Department of Electrical Engineering
//	University of Cape Town
//	craig.tong@uct.ac.za
//  Copyright © 2013 Craig Tong
//********************************************

#include "AboutDialog.h"
#include <QDateTime>

cAboutDialog::cAboutDialog(QWidget *parent) : QDialog(parent, Qt::Dialog)
{
    m_pADialog = new Ui_AboutDialog();
    m_pADialog->setupUi(this);
    m_pADialog->label_version->setText(QString("Version: %1").arg(QCoreApplication::applicationVersion()));


#ifdef _WIN32
    #ifdef _WIN64
        m_pADialog->label_platform->setText(QString("Platform: MSVC Win64"));
    #else
        m_pADialog->label_platform->setText(QString("Platform: MSVC Win32"));
    #endif
#else
    #ifdef __unix__
        #ifdef __x86_64__
            m_pADialog->label_platform->setText(QString("Platform: GCC x86_64"));
        #else
            m_pADialog->label_platform->setText(QString("Platform: GCC x86"));
        #endif
    #else
        #ifdef __MAC__
            m_pADialog->label_platform->setText(QString("Platform: Mac"));
        #else
            m_pADialog->label_platform->setText(QString("Platform: Unknown"));
        #endif
    #endif
#endif

    m_pADialog->label_buildDate->setText(QString("Build date: %1 %2").arg(__DATE__).arg(__TIME__));
    m_pADialog->label_year->setText(QString("Copyright %1 2015 SKA SA. All rights reserved").arg(QString::fromUtf8("©")));
}

cAboutDialog::~cAboutDialog()
{
    delete m_pADialog;
}
