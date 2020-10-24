/********************************************************************************
** Form generated from reading UI file 'add_full_dir.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADD_FULL_DIR_H
#define UI_ADD_FULL_DIR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_add_full_dir
{
public:
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLineEdit *txt_path;
    QLineEdit *txt_artistName;
    QLabel *label_7;
    QPushButton *f_bt_browse;
    QPushButton *f_bt_add;
    QLabel *label_8;
    QComboBox *f_cbox_genre1;
    QComboBox *f_cbox_genre2;
    QDateEdit *f_date;
    QPushButton *f_bt_manageGenres;
    QLabel *label_9;
    QComboBox *f_cbox_country;
    QLabel *label_10;

    void setupUi(QDialog *add_full_dir)
    {
        if (add_full_dir->objectName().isEmpty())
            add_full_dir->setObjectName(QStringLiteral("add_full_dir"));
        add_full_dir->resize(646, 342);
        add_full_dir->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        label = new QLabel(add_full_dir);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(80, 20, 521, 21));
        QFont font;
        font.setPointSize(11);
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);
        label_2 = new QLabel(add_full_dir);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(20, 70, 54, 17));
        label_3 = new QLabel(add_full_dir);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(40, 110, 141, 17));
        label_4 = new QLabel(add_full_dir);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(40, 140, 131, 20));
        label_5 = new QLabel(add_full_dir);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(40, 170, 131, 17));
        label_6 = new QLabel(add_full_dir);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(20, 300, 281, 20));
        QFont font1;
        font1.setPointSize(8);
        font1.setBold(false);
        font1.setItalic(true);
        font1.setUnderline(false);
        font1.setWeight(50);
        label_6->setFont(font1);
        txt_path = new QLineEdit(add_full_dir);
        txt_path->setObjectName(QStringLiteral("txt_path"));
        txt_path->setGeometry(QRect(70, 70, 231, 20));
        txt_artistName = new QLineEdit(add_full_dir);
        txt_artistName->setObjectName(QStringLiteral("txt_artistName"));
        txt_artistName->setGeometry(QRect(200, 105, 241, 20));
        label_7 = new QLabel(add_full_dir);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(180, 100, 16, 17));
        f_bt_browse = new QPushButton(add_full_dir);
        f_bt_browse->setObjectName(QStringLiteral("f_bt_browse"));
        f_bt_browse->setGeometry(QRect(310, 70, 91, 21));
        f_bt_add = new QPushButton(add_full_dir);
        f_bt_add->setObjectName(QStringLiteral("f_bt_add"));
        f_bt_add->setGeometry(QRect(460, 270, 161, 51));
        label_8 = new QLabel(add_full_dir);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(40, 230, 141, 20));
        f_cbox_genre1 = new QComboBox(add_full_dir);
        f_cbox_genre1->setObjectName(QStringLiteral("f_cbox_genre1"));
        f_cbox_genre1->setGeometry(QRect(200, 140, 241, 21));
        f_cbox_genre1->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));
        f_cbox_genre2 = new QComboBox(add_full_dir);
        f_cbox_genre2->setObjectName(QStringLiteral("f_cbox_genre2"));
        f_cbox_genre2->setGeometry(QRect(200, 170, 241, 21));
        f_cbox_genre2->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));
        f_date = new QDateEdit(add_full_dir);
        f_date->setObjectName(QStringLiteral("f_date"));
        f_date->setGeometry(QRect(200, 230, 241, 25));
        f_bt_manageGenres = new QPushButton(add_full_dir);
        f_bt_manageGenres->setObjectName(QStringLiteral("f_bt_manageGenres"));
        f_bt_manageGenres->setGeometry(QRect(450, 140, 121, 25));
        label_9 = new QLabel(add_full_dir);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(40, 200, 131, 17));
        f_cbox_country = new QComboBox(add_full_dir);
        f_cbox_country->setObjectName(QStringLiteral("f_cbox_country"));
        f_cbox_country->setGeometry(QRect(200, 200, 241, 25));
        f_cbox_country->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));
        label_10 = new QLabel(add_full_dir);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(20, 0, 51, 51));
        label_10->setPixmap(QPixmap(QString::fromUtf8(":/icons/app-ogg.png")));

        retranslateUi(add_full_dir);

        QMetaObject::connectSlotsByName(add_full_dir);
    } // setupUi

    void retranslateUi(QDialog *add_full_dir)
    {
        add_full_dir->setWindowTitle(QApplication::translate("add_full_dir", "Add a full directory", nullptr));
        label->setText(QApplication::translate("add_full_dir", "Add all songs in a folder", nullptr));
        label_2->setText(QApplication::translate("add_full_dir", "Folder:", nullptr));
        label_3->setText(QApplication::translate("add_full_dir", "Artist name:", nullptr));
        label_4->setText(QApplication::translate("add_full_dir", "Genre1:", nullptr));
        label_5->setText(QApplication::translate("add_full_dir", "Genre2:", nullptr));
        label_6->setText(QApplication::translate("add_full_dir", "*leave empty for auto detection (artist - song.ext)", nullptr));
        label_7->setText(QApplication::translate("add_full_dir", "*", nullptr));
        f_bt_browse->setText(QApplication::translate("add_full_dir", "Browse", nullptr));
        f_bt_add->setText(QApplication::translate("add_full_dir", "Add", nullptr));
        label_8->setText(QApplication::translate("add_full_dir", "Published date:", nullptr));
        f_date->setDisplayFormat(QApplication::translate("add_full_dir", "yyyy/MM/dd", nullptr));
        f_bt_manageGenres->setText(QApplication::translate("add_full_dir", "Manage genres", nullptr));
        label_9->setText(QApplication::translate("add_full_dir", "Country:", nullptr));
        f_cbox_country->setCurrentText(QString());
        label_10->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class add_full_dir: public Ui_add_full_dir {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADD_FULL_DIR_H
