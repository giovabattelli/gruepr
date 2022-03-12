﻿/////////////////////////////////////////////////////////////////////////////////////////////////////////
// gruepr
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 - 2022
// Joshua Hertz
// info@gruepr.com
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see < https://www.gnu.org/licenses/ >.
//
//    This software incorporates code from the open source Qt libraries,
//    using version 5.15. These can be freely downloaded from
//    < http://qt.io/download >.
//
//    Icons were originally created by Icons8 < https://icons8.com >.
//    These icons have been made available under the creative commons license:
//    Attribution-NoDerivs 3.0 Unported (CC BY-ND 3.0).
//
//    An embedded font is used, mostly for pdf and printer output:
//    Oxygen Mono, Copyright (C) 2012, Vernon Adams (vern@newtypography.co.uk)
//    released under SIL OPEN FONT LICENSE V1.1.
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DONE:
//
// TO DO:
// - bugfix: crash on mac when selecting alternate matching name in select-name-dialogue
// - add which student listed the name that needs to be matched in select-name-dialogue
// - made the "Create Teams" button more emphasized/obvious
// - in gatherteammates dialog, enable the 'load from teamsTab' action
// - Sync with Canvas to load the teams into the groups part of Canvas (under the people tab)
// - more granular scheduling option, down to the 15 minute level at least
// - integrate with Google Drive: download survey results from within the application; expand to Canvas, Qualtrics, and other OAuth2 integration
//
// WAYS THAT MIGHT IMPROVE THE GENETIC ALGORITHM IN FUTURE:
// - use multiple genepools with limited cross-breeding
// - to get around the redundancy-of-genome issue, store each genome as unordered_set of unordered_set. Each team is set of IDs; each section is set of teams.
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gruepr.h"
#include "dialogs/registerDialog.h"
#include "surveymaker.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QMessageBox>
#include <QSettings>
#include <QSplashScreen>
#include <QThread>
#include <QToolButton>
#include <QtNetwork>

int main(int argc, char *argv[])
{
    // Set up application
    QApplication a(argc, argv);
    QApplication::setOrganizationName("gruepr");
    QApplication::setApplicationName("gruepr");
    QApplication::setApplicationVersion(GRUEPR_VERSION_NUMBER);
    QFontDatabase::addApplicationFont(":/fonts/OxygenMono-Regular.otf");

    // Show splash screen
    auto *splash = new QSplashScreen;
    QPixmap pic(":/icons/fish.png");
    splash->setPixmap(pic);
    splash->showMessage("version " GRUEPR_VERSION_NUMBER "\nCopyright © " GRUEPR_COPYRIGHT_YEAR "\nJoshua Hertz\ninfo@gruepr.com", Qt::AlignCenter, Qt::white);
    splash->show();

    // check the latest version available for download and compare to this current version
    auto *manager = new QNetworkAccessManager(splash);
    QNetworkRequest request((QUrl(VERSION_CHECK_URL)));
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
    QNetworkReply *networkReply = manager->get(request);
    QEventLoop loop;
    QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    QByteArray latestVersion = (networkReply->bytesAvailable() != 0) ? networkReply->readAll() : "0.0";
    delete manager;
    bool upgradeAvailable = (latestVersion.toFloat() > QString(GRUEPR_VERSION_NUMBER).toFloat());

    // Create application choice (gruepr or SurveyMaker) window
    auto *startWindow = new QMessageBox;
    auto *boxFont = new QFont("Oxygen Mono", QApplication::font().pointSize()+8);
    startWindow->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    startWindow->setWindowTitle("gruepr");
    startWindow->setFont(*boxFont); startWindow->setText(QObject::tr("Select an app to run:"));

    // Button metrics
    boxFont->setPointSize(QApplication::font().pointSize()+4);
    int labelWidth = (QFontMetrics(*boxFont)).boundingRect(QObject::tr("Register gruepr")).width();
    QSize defIconSize(labelWidth-20,labelWidth-20);
    QSize defButtonSize(labelWidth+20,labelWidth);

    // Create and add buttons
    auto *leaveButton = new QToolButton(startWindow);
    leaveButton->setIconSize(defIconSize); leaveButton->setFont(*boxFont); leaveButton->setFixedSize(defButtonSize);
    leaveButton->setIcon(QIcon(":/icons/exit.png")); leaveButton->setText(QObject::tr("Exit"));
    leaveButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto *grueprButton = new QToolButton(startWindow);
    grueprButton->setIconSize(defIconSize); grueprButton->setFont(*boxFont); grueprButton->setFixedSize(defButtonSize);
    grueprButton->setIcon(QIcon(":/icons/gruepr.png")); grueprButton->setText("gruepr");
    grueprButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    auto *survMakeButton = new QToolButton(startWindow);
    survMakeButton->setIconSize(defIconSize); survMakeButton->setFont(*boxFont); survMakeButton->setFixedSize(defButtonSize);
    survMakeButton->setIcon(QIcon(":/icons/surveymaker.png")); survMakeButton->setText("SurveyMaker");
    survMakeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    //Will add upgrade button only if latest downloadable version is newer.
    QToolButton *upgradeButton = nullptr;
    //Will add registration button only if unregistered
    QToolButton *registerUserButton = nullptr;
    QSettings savedSettings;
    QString registeredUser = savedSettings.value("registeredUser", "").toString();
    QString UserID = savedSettings.value("registeredUserID", "").toString();
    bool registered = (!registeredUser.isEmpty() && (UserID == QString(QCryptographicHash::hash((registeredUser.toUtf8()), QCryptographicHash::Md5).toHex())));

    //order switches for some reason mac->windows
#ifdef Q_OS_MACOS
    if(upgradeAvailable)
    {
        upgradeButton = new QToolButton(startWindow);
        upgradeButton->setIconSize(defIconSize); upgradeButton->setFont(*boxFont); upgradeButton->setFixedSize(defButtonSize);
        upgradeButton->setIcon(QIcon(":/icons/website.png")); upgradeButton->setText(QObject::tr("New version!"));
        upgradeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        upgradeButton->setStyleSheet("background-color: #83f2a5");
        upgradeButton->setToolTip("<html>" + QObject::tr("There is a newer version of gruepr available. You have version ") +
                                  GRUEPR_VERSION_NUMBER + QObject::tr(" and you can now download version ") + latestVersion + ".</html>");
        startWindow->addButton(upgradeButton, QMessageBox::YesRole);
    }
    if(!registered)
    {
        registerUserButton = new QToolButton(startWindow);
        registerUserButton->setIconSize(defIconSize); registerUserButton->setFont(*boxFont); registerUserButton->setFixedSize(defButtonSize);
        registerUserButton->setIcon(QIcon(":/icons/license.png")); registerUserButton->setText("Register gruepr");
        registerUserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        registerUserButton->setStyleSheet("background-color: #f283a5");
        registerUserButton->setToolTip("<html>" + QObject::tr("Please register your copy of gruepr. Doing so helps me support the community of educators that use it.") + "</html>");
        startWindow->addButton(registerUserButton, QMessageBox::YesRole);
    }
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
#endif
#ifdef Q_OS_WIN32
    startWindow->addButton(survMakeButton, QMessageBox::YesRole);
    startWindow->addButton(grueprButton, QMessageBox::YesRole);
    startWindow->addButton(leaveButton, QMessageBox::YesRole);
    if(!registered)
    {
        registerUserButton = new QToolButton(startWindow);
        registerUserButton->setIconSize(defIconSize); registerUserButton->setFont(*boxFont); registerUserButton->setFixedSize(defButtonSize);
        registerUserButton->setIcon(QIcon(":/icons/license.png")); registerUserButton->setText(QObject::tr("Register gruepr"));
        registerUserButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        registerUserButton->setStyleSheet("background-color: #f283a5");
        registerUserButton->setToolTip("<html>" + QObject::tr("Please register your copy of gruepr. Doing so helps me support the community of educators that use it.") + "</html>");
        startWindow->addButton(registerUserButton, QMessageBox::YesRole);
    }
    if(upgradeAvailable)
    {
        upgradeButton = new QToolButton(startWindow);
        upgradeButton->setIconSize(defIconSize); upgradeButton->setFont(*boxFont); upgradeButton->setFixedSize(defButtonSize);
        upgradeButton->setIcon(QIcon(":/icons/website.png")); upgradeButton->setText(QObject::tr("New version!"));
        upgradeButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        upgradeButton->setStyleSheet("background-color: #83f2a5");
        upgradeButton->setToolTip("<html>" + QObject::tr("There is a newer version of gruepr available. You have version ") +
                                  GRUEPR_VERSION_NUMBER + QObject::tr(" and you can now download version ") + latestVersion + ".</html>");
        startWindow->addButton(upgradeButton, QMessageBox::YesRole);
    }
#endif
    startWindow->setEscapeButton(leaveButton);

    // Show application choice window and delete splash
    splash->finish(startWindow);
    startWindow->exec();
    QAbstractButton *result = startWindow->clickedButton();
    delete splash;
    delete boxFont;

    // Run chosen application
    int executionResult = 0;
    while(result != leaveButton)
    {
        if(result == grueprButton)
        {
            gruepr w;
            w.setWindowTitle("gruepr [*]");         // asterisk is placeholder, shown when there is unsaved work
            w.show();
            executionResult = QApplication::exec();
            result = leaveButton;
        }
        else if(result == survMakeButton)
        {
            SurveyMaker w;
            w.setWindowTitle("gruepr: SurveyMaker [*]");         // asterisk is placeholder, shown when there is unsaved work
            w.show();
            executionResult = QApplication::exec();
            result = leaveButton;
        }
        else if(result == registerUserButton)
        {
            //make sure we can connect to google
            auto *manager = new QNetworkAccessManager(startWindow);
            QEventLoop loop;
            QNetworkReply *networkReply = manager->get(QNetworkRequest(QUrl("http://www.google.com")));
            QObject::connect(networkReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if(networkReply->bytesAvailable() == 0)
            {
                //no internet right now
                QMessageBox::critical(startWindow, QObject::tr("No Internet Connection"), QObject::tr("There does not seem to be an internet connection.\n"
                                                                                                      "Please register at another time."));
                delete manager;
            }
            else
            {
                //we can connect, so gather name, institution, and email address for submission
                auto *window = new registerDialog(startWindow);
                int reply = window->exec();
                //If user clicks OK, email registration info and add to saved settings
                if(reply == QDialog::Accepted)
                {
                    // using DesktopServices (i.e., user's browser) because access to Google Script is via https, and ssl is tough in Qt
                    if(QDesktopServices::openUrl(QUrl(QString(USER_REGISTRATION_URL)+
                                                      "?name="+QUrl::toPercentEncoding(window->name->text())+
                                                      "&institution="+QUrl::toPercentEncoding(window->institution->text())+
                                                      "&email="+QUrl::toPercentEncoding(window->email->text()))))
                    {
                        registeredUser = window->name->text();
                        QSettings savedSettings;
                        savedSettings.setValue("registeredUser", registeredUser);
                        savedSettings.setValue("registeredUserID",QString(QCryptographicHash::hash((registeredUser.toUtf8()),QCryptographicHash::Md5).toHex()));
                        startWindow->removeButton(registerUserButton);
                    }
                    else
                    {
                        QMessageBox::critical(startWindow, QObject::tr("No Internet Connection"),
                                              QObject::tr("There seems to be a problem with submitting your registration.\n"
                                                          "Please try again at another time or contact <info@gruepr.com>."));
                    }
                }
                delete manager;
                delete window;
            }
            startWindow->exec();
            result = startWindow->clickedButton();
        }
        else if(result == upgradeButton)
        {
            QDesktopServices::openUrl(QUrl("http://gruepr.com"));
            startWindow->exec();
            result = startWindow->clickedButton();
        }
    }

    delete startWindow;
    return executionResult;
}
