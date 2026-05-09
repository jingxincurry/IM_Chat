/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QTextBrowser *textBrowser_chat;
    QTextEdit *textEdit_input;
    QPushButton *btn_sendMsg;
    QLabel *label;
    QPushButton *btn_connectServer;
    QLineEdit *lineEdit_username;
    QLineEdit *lineEdit_password;
    QPushButton *btn_login;
    QPushButton *btn_register;
    QListWidget *listWidget_users;
    QListWidget *listWidget_files;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *btn_sendFile;
    QPushButton *btn_downloadFile;
    QLabel *label_4;
    QPushButton *btn_close;
    QTextBrowser *systemInfoBrowser;
    QLabel *label_5;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(925, 665);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        textBrowser_chat = new QTextBrowser(centralwidget);
        textBrowser_chat->setObjectName("textBrowser_chat");
        textBrowser_chat->setGeometry(QRect(10, 20, 391, 421));
        textEdit_input = new QTextEdit(centralwidget);
        textEdit_input->setObjectName("textEdit_input");
        textEdit_input->setGeometry(QRect(10, 460, 391, 71));
        btn_sendMsg = new QPushButton(centralwidget);
        btn_sendMsg->setObjectName("btn_sendMsg");
        btn_sendMsg->setGeometry(QRect(310, 540, 93, 28));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(410, 190, 69, 19));
        btn_connectServer = new QPushButton(centralwidget);
        btn_connectServer->setObjectName("btn_connectServer");
        btn_connectServer->setGeometry(QRect(570, 20, 93, 28));
        lineEdit_username = new QLineEdit(centralwidget);
        lineEdit_username->setObjectName("lineEdit_username");
        lineEdit_username->setGeometry(QRect(512, 60, 141, 25));
        lineEdit_password = new QLineEdit(centralwidget);
        lineEdit_password->setObjectName("lineEdit_password");
        lineEdit_password->setGeometry(QRect(512, 100, 141, 21));
        btn_login = new QPushButton(centralwidget);
        btn_login->setObjectName("btn_login");
        btn_login->setGeometry(QRect(430, 130, 93, 28));
        btn_register = new QPushButton(centralwidget);
        btn_register->setObjectName("btn_register");
        btn_register->setGeometry(QRect(560, 130, 93, 28));
        listWidget_users = new QListWidget(centralwidget);
        listWidget_users->setObjectName("listWidget_users");
        listWidget_users->setGeometry(QRect(410, 210, 271, 321));
        listWidget_files = new QListWidget(centralwidget);
        listWidget_files->setObjectName("listWidget_files");
        listWidget_files->setGeometry(QRect(700, 40, 211, 491));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(430, 60, 69, 19));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(430, 100, 69, 19));
        btn_sendFile = new QPushButton(centralwidget);
        btn_sendFile->setObjectName("btn_sendFile");
        btn_sendFile->setGeometry(QRect(700, 540, 93, 28));
        btn_downloadFile = new QPushButton(centralwidget);
        btn_downloadFile->setObjectName("btn_downloadFile");
        btn_downloadFile->setGeometry(QRect(810, 540, 93, 28));
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(700, 20, 69, 19));
        btn_close = new QPushButton(centralwidget);
        btn_close->setObjectName("btn_close");
        btn_close->setGeometry(QRect(210, 540, 93, 28));
        systemInfoBrowser = new QTextBrowser(centralwidget);
        systemInfoBrowser->setObjectName("systemInfoBrowser");
        systemInfoBrowser->setGeometry(QRect(20, 571, 881, 61));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(20, 550, 81, 21));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 925, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        btn_sendMsg->setText(QCoreApplication::translate("MainWindow", "\345\217\221\351\200\201", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\345\234\250\347\272\277\347\224\250\346\210\267", nullptr));
        btn_connectServer->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\346\234\215\345\212\241\345\231\250", nullptr));
        btn_login->setText(QCoreApplication::translate("MainWindow", "\347\231\273\345\275\225", nullptr));
        btn_register->setText(QCoreApplication::translate("MainWindow", "\346\263\250\345\206\214", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\347\224\250\346\210\267\345\220\215\357\274\232", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "\345\257\206\347\240\201\357\274\232", nullptr));
        btn_sendFile->setText(QCoreApplication::translate("MainWindow", "\344\270\212\344\274\240\346\226\207\344\273\266", nullptr));
        btn_downloadFile->setText(QCoreApplication::translate("MainWindow", "\344\270\213\350\275\275 ", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266\344\274\240\350\276\223", nullptr));
        btn_close->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "\347\263\273\347\273\237\346\217\220\347\244\272\346\240\217", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
