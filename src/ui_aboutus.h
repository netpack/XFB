/********************************************************************************
** Form generated from reading UI file 'aboutus.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
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
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;

    void setupUi(QDialog *aboutUs)
    {
        if (aboutUs->objectName().isEmpty())
            aboutUs->setObjectName("aboutUs");
        aboutUs->setEnabled(true);
        aboutUs->resize(859, 629);
        label = new QLabel(aboutUs);
        label->setObjectName("label");
        label->setGeometry(QRect(60, 60, 571, 281));
        label_2 = new QLabel(aboutUs);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(10, 0, 71, 61));
        label_2->setPixmap(QPixmap(QString::fromUtf8(":/48x48.png")));
        label_3 = new QLabel(aboutUs);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(300, 530, 241, 91));
        label_3->setPixmap(QPixmap(QString::fromUtf8(":/images/novo logo_12_cfundo2_900.png")));
        label_3->setScaledContents(true);
        label_4 = new QLabel(aboutUs);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(80, 10, 251, 41));
        QFont font;
        font.setFamilies({QString::fromUtf8("Purisa")});
        font.setPointSize(25);
        font.setBold(true);
        label_4->setFont(font);
        label_5 = new QLabel(aboutUs);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(660, 80, 141, 151));
        label_5->setPixmap(QPixmap(QString::fromUtf8(":/images/donate.png")));
        label_6 = new QLabel(aboutUs);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(680, 230, 101, 21));
        label_7 = new QLabel(aboutUs);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(60, 340, 731, 181));
        label_7->setTextFormat(Qt::TextFormat::PlainText);
        label_7->setWordWrap(true);
        label_2->raise();
        label->raise();
        label_4->raise();
        label_3->raise();
        label_5->raise();
        label_6->raise();
        label_7->raise();

        retranslateUi(aboutUs);

        QMetaObject::connectSlotsByName(aboutUs);
    } // setupUi

    void retranslateUi(QDialog *aboutUs)
    {
        aboutUs->setWindowTitle(QCoreApplication::translate("aboutUs", "About", nullptr));
        label->setText(QCoreApplication::translate("aboutUs", "<html><head/><body><p>XFB is an Open-source software, writen in C++ and distributed under GPL-3.0.</p><p>Developed by Fr\303\251d\303\251ric Bogaerts.</p><p><a href=\"https://www.researchgate.net/profile/Frederic-Bogaerts\">https://www.researchgate.net/profile/Frederic-Bogaerts</a></p><p>Any support is very welcome!</p><p>If you can, buy me a coffee!: fred@netpack.pt (Paypal)</p><p>Thank you very much for your support!<br/></p><p>For more information about Netpack visit <a href=\"http://www.netpack.pt\">www.netpack.pt</a></p></body></html>", nullptr));
        label_2->setText(QString());
        label_3->setText(QString());
        label_4->setText(QCoreApplication::translate("aboutUs", "XFB", nullptr));
        label_5->setText(QString());
        label_6->setText(QCoreApplication::translate("aboutUs", "Scan to donate", nullptr));
        label_7->setText(QCoreApplication::translate("aboutUs", "This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 (GPL-3.0) as published by the Free Software Foundation.\n"
"\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.\n"
"", nullptr));
    } // retranslateUi

};

namespace Ui {
    class aboutUs: public Ui_aboutUs {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTUS_H
