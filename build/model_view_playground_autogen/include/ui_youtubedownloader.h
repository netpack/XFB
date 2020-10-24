/********************************************************************************
** Form generated from reading UI file 'youtubedownloader.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_YOUTUBEDOWNLOADER_H
#define UI_YOUTUBEDOWNLOADER_H

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

class Ui_youtubedownloader
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

    void setupUi(QWidget *youtubedownloader)
    {
        if (youtubedownloader->objectName().isEmpty())
            youtubedownloader->setObjectName(QStringLiteral("youtubedownloader"));
        youtubedownloader->resize(696, 548);
        QIcon icon;
        icon.addFile(QStringLiteral(":/48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        youtubedownloader->setWindowIcon(icon);
        youtubedownloader->setStyleSheet(QStringLiteral("background-color:#FFF;"));
        label = new QLabel(youtubedownloader);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(120, 10, 181, 41));
        QFont font;
        font.setFamily(QStringLiteral("URW Chancery L"));
        font.setPointSize(17);
        font.setItalic(true);
        label->setFont(font);
        txt_teminal_yd1 = new QPlainTextEdit(youtubedownloader);
        txt_teminal_yd1->setObjectName(QStringLiteral("txt_teminal_yd1"));
        txt_teminal_yd1->setGeometry(QRect(10, 320, 671, 161));
        QFont font1;
        font1.setFamily(QStringLiteral("Ubuntu"));
        font1.setPointSize(8);
        txt_teminal_yd1->setFont(font1);
        txt_teminal_yd1->setStyleSheet(QLatin1String("color:rgb(255, 255, 255);\n"
"background-color: rgb(83, 28, 28);"));
        txt_teminal_yd1->setReadOnly(true);
        txt_teminal_yd1->setTabStopWidth(80);
        txt_teminal_yd1->setCursorWidth(2);
        frame_loading = new QFrame(youtubedownloader);
        frame_loading->setObjectName(QStringLiteral("frame_loading"));
        frame_loading->setEnabled(true);
        frame_loading->setGeometry(QRect(10, 320, 671, 161));
        frame_loading->setStyleSheet(QStringLiteral("background-color: rgb(92, 80, 80);"));
        frame_loading->setFrameShape(QFrame::StyledPanel);
        frame_loading->setFrameShadow(QFrame::Raised);
        label_8 = new QLabel(frame_loading);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(10, 50, 651, 51));
        QFont font2;
        font2.setFamily(QStringLiteral("URW Palladio L"));
        font2.setPointSize(8);
        font2.setItalic(true);
        label_8->setFont(font2);
        label_8->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_8->setAlignment(Qt::AlignCenter);
        label_9 = new QLabel(frame_loading);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(20, 20, 641, 41));
        QFont font3;
        font3.setFamily(QStringLiteral("URW Palladio L"));
        font3.setPointSize(18);
        font3.setItalic(true);
        label_9->setFont(font3);
        label_9->setStyleSheet(QStringLiteral("color:#FFF"));
        label_9->setAlignment(Qt::AlignCenter);
        label_10 = new QLabel(frame_loading);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(10, 130, 651, 20));
        label_10->setFont(font2);
        label_10->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_10->setScaledContents(false);
        label_10->setAlignment(Qt::AlignCenter);
        label_5 = new QLabel(youtubedownloader);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(40, 170, 71, 17));
        cbox_g2 = new QComboBox(youtubedownloader);
        cbox_g2->setObjectName(QStringLiteral("cbox_g2"));
        cbox_g2->setEnabled(true);
        cbox_g2->setGeometry(QRect(120, 190, 201, 27));
        cbox_g2->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));
        cbox_g1 = new QComboBox(youtubedownloader);
        cbox_g1->setObjectName(QStringLiteral("cbox_g1"));
        cbox_g1->setEnabled(true);
        cbox_g1->setGeometry(QRect(120, 160, 201, 27));
        cbox_g1->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));
        txt_song = new QLineEdit(youtubedownloader);
        txt_song->setObjectName(QStringLiteral("txt_song"));
        txt_song->setEnabled(true);
        txt_song->setGeometry(QRect(120, 130, 461, 27));
        txt_artist = new QLineEdit(youtubedownloader);
        txt_artist->setObjectName(QStringLiteral("txt_artist"));
        txt_artist->setEnabled(true);
        txt_artist->setGeometry(QRect(120, 100, 461, 27));
        checkBox_cplp = new QCheckBox(youtubedownloader);
        checkBox_cplp->setObjectName(QStringLiteral("checkBox_cplp"));
        checkBox_cplp->setEnabled(true);
        checkBox_cplp->setGeometry(QRect(40, 260, 281, 22));
        label_4 = new QLabel(youtubedownloader);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(40, 140, 62, 17));
        label_3 = new QLabel(youtubedownloader);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(40, 110, 71, 17));
        label_6 = new QLabel(youtubedownloader);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(40, 200, 71, 17));
        label_7 = new QLabel(youtubedownloader);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(40, 230, 111, 17));
        dateEdit_publishedDate = new QDateEdit(youtubedownloader);
        dateEdit_publishedDate->setObjectName(QStringLiteral("dateEdit_publishedDate"));
        dateEdit_publishedDate->setEnabled(true);
        dateEdit_publishedDate->setGeometry(QRect(150, 220, 171, 27));
        label_2 = new QLabel(youtubedownloader);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(40, 80, 81, 17));
        txt_videoLink = new QLineEdit(youtubedownloader);
        txt_videoLink->setObjectName(QStringLiteral("txt_videoLink"));
        txt_videoLink->setGeometry(QRect(120, 70, 521, 27));
        QFont font4;
        font4.setPointSize(8);
        txt_videoLink->setFont(font4);
        pushButton = new QPushButton(youtubedownloader);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(350, 170, 171, 31));
        label_11 = new QLabel(youtubedownloader);
        label_11->setObjectName(QStringLiteral("label_11"));
        label_11->setGeometry(QRect(40, 10, 61, 51));
        label_11->setPixmap(QPixmap(QString::fromUtf8(":/icons/icon48x48.png")));
        label_11->setScaledContents(true);
        layoutWidget = new QWidget(youtubedownloader);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 490, 671, 51));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        bt_youtube_getIt = new QPushButton(layoutWidget);
        bt_youtube_getIt->setObjectName(QStringLiteral("bt_youtube_getIt"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/icons/svn-update.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_youtube_getIt->setIcon(icon1);
        bt_youtube_getIt->setIconSize(QSize(24, 24));

        horizontalLayout->addWidget(bt_youtube_getIt);

        bt_clear = new QPushButton(layoutWidget);
        bt_clear->setObjectName(QStringLiteral("bt_clear"));

        horizontalLayout->addWidget(bt_clear);

        bt_close = new QPushButton(layoutWidget);
        bt_close->setObjectName(QStringLiteral("bt_close"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/icons/application-exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        bt_close->setIcon(icon2);

        horizontalLayout->addWidget(bt_close);


        retranslateUi(youtubedownloader);

        QMetaObject::connectSlotsByName(youtubedownloader);
    } // setupUi

    void retranslateUi(QWidget *youtubedownloader)
    {
        youtubedownloader->setWindowTitle(QApplication::translate("youtubedownloader", "Youtube Downloader", nullptr));
        label->setText(QApplication::translate("youtubedownloader", "Youtube Downloader", nullptr));
        label_8->setText(QApplication::translate("youtubedownloader", "Moving mountains for you... please wait while our little hamsters are doing the work ;-)", nullptr));
        label_9->setText(QApplication::translate("youtubedownloader", "Dowloading and Converting Video...", nullptr));
        label_10->setText(QApplication::translate("youtubedownloader", "Even with a lot of electrons one must persist on thinking positive!", nullptr));
        label_5->setText(QApplication::translate("youtubedownloader", "Genre 1:", nullptr));
        txt_artist->setPlaceholderText(QString());
#ifndef QT_NO_TOOLTIP
        checkBox_cplp->setToolTip(QApplication::translate("youtubedownloader", "Check this to indicate that this is a Portuguese speaking track", nullptr));
#endif // QT_NO_TOOLTIP
        checkBox_cplp->setText(QApplication::translate("youtubedownloader", "CPLP", nullptr));
        label_4->setText(QApplication::translate("youtubedownloader", "Song:", nullptr));
        label_3->setText(QApplication::translate("youtubedownloader", "Artist(s):", nullptr));
        label_6->setText(QApplication::translate("youtubedownloader", "Genre 2:", nullptr));
        label_7->setText(QApplication::translate("youtubedownloader", "Published date:", nullptr));
        dateEdit_publishedDate->setDisplayFormat(QApplication::translate("youtubedownloader", "yyyy/MM/dd", nullptr));
        label_2->setText(QApplication::translate("youtubedownloader", "Video Link:", nullptr));
        txt_videoLink->setPlaceholderText(QApplication::translate("youtubedownloader", "https://www.youtube.com/watch?v=qWG2dsXV5HI", nullptr));
        pushButton->setText(QApplication::translate("youtubedownloader", "Manage Genres", nullptr));
        label_11->setText(QString());
        bt_youtube_getIt->setText(QApplication::translate("youtubedownloader", "Get it!", nullptr));
        bt_clear->setText(QApplication::translate("youtubedownloader", "Clear", nullptr));
        bt_close->setText(QApplication::translate("youtubedownloader", "Close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class youtubedownloader: public Ui_youtubedownloader {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_YOUTUBEDOWNLOADER_H
