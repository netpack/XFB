/********************************************************************************
** Form generated from reading UI file 'addgenre.ui'
**
** Created by: Qt User Interface Compiler version 5.15.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDGENRE_H
#define UI_ADDGENRE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_addgenre
{
public:
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QListView *listGenres;
    QPushButton *btAddNewGenre;
    QLabel *label_4;
    QLineEdit *txtNewGenre;
    QPushButton *close;
    QWidget *layoutWidget;
    QGridLayout *gridLayout;
    QPushButton *btShowGenre;
    QPushButton *btDelGenre;

    void setupUi(QDialog *addgenre)
    {
        if (addgenre->objectName().isEmpty())
            addgenre->setObjectName(QString::fromUtf8("addgenre"));
        addgenre->resize(412, 494);
        addgenre->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        label = new QLabel(addgenre);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(30, 10, 51, 61));
        label->setPixmap(QPixmap(QString::fromUtf8(":/usr/share/icons/Mint-X/apps/48/radio.png")));
        label_2 = new QLabel(addgenre);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(90, 20, 161, 31));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setWeight(75);
        label_2->setFont(font);
        label_3 = new QLabel(addgenre);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(30, 70, 62, 17));
        listGenres = new QListView(addgenre);
        listGenres->setObjectName(QString::fromUtf8("listGenres"));
        listGenres->setGeometry(QRect(30, 90, 331, 151));
        btAddNewGenre = new QPushButton(addgenre);
        btAddNewGenre->setObjectName(QString::fromUtf8("btAddNewGenre"));
        btAddNewGenre->setGeometry(QRect(210, 340, 151, 51));
        label_4 = new QLabel(addgenre);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(30, 290, 221, 17));
        txtNewGenre = new QLineEdit(addgenre);
        txtNewGenre->setObjectName(QString::fromUtf8("txtNewGenre"));
        txtNewGenre->setGeometry(QRect(30, 310, 331, 27));
        close = new QPushButton(addgenre);
        close->setObjectName(QString::fromUtf8("close"));
        close->setGeometry(QRect(70, 420, 251, 41));
        layoutWidget = new QWidget(addgenre);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(30, 250, 331, 29));
        gridLayout = new QGridLayout(layoutWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        btShowGenre = new QPushButton(layoutWidget);
        btShowGenre->setObjectName(QString::fromUtf8("btShowGenre"));

        gridLayout->addWidget(btShowGenre, 0, 0, 1, 1);

        btDelGenre = new QPushButton(layoutWidget);
        btDelGenre->setObjectName(QString::fromUtf8("btDelGenre"));

        gridLayout->addWidget(btDelGenre, 0, 1, 1, 1);


        retranslateUi(addgenre);

        QMetaObject::connectSlotsByName(addgenre);
    } // setupUi

    void retranslateUi(QDialog *addgenre)
    {
        addgenre->setWindowTitle(QCoreApplication::translate("addgenre", "Manage Genres", nullptr));
        label->setText(QString());
        label_2->setText(QCoreApplication::translate("addgenre", "Manage Genres", nullptr));
        label_3->setText(QCoreApplication::translate("addgenre", "Genres:", nullptr));
        btAddNewGenre->setText(QCoreApplication::translate("addgenre", "Add Genre", nullptr));
        label_4->setText(QCoreApplication::translate("addgenre", "Add a new Genre:", nullptr));
        close->setText(QCoreApplication::translate("addgenre", "Close", nullptr));
        btShowGenre->setText(QCoreApplication::translate("addgenre", "Show / Refresh List", nullptr));
        btDelGenre->setText(QCoreApplication::translate("addgenre", "Delete Selected Genre", nullptr));
    } // retranslateUi

};

namespace Ui {
    class addgenre: public Ui_addgenre {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDGENRE_H
