/********************************************************************************
** Form generated from reading UI file 'add_music_single.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADD_MUSIC_SINGLE_H
#define UI_ADD_MUSIC_SINGLE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_add_music_single
{
public:
    QLabel *label;
    QToolButton *toolButton;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QWidget *layoutWidget_music1;
    QVBoxLayout *verticalLayout;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_8;
    QWidget *layoutWidgetmusic2;
    QVBoxLayout *verticalLayout_2;
    QLineEdit *txt_file;
    QLineEdit *txt_artist;
    QLineEdit *txt_song;
    QComboBox *cbox_g1;
    QComboBox *cbox_g2;
    QComboBox *cbox_country;
    QLabel *label_9;
    QDateEdit *f_date;
    QPushButton *bt_manageGenres;

    void setupUi(QDialog *add_music_single)
    {
        if (add_music_single->objectName().isEmpty())
            add_music_single->setObjectName("add_music_single");
        add_music_single->resize(678, 396);
        add_music_single->setStyleSheet(QString::fromUtf8("\n"
"    QDialog {\n"
"        background-color: #2b2b2b;\n"
"        color: #bbbbbb; /* Default text color */\n"
"    }\n"
"    QLabel {\n"
"        color: #bbbbbb;\n"
"    }\n"
"    QLineEdit, QTextEdit, QPlainTextEdit {\n"
"        background-color: #3c3f41;\n"
"        color: #bbbbbb;\n"
"        border: 1px solid #555555;\n"
"        selection-background-color: #007acc;\n"
"        selection-color: white;\n"
"    }\n"
"    QPushButton, QToolButton {\n"
"        background-color: #555555;\n"
"        color: #bbbbbb;\n"
"        border: 1px solid #666666;\n"
"        padding: 5px;\n"
"    }\n"
"    QPushButton:hover, QToolButton:hover {\n"
"        background-color: #666666;\n"
"    }\n"
"    QPushButton:pressed, QToolButton:pressed {\n"
"        background-color: #444444;\n"
"    }\n"
"    QComboBox {\n"
"        background-color: #3c3f41;\n"
"        color: #bbbbbb;\n"
"        border: 1px solid #555555;\n"
"        selection-background-color: #007acc;\n"
"        /* selection-color: white; */\n"
"    }\n"
""
                        "    QComboBox::drop-down {\n"
"        border: none;\n"
"    }\n"
"    QComboBox QAbstractItemView {\n"
"        background-color: #3c3f41;\n"
"        color: #bbbbbb;\n"
"        selection-background-color: #007acc;\n"
"        selection-color: white;\n"
"        border: 1px solid #555555;\n"
"        outline: 0px;\n"
"    }\n"
"    QDateEdit {\n"
"        background-color: #3c3f41;\n"
"        color: #bbbbbb;\n"
"        border: 1px solid #555555;\n"
"        selection-background-color: #007acc;\n"
"        selection-color: white;\n"
"    }\n"
"    QDateEdit::drop-down {\n"
"         /* Style dropdown button if needed */\n"
"    }\n"
"    QDateEdit QAbstractItemView {\n"
"        background-color: #3c3f41;\n"
"        color: #bbbbbb;\n"
"        selection-background-color: #007acc;\n"
"    }\n"
"   "));
        label = new QLabel(add_music_single);
        label->setObjectName("label");
        label->setGeometry(QRect(120, 10, 531, 31));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);
        toolButton = new QToolButton(add_music_single);
        toolButton->setObjectName("toolButton");
        toolButton->setGeometry(QRect(550, 60, 111, 31));
        pushButton = new QPushButton(add_music_single);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(540, 320, 111, 51));
        pushButton_2 = new QPushButton(add_music_single);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(50, 320, 111, 51));
        layoutWidget_music1 = new QWidget(add_music_single);
        layoutWidget_music1->setObjectName("layoutWidget_music1");
        layoutWidget_music1->setGeometry(QRect(50, 60, 111, 231));
        verticalLayout = new QVBoxLayout(layoutWidget_music1);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(layoutWidget_music1);
        label_2->setObjectName("label_2");

        verticalLayout->addWidget(label_2);

        label_3 = new QLabel(layoutWidget_music1);
        label_3->setObjectName("label_3");

        verticalLayout->addWidget(label_3);

        label_4 = new QLabel(layoutWidget_music1);
        label_4->setObjectName("label_4");

        verticalLayout->addWidget(label_4);

        label_5 = new QLabel(layoutWidget_music1);
        label_5->setObjectName("label_5");

        verticalLayout->addWidget(label_5);

        label_6 = new QLabel(layoutWidget_music1);
        label_6->setObjectName("label_6");

        verticalLayout->addWidget(label_6);

        label_7 = new QLabel(layoutWidget_music1);
        label_7->setObjectName("label_7");

        verticalLayout->addWidget(label_7);

        label_8 = new QLabel(layoutWidget_music1);
        label_8->setObjectName("label_8");

        verticalLayout->addWidget(label_8);

        layoutWidgetmusic2 = new QWidget(add_music_single);
        layoutWidgetmusic2->setObjectName("layoutWidgetmusic2");
        layoutWidgetmusic2->setGeometry(QRect(170, 60, 371, 194));
        verticalLayout_2 = new QVBoxLayout(layoutWidgetmusic2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        txt_file = new QLineEdit(layoutWidgetmusic2);
        txt_file->setObjectName("txt_file");

        verticalLayout_2->addWidget(txt_file);

        txt_artist = new QLineEdit(layoutWidgetmusic2);
        txt_artist->setObjectName("txt_artist");

        verticalLayout_2->addWidget(txt_artist);

        txt_song = new QLineEdit(layoutWidgetmusic2);
        txt_song->setObjectName("txt_song");

        verticalLayout_2->addWidget(txt_song);

        cbox_g1 = new QComboBox(layoutWidgetmusic2);
        cbox_g1->setObjectName("cbox_g1");
        cbox_g1->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        verticalLayout_2->addWidget(cbox_g1);

        cbox_g2 = new QComboBox(layoutWidgetmusic2);
        cbox_g2->setObjectName("cbox_g2");
        cbox_g2->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        verticalLayout_2->addWidget(cbox_g2);

        cbox_country = new QComboBox(layoutWidgetmusic2);
        cbox_country->addItem(QString());
        cbox_country->addItem(QString());
        cbox_country->setObjectName("cbox_country");
        cbox_country->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        verticalLayout_2->addWidget(cbox_country);

        label_9 = new QLabel(add_music_single);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(50, 0, 51, 51));
        label_9->setPixmap(QPixmap(QString::fromUtf8("../../../../usr/share/icons/Mint-X/mimetypes/48/application-ogg.png")));
        f_date = new QDateEdit(add_music_single);
        f_date->setObjectName("f_date");
        f_date->setGeometry(QRect(170, 260, 161, 26));
        bt_manageGenres = new QPushButton(add_music_single);
        bt_manageGenres->setObjectName("bt_manageGenres");
        bt_manageGenres->setGeometry(QRect(550, 160, 111, 25));

        retranslateUi(add_music_single);

        QMetaObject::connectSlotsByName(add_music_single);
    } // setupUi

    void retranslateUi(QDialog *add_music_single)
    {
        add_music_single->setWindowTitle(QCoreApplication::translate("add_music_single", "Add a song", nullptr));
        label->setText(QCoreApplication::translate("add_music_single", "Add Single music file", nullptr));
        toolButton->setText(QCoreApplication::translate("add_music_single", "Browse", nullptr));
        pushButton->setText(QCoreApplication::translate("add_music_single", "Save", nullptr));
        pushButton_2->setText(QCoreApplication::translate("add_music_single", "Cancel", nullptr));
        label_2->setText(QCoreApplication::translate("add_music_single", "File:", nullptr));
        label_3->setText(QCoreApplication::translate("add_music_single", "Artist:", nullptr));
        label_4->setText(QCoreApplication::translate("add_music_single", "Song:", nullptr));
        label_5->setText(QCoreApplication::translate("add_music_single", "Genre 1:", nullptr));
        label_6->setText(QCoreApplication::translate("add_music_single", "Genre 2:", nullptr));
#if QT_CONFIG(tooltip)
        label_7->setToolTip(QCoreApplication::translate("add_music_single", "<html><head/><body><p>This field will be used to see if a music is &quot;national&quot; or &quot;international&quot;</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        label_7->setText(QCoreApplication::translate("add_music_single", "Country:", nullptr));
#if QT_CONFIG(tooltip)
        label_8->setToolTip(QCoreApplication::translate("add_music_single", "<html><head/><body><p>This date will be used to calculate the amount of new music played</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        label_8->setText(QCoreApplication::translate("add_music_single", "Published date:", nullptr));
        cbox_country->setItemText(0, QCoreApplication::translate("add_music_single", "Portugal (or Portuguese speaking partners of CPLP)", nullptr));
        cbox_country->setItemText(1, QCoreApplication::translate("add_music_single", "Other", nullptr));

        cbox_country->setCurrentText(QCoreApplication::translate("add_music_single", "Portugal (or Portuguese speaking partners of CPLP)", nullptr));
        label_9->setText(QString());
        f_date->setDisplayFormat(QCoreApplication::translate("add_music_single", "yyyy/MM/dd", nullptr));
        bt_manageGenres->setText(QCoreApplication::translate("add_music_single", "Manage Genres", nullptr));
    } // retranslateUi

};

namespace Ui {
    class add_music_single: public Ui_add_music_single {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADD_MUSIC_SINGLE_H
