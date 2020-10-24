/********************************************************************************
** Form generated from reading UI file 'aboutus.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTUS_H
#define UI_ABOUTUS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>

QT_BEGIN_NAMESPACE

class Ui_aboutUs
{
public:
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;

    void setupUi(QDialog *aboutUs)
    {
        if (aboutUs->objectName().isEmpty())
            aboutUs->setObjectName(QStringLiteral("aboutUs"));
        aboutUs->resize(630, 310);
        aboutUs->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        label = new QLabel(aboutUs);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(20, 60, 591, 211));
        label_2 = new QLabel(aboutUs);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(10, 0, 71, 61));
        label_2->setPixmap(QPixmap(QString::fromUtf8(":/48x48.png")));
        label_3 = new QLabel(aboutUs);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(190, 230, 241, 91));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/images/novo logo_12_cfundo2_900.png")));
        label_3->setScaledContents(true);
        label_4 = new QLabel(aboutUs);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(80, 10, 251, 41));
        QFont font;
        font.setFamily(QStringLiteral("Purisa"));
        font.setPointSize(25);
        font.setBold(true);
        font.setWeight(75);
        label_4->setFont(font);
        label_2->raise();
        label->raise();
        label_4->raise();
        label_3->raise();

        retranslateUi(aboutUs);

        QMetaObject::connectSlotsByName(aboutUs);
    } // setupUi

    void retranslateUi(QDialog *aboutUs)
    {
        aboutUs->setWindowTitle(QApplication::translate("aboutUs", "About", nullptr));
        label->setText(QApplication::translate("aboutUs", "<html><head/><body><p>XFB is an Open-Source Software, writen in C++,<br/>by Fr\303\251d\303\251ric Bogaerts, Founder, CEO and developer at Netpack - Online Solutions!</p><p>Any financial support is welcome.</p><p>If you can pay me a coffee please send it to ( Paypal ): fred@netpack.pt</p><p>Thank you very much for your support!<br/></p><p>For more information about Netpack visit <a href=\"http://www.netpack.pt\"><span style=\" text-decoration: underline; color:#0000ff;\">www.netpack.pt</span></a></p><p><br/></p></body></html>", nullptr));
        label_2->setText(QString());
        label_3->setText(QString());
        label_4->setText(QApplication::translate("aboutUs", "XFB", nullptr));
    } // retranslateUi

};

namespace Ui {
    class aboutUs: public Ui_aboutUs {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTUS_H
