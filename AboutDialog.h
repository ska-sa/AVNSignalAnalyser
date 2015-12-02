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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include "ui_AboutDialog.h"

//This class displays an about dialog for the ARDView app.

class cAboutDialog : public QDialog
{
public:
    cAboutDialog(QWidget*);
    ~cAboutDialog();

private:
    Ui_AboutDialog* m_pADialog;
};

#endif // ABOUTDIALOG_H
