/********************************************************************************
** Form generated from reading UI file 'manage_db.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MANAGE_DB_H
#define UI_MANAGE_DB_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_manage_db
{
public:
    QDialogButtonBox *buttonBox;
    QLabel *label;
    QPushButton *pushButton;
    QLabel *label_2;

    void setupUi(QDialog *manage_db)
    {
        if (manage_db->objectName().isEmpty())
            manage_db->setObjectName(QStringLiteral("manage_db"));
        manage_db->resize(400, 300);
        buttonBox = new QDialogButtonBox(manage_db);
        buttonBox->setObjectName(QStringLiteral("buttonBox"));
        buttonBox->setGeometry(QRect(30, 240, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        label = new QLabel(manage_db);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(20, 10, 101, 21));
        pushButton = new QPushButton(manage_db);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(120, 30, 131, 31));
        label_2 = new QLabel(manage_db);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(130, 10, 261, 21));

        retranslateUi(manage_db);
        QObject::connect(buttonBox, SIGNAL(accepted()), manage_db, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), manage_db, SLOT(reject()));

        QMetaObject::connectSlotsByName(manage_db);
    } // setupUi

    void retranslateUi(QDialog *manage_db)
    {
        manage_db->setWindowTitle(QApplication::translate("manage_db", "Dialog", nullptr));
        label->setText(QApplication::translate("manage_db", "Database Path:", nullptr));
        pushButton->setText(QApplication::translate("manage_db", "Select database", nullptr));
        label_2->setText(QApplication::translate("manage_db", "No databse selected...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class manage_db: public Ui_manage_db {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MANAGE_DB_H
