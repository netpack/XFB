/********************************************************************************
** Form generated from reading UI file 'addjingle.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADDJINGLE_H
#define UI_ADDJINGLE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_addJingle
{
public:
    QPushButton *pushButton;
    QToolButton *toolButton;
    QLabel *label;
    QLabel *label_9;
    QWidget *layoutWidget;
    QVBoxLayout *verticalLayout;
    QLabel *label_2;
    QLabel *label_3;
    QWidget *layoutWidget_2;
    QVBoxLayout *verticalLayout_2;
    QLineEdit *txt_file;
    QLineEdit *txt_name;
    QPushButton *pushButton_2;

    void setupUi(QDialog *addJingle)
    {
        if (addJingle->objectName().isEmpty())
            addJingle->setObjectName(QStringLiteral("addJingle"));
        addJingle->resize(699, 343);
        addJingle->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        pushButton = new QPushButton(addJingle);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(530, 270, 121, 51));
        toolButton = new QToolButton(addJingle);
        toolButton->setObjectName(QStringLiteral("toolButton"));
        toolButton->setGeometry(QRect(530, 100, 61, 31));
        label = new QLabel(addJingle);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(120, 30, 201, 17));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);
        label_9 = new QLabel(addJingle);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(50, 10, 51, 51));
        label_9->setPixmap(QPixmap(QString::fromUtf8("../../../../usr/share/icons/Mint-X/mimetypes/48/application-ogg.png")));
        layoutWidget = new QWidget(addJingle);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(75, 90, 61, 101));
        verticalLayout = new QVBoxLayout(layoutWidget);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        label_2 = new QLabel(layoutWidget);
        label_2->setObjectName(QStringLiteral("label_2"));

        verticalLayout->addWidget(label_2);

        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName(QStringLiteral("label_3"));

        verticalLayout->addWidget(label_3);

        layoutWidget_2 = new QWidget(addJingle);
        layoutWidget_2->setObjectName(QStringLiteral("layoutWidget_2"));
        layoutWidget_2->setGeometry(QRect(140, 90, 381, 101));
        verticalLayout_2 = new QVBoxLayout(layoutWidget_2);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        txt_file = new QLineEdit(layoutWidget_2);
        txt_file->setObjectName(QStringLiteral("txt_file"));

        verticalLayout_2->addWidget(txt_file);

        txt_name = new QLineEdit(layoutWidget_2);
        txt_name->setObjectName(QStringLiteral("txt_name"));

        verticalLayout_2->addWidget(txt_name);

        pushButton_2 = new QPushButton(addJingle);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(40, 270, 121, 51));

        retranslateUi(addJingle);

        QMetaObject::connectSlotsByName(addJingle);
    } // setupUi

    void retranslateUi(QDialog *addJingle)
    {
        addJingle->setWindowTitle(QApplication::translate("addJingle", "Add a Jingle", nullptr));
        pushButton->setText(QApplication::translate("addJingle", "Add jingle", nullptr));
        toolButton->setText(QApplication::translate("addJingle", "...", nullptr));
        label->setText(QApplication::translate("addJingle", "Add Jingle", nullptr));
        label_9->setText(QString());
        label_2->setText(QApplication::translate("addJingle", "File:", nullptr));
        label_3->setText(QApplication::translate("addJingle", "Name:", nullptr));
        pushButton_2->setText(QApplication::translate("addJingle", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class addJingle: public Ui_addJingle {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADDJINGLE_H
