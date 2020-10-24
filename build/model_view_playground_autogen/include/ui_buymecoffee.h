/********************************************************************************
** Form generated from reading UI file 'buymecoffee.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BUYMECOFFEE_H
#define UI_BUYMECOFFEE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BuyMeCoffee
{
public:
    QLabel *label;
    QLabel *label_2;
    QLabel *label_4;
    QPushButton *pushButton;
    QLabel *label_5;
    QPushButton *pushButton_2;
    QLabel *label_7;
    QLabel *label_8;

    void setupUi(QWidget *BuyMeCoffee)
    {
        if (BuyMeCoffee->objectName().isEmpty())
            BuyMeCoffee->setObjectName(QStringLiteral("BuyMeCoffee"));
        BuyMeCoffee->resize(640, 480);
        BuyMeCoffee->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        label = new QLabel(BuyMeCoffee);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(40, 80, 561, 181));
        label_2 = new QLabel(BuyMeCoffee);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(250, 30, 151, 21));
        QFont font;
        font.setPointSize(12);
        label_2->setFont(font);
        label_4 = new QLabel(BuyMeCoffee);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(100, 250, 421, 141));
        label_4->setPixmap(QPixmap(QString::fromUtf8(":/images/images/novo logo_12_cfundo2_900.png")));
        label_4->setScaledContents(true);
        pushButton = new QPushButton(BuyMeCoffee);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(20, 410, 141, 41));
        label_5 = new QLabel(BuyMeCoffee);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(150, 420, 51, 51));
        label_5->setPixmap(QPixmap(QString::fromUtf8(":/images/usr/share/icons/Mint-X/apps/48/caffeine.png")));
        pushButton_2 = new QPushButton(BuyMeCoffee);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(280, 800, 281, 51));
        label_7 = new QLabel(BuyMeCoffee);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(140, 480, 561, 181));
        label_8 = new QLabel(BuyMeCoffee);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(200, 650, 421, 141));
        label_8->setPixmap(QPixmap(QString::fromUtf8(":/images/images/novo logo_12_cfundo2_900.png")));
        label_8->setScaledContents(true);

        retranslateUi(BuyMeCoffee);

        QMetaObject::connectSlotsByName(BuyMeCoffee);
    } // setupUi

    void retranslateUi(QWidget *BuyMeCoffee)
    {
        BuyMeCoffee->setWindowTitle(QApplication::translate("BuyMeCoffee", "Buy me a Coffee!", nullptr));
        label->setText(QApplication::translate("BuyMeCoffee", "<html><head/><body><p>AudioX is an Open-Source Software, writen in C++,<br/>by Fr\303\251d\303\251ric Bogaerts, Founder, CEO and developer at Netpack - Online Solutions!</p><p>Any financial support is welcome.</p><p>If you can pay me a coffee please send it with Paypal to: fred@netpack.pt</p><p>Thank you very mutch for your support!<br/></p><p>For more information about Netpack visit <a href=\"http://www.netpack.pt\"><span style=\" text-decoration: underline; color:#0000ff;\">www.netpack.pt</span></a></p><p><br/></p></body></html>", nullptr));
        label_2->setText(QApplication::translate("BuyMeCoffee", "Buy me a Coffee!", nullptr));
        label_4->setText(QString());
        pushButton->setText(QApplication::translate("BuyMeCoffee", "Close", nullptr));
        label_5->setText(QString());
        pushButton_2->setText(QApplication::translate("BuyMeCoffee", "Close", nullptr));
        label_7->setText(QApplication::translate("BuyMeCoffee", "<html><head/><body><p>AudioX is an Open-Source Software, writen in C++,<br/>by Fr\303\251d\303\251ric Bogaerts, Founder, CEO and developer at Netpack - Online Solutions!</p><p>Any financial support is welcome.</p><p>If you can pay me a coffee please send it with Paypal to: fred@netpack.pt</p><p>Thank you very mutch for your support!<br/></p><p>For more information about Netpack visit <a href=\"http://www.netpack.pt\"><span style=\" text-decoration: underline; color:#0000ff;\">www.netpack.pt</span></a></p><p><br/></p></body></html>", nullptr));
        label_8->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class BuyMeCoffee: public Ui_BuyMeCoffee {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BUYMECOFFEE_H
