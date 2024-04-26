/********************************************************************************
** Form generated from reading UI file 'externaldownloader.ui'
**
** Created by: Qt User Interface Compiler version 5.15.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXTERNALDOWNLOADER_H
#define UI_EXTERNALDOWNLOADER_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_externaldownloader
{
public:
    QLabel *label;
    QPlainTextEdit *txt_teminal_yd1;
    QFrame *frame_loading;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label_5;
    QComboBox *cbox_g2;
    QComboBox *cbox_g1;
    QLineEdit *txt_song;
    QLineEdit *txt_artist;
    QCheckBox *checkBox_cplp;
    QLabel *label_4;
    QLabel *label_3;
    QLabel *label_6;
    QLabel *label_7;
    QDateEdit *dateEdit_publishedDate;
    QLabel *label_2;
    QLineEdit *txt_videoLink;
    QPushButton *pushButton;
    QLabel *label_11;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QPushButton *bt_youtube_getIt;
    QPushButton *bt_clear;
    QPushButton *bt_close;

    void setupUi(QWidget *externaldownloader)
    {
        if (externaldownloader->objectName().isEmpty())
            externaldownloader->setObjectName(QString::fromUtf8("externaldownloader"));
        externaldownloader->resize(696, 548);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        externaldownloader->setWindowIcon(icon);
        externaldownloader->setStyleSheet(QString::fromUtf8("background-color:#FFF;"));
        label = new QLabel(externaldownloader);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(120, 10, 191, 41));
        QFont font;
        font.setFamily(QString::fromUtf8("URW Chancery L"));
        font.setPointSize(17);
        font.setItalic(true);
        label->setFont(font);
        txt_teminal_yd1 = new QPlainTextEdit(externaldownloader);
        txt_teminal_yd1->setObjectName(QString::fromUtf8("txt_teminal_yd1"));
        txt_teminal_yd1->setGeometry(QRect(10, 320, 671, 161));
        QFont font1;
        font1.setFamily(QString::fromUtf8("Ubuntu"));
        font1.setPointSize(8);
        txt_teminal_yd1->setFont(font1);
        txt_teminal_yd1->setStyleSheet(QString::fromUtf8("color:rgb(255, 255, 255);\n"
"background-color: rgb(83, 28, 28);"));
        txt_teminal_yd1->setReadOnly(true);
        txt_teminal_yd1->setCursorWidth(2);
        txt_teminal_yd1->setProperty("tabStopWidth", QVariant(80));
        frame_loading = new QFrame(externaldownloader);
        frame_loading->setObjectName(QString::fromUtf8("frame_loading"));
        frame_loading->setEnabled(true);
        frame_loading->setGeometry(QRect(10, 320, 671, 161));
        frame_loading->setStyleSheet(QString::fromUtf8("background-color: rgb(92, 80, 80);"));
        frame_loading->setFrameShape(QFrame::StyledPanel);
        frame_loading->setFrameShadow(QFrame::Raised);
        label_8 = new QLabel(frame_loading);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(10, 50, 651, 51));
        QFont font2;
        font2.setFamily(QString::fromUtf8("URW Palladio L"));
        font2.setPointSize(8);
        font2.setItalic(true);
        label_8->setFont(font2);
        label_8->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));
        label_8->setAlignment(Qt::AlignCenter);
        label_9 = new QLabel(frame_loading);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(20, 20, 641, 41));
        QFont font3;
        font3.setFamily(QString::fromUtf8("URW Palladio L"));
        font3.setPointSize(18);
        font3.setItalic(true);
        label_9->setFont(font3);
        label_9->setStyleSheet(QString::fromUtf8("color:#FFF"));
        label_9->setAlignment(Qt::AlignCenter);
        label_10 = new QLabel(frame_loading);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(10, 130, 651, 20));
        label_10->setFont(font2);
        label_10->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);"));
        label_10->setScaledContents(false);
        label_10->setAlignment(Qt::AlignCenter);
        label_5 = new QLabel(externaldownloader);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(40, 170, 71, 17));
        cbox_g2 = new QComboBox(externaldownloader);
        cbox_g2->setObjectName(QString::fromUtf8("cbox_g2"));
        cbox_g2->setEnabled(true);
        cbox_g2->setGeometry(QRect(120, 190, 201, 27));
        cbox_g2->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        cbox_g1 = new QComboBox(externaldownloader);
        cbox_g1->setObjectName(QString::fromUtf8("cbox_g1"));
        cbox_g1->setEnabled(true);
        cbox_g1->setGeometry(QRect(120, 160, 201, 27));
        cbox_g1->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        txt_song = new QLineEdit(externaldownloader);
        txt_song->setObjectName(QString::fromUtf8("txt_song"));
        txt_song->setEnabled(true);
        txt_song->setGeometry(QRect(120, 130, 461, 27));
        txt_artist = new QLineEdit(externaldownloader);
        txt_artist->setObjectName(QString::fromUtf8("txt_artist"));
        txt_artist->setEnabled(true);
        txt_artist->setGeometry(QRect(120, 100, 461, 27));
        checkBox_cplp = new QCheckBox(externaldownloader);
        checkBox_cplp->setObjectName(QString::fromUtf8("checkBox_cplp"));
        checkBox_cplp->setEnabled(true);
        checkBox_cplp->setGeometry(QRect(40, 260, 281, 22));
        label_4 = new QLabel(externaldownloader);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(40, 140, 62, 17));
        label_3 = new QLabel(externaldownloader);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(40, 110, 71, 17));
        label_6 = new QLabel(externaldownloader);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(40, 200, 71, 17));
        label_7 = new QLabel(externaldownloader);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(40, 230, 111, 17));
        dateEdit_publishedDate = new QDateEdit(externaldownloader);
        dateEdit_publishedDate->setObjectName(QString::fromUtf8("dateEdit_publishedDate"));
        dateEdit_publishedDate->setEnabled(true);
        dateEdit_publishedDate->setGeometry(QRect(150, 220, 171, 27));
        label_2 = new QLabel(externaldownloader);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(40, 80, 81, 17));
        txt_videoLink = new QLineEdit(externaldownloader);
        txt_videoLink->setObjectName(QString::fromUtf8("txt_videoLink"));
        txt_videoLink->setGeometry(QRect(120, 70, 521, 27));
        QFont font4;
        font4.setPointSize(8);
        txt_videoLink->setFont(font4);
        pushButton = new QPushButton(externaldownloader);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));
        pushButton->setGeometry(QRect(350, 170, 171, 31));
        label_11 = new QLabel(externaldownloader);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(40, 10, 61, 51));
        label_11->setPixmap(QPixmap(QString::fromUtf8(":/icons/icon48x48.png")));
        label_11->setScaledContents(true);
        layoutWidget = new QWidget(externaldownloader);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 490, 671, 51));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        bt_youtube_getIt = new QPushButton(layoutWidget);
        bt_youtube_getIt->setObjectName(QString::fromUtf8("bt_youtube_getIt"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/svn-update.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_youtube_getIt->setIcon(icon1);
        bt_youtube_getIt->setIconSize(QSize(24, 24));

        horizontalLayout->addWidget(bt_youtube_getIt);

        bt_clear = new QPushButton(layoutWidget);
        bt_clear->setObjectName(QString::fromUtf8("bt_clear"));

        horizontalLayout->addWidget(bt_clear);

        bt_close = new QPushButton(layoutWidget);
        bt_close->setObjectName(QString::fromUtf8("bt_close"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/application-exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_close->setIcon(icon2);

        horizontalLayout->addWidget(bt_close);


        retranslateUi(externaldownloader);

        QMetaObject::connectSlotsByName(externaldownloader);
    } // setupUi

    void retranslateUi(QWidget *externaldownloader)
    {
        externaldownloader->setWindowTitle(QCoreApplication::translate("externaldownloader", "External Downloader", nullptr));
        label->setText(QCoreApplication::translate("externaldownloader", "External Downloader", nullptr));
        label_8->setText(QCoreApplication::translate("externaldownloader", "Moving mountains for you... please wait while our little hamsters are doing the work ;-)", nullptr));
        label_9->setText(QCoreApplication::translate("externaldownloader", "Dowloading and Converting Video...", nullptr));
        label_10->setText(QCoreApplication::translate("externaldownloader", "Even with a lot of electrons one must persist on thinking positive!", nullptr));
        label_5->setText(QCoreApplication::translate("externaldownloader", "Genre 1:", nullptr));
        txt_artist->setPlaceholderText(QString());
#if QT_CONFIG(tooltip)
        checkBox_cplp->setToolTip(QCoreApplication::translate("externaldownloader", "Check this to indicate that this is a Portuguese speaking track", nullptr));
#endif // QT_CONFIG(tooltip)
        checkBox_cplp->setText(QCoreApplication::translate("externaldownloader", "CPLP", nullptr));
        label_4->setText(QCoreApplication::translate("externaldownloader", "Song:", nullptr));
        label_3->setText(QCoreApplication::translate("externaldownloader", "Artist(s):", nullptr));
        label_6->setText(QCoreApplication::translate("externaldownloader", "Genre 2:", nullptr));
        label_7->setText(QCoreApplication::translate("externaldownloader", "Published date:", nullptr));
        dateEdit_publishedDate->setDisplayFormat(QCoreApplication::translate("externaldownloader", "yyyy/MM/dd", nullptr));
        label_2->setText(QCoreApplication::translate("externaldownloader", "Video Link:", nullptr));
        txt_videoLink->setPlaceholderText(QCoreApplication::translate("externaldownloader", "https://www.youtube.com/watch?v=qWG2dsXV5HI", nullptr));
        pushButton->setText(QCoreApplication::translate("externaldownloader", "Manage Genres", nullptr));
        label_11->setText(QString());
        bt_youtube_getIt->setText(QCoreApplication::translate("externaldownloader", "Get it!", nullptr));
        bt_clear->setText(QCoreApplication::translate("externaldownloader", "Clear", nullptr));
        bt_close->setText(QCoreApplication::translate("externaldownloader", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class externaldownloader: public Ui_externaldownloader {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXTERNALDOWNLOADER_H
