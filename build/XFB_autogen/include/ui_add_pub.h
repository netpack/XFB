/********************************************************************************
** Form generated from reading UI file 'add_pub.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADD_PUB_H
#define UI_ADD_PUB_H

#include <QtCore/QDate>
#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_add_pub
{
public:
    QLabel *label;
    QPushButton *pushButton;
    QPushButton *pushButton_2;
    QGroupBox *schedules;
    QDateTimeEdit *dateTimeEdit;
    QPushButton *pushButton_4;
    QPushButton *pushButton_5;
    QLabel *label_5;
    QLabel *label_6;
    QFrame *line;
    QFrame *line_2;
    QComboBox *cbox_dayOfTheWeek;
    QTimeEdit *hourMinute;
    QPushButton *pushButton_6;
    QFrame *line_3;
    QLabel *label_7;
    QDateEdit *dateEdit;
    QLabel *label_8;
    QLabel *label_9;
    QDateEdit *dateEdit_2;
    QLabel *label_10;
    QTimeEdit *timeEdit_2;
    QPushButton *pushButton_7;
    QListWidget *listWidget;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QLabel *label_3;
    QLineEdit *txt_name;
    QWidget *widget;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QLabel *txt_selected_file;
    QPushButton *pushButton_3;

    void setupUi(QDialog *add_pub)
    {
        if (add_pub->objectName().isEmpty())
            add_pub->setObjectName(QStringLiteral("add_pub"));
        add_pub->setWindowModality(Qt::WindowModal);
        add_pub->resize(811, 704);
        add_pub->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        label = new QLabel(add_pub);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(40, 10, 311, 31));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setWeight(75);
        label->setFont(font);
        pushButton = new QPushButton(add_pub);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setGeometry(QRect(610, 620, 131, 51));
        pushButton_2 = new QPushButton(add_pub);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
        pushButton_2->setGeometry(QRect(60, 620, 131, 51));
        schedules = new QGroupBox(add_pub);
        schedules->setObjectName(QStringLiteral("schedules"));
        schedules->setEnabled(true);
        schedules->setGeometry(QRect(60, 150, 681, 441));
        dateTimeEdit = new QDateTimeEdit(schedules);
        dateTimeEdit->setObjectName(QStringLiteral("dateTimeEdit"));
        dateTimeEdit->setGeometry(QRect(60, 210, 141, 27));
        dateTimeEdit->setDateTime(QDateTime(QDate(2014, 8, 25), QTime(0, 0, 0)));
        dateTimeEdit->setMinimumDateTime(QDateTime(QDate(1814, 8, 25), QTime(13, 0, 0)));
        dateTimeEdit->setMinimumDate(QDate(1814, 8, 25));
        dateTimeEdit->setMinimumTime(QTime(13, 0, 0));
        dateTimeEdit->setCalendarPopup(true);
        pushButton_4 = new QPushButton(schedules);
        pushButton_4->setObjectName(QStringLiteral("pushButton_4"));
        pushButton_4->setGeometry(QRect(230, 210, 120, 27));
        pushButton_5 = new QPushButton(schedules);
        pushButton_5->setObjectName(QStringLiteral("pushButton_5"));
        pushButton_5->setGeometry(QRect(500, 136, 121, 31));
        label_5 = new QLabel(schedules);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setGeometry(QRect(50, 190, 101, 20));
        label_6 = new QLabel(schedules);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setGeometry(QRect(50, 260, 161, 20));
        line = new QFrame(schedules);
        line->setObjectName(QStringLiteral("line"));
        line->setGeometry(QRect(20, 170, 601, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line_2 = new QFrame(schedules);
        line_2->setObjectName(QStringLiteral("line_2"));
        line_2->setGeometry(QRect(20, 240, 601, 16));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);
        cbox_dayOfTheWeek = new QComboBox(schedules);
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->setObjectName(QStringLiteral("cbox_dayOfTheWeek"));
        cbox_dayOfTheWeek->setEnabled(true);
        cbox_dayOfTheWeek->setGeometry(QRect(60, 280, 100, 27));
        hourMinute = new QTimeEdit(schedules);
        hourMinute->setObjectName(QStringLiteral("hourMinute"));
        hourMinute->setEnabled(true);
        hourMinute->setGeometry(QRect(170, 280, 117, 27));
        hourMinute->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        hourMinute->setTimeSpec(Qt::LocalTime);
        pushButton_6 = new QPushButton(schedules);
        pushButton_6->setObjectName(QStringLiteral("pushButton_6"));
        pushButton_6->setEnabled(true);
        pushButton_6->setGeometry(QRect(310, 280, 120, 27));
        line_3 = new QFrame(schedules);
        line_3->setObjectName(QStringLiteral("line_3"));
        line_3->setGeometry(QRect(20, 310, 601, 16));
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);
        label_7 = new QLabel(schedules);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setGeometry(QRect(50, 330, 171, 17));
        dateEdit = new QDateEdit(schedules);
        dateEdit->setObjectName(QStringLiteral("dateEdit"));
        dateEdit->setEnabled(false);
        dateEdit->setGeometry(QRect(110, 350, 110, 27));
        label_8 = new QLabel(schedules);
        label_8->setObjectName(QStringLiteral("label_8"));
        label_8->setGeometry(QRect(70, 360, 31, 17));
        label_9 = new QLabel(schedules);
        label_9->setObjectName(QStringLiteral("label_9"));
        label_9->setGeometry(QRect(230, 360, 21, 17));
        dateEdit_2 = new QDateEdit(schedules);
        dateEdit_2->setObjectName(QStringLiteral("dateEdit_2"));
        dateEdit_2->setEnabled(false);
        dateEdit_2->setGeometry(QRect(250, 350, 110, 27));
        label_10 = new QLabel(schedules);
        label_10->setObjectName(QStringLiteral("label_10"));
        label_10->setGeometry(QRect(90, 400, 31, 17));
        timeEdit_2 = new QTimeEdit(schedules);
        timeEdit_2->setObjectName(QStringLiteral("timeEdit_2"));
        timeEdit_2->setEnabled(false);
        timeEdit_2->setGeometry(QRect(110, 390, 111, 27));
        pushButton_7 = new QPushButton(schedules);
        pushButton_7->setObjectName(QStringLiteral("pushButton_7"));
        pushButton_7->setEnabled(false);
        pushButton_7->setGeometry(QRect(240, 390, 121, 27));
        listWidget = new QListWidget(schedules);
        listWidget->setObjectName(QStringLiteral("listWidget"));
        listWidget->setGeometry(QRect(20, 30, 601, 101));
        layoutWidget = new QWidget(add_pub);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(70, 70, 671, 27));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_3 = new QLabel(layoutWidget);
        label_3->setObjectName(QStringLiteral("label_3"));

        horizontalLayout->addWidget(label_3);

        txt_name = new QLineEdit(layoutWidget);
        txt_name->setObjectName(QStringLiteral("txt_name"));

        horizontalLayout->addWidget(txt_name);

        widget = new QWidget(add_pub);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(71, 100, 671, 27));
        horizontalLayout_2 = new QHBoxLayout(widget);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_4 = new QLabel(widget);
        label_4->setObjectName(QStringLiteral("label_4"));

        horizontalLayout_2->addWidget(label_4);

        txt_selected_file = new QLabel(widget);
        txt_selected_file->setObjectName(QStringLiteral("txt_selected_file"));

        horizontalLayout_2->addWidget(txt_selected_file);

        pushButton_3 = new QPushButton(widget);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));

        horizontalLayout_2->addWidget(pushButton_3);


        retranslateUi(add_pub);

        QMetaObject::connectSlotsByName(add_pub);
    } // setupUi

    void retranslateUi(QDialog *add_pub)
    {
        add_pub->setWindowTitle(QApplication::translate("add_pub", "Add a Publicity", nullptr));
        label->setText(QApplication::translate("add_pub", "Add new publicity", nullptr));
        pushButton->setText(QApplication::translate("add_pub", "Save", nullptr));
        pushButton_2->setText(QApplication::translate("add_pub", "Cancel", nullptr));
        schedules->setTitle(QApplication::translate("add_pub", "Schedule Options", nullptr));
        dateTimeEdit->setDisplayFormat(QApplication::translate("add_pub", "dd/MM/yyyy hh:mm", nullptr));
        pushButton_4->setText(QApplication::translate("add_pub", "Add new", nullptr));
        pushButton_5->setText(QApplication::translate("add_pub", "Delete selected", nullptr));
        label_5->setText(QApplication::translate("add_pub", "Date and time:", nullptr));
        label_6->setText(QApplication::translate("add_pub", "Day of the week and time:", nullptr));
        cbox_dayOfTheWeek->setItemText(0, QApplication::translate("add_pub", "Monday", nullptr));
        cbox_dayOfTheWeek->setItemText(1, QApplication::translate("add_pub", "Tuesday", nullptr));
        cbox_dayOfTheWeek->setItemText(2, QApplication::translate("add_pub", "Wednesday", nullptr));
        cbox_dayOfTheWeek->setItemText(3, QApplication::translate("add_pub", "Thursday", nullptr));
        cbox_dayOfTheWeek->setItemText(4, QApplication::translate("add_pub", "Friday", nullptr));
        cbox_dayOfTheWeek->setItemText(5, QApplication::translate("add_pub", "Saturday", nullptr));
        cbox_dayOfTheWeek->setItemText(6, QApplication::translate("add_pub", "Sunday", nullptr));

        hourMinute->setDisplayFormat(QApplication::translate("add_pub", "hh:mm", nullptr));
        pushButton_6->setText(QApplication::translate("add_pub", "Add new", nullptr));
        label_7->setText(QApplication::translate("add_pub", "Date interval and time:", nullptr));
        label_8->setText(QApplication::translate("add_pub", "From:", nullptr));
        label_9->setText(QApplication::translate("add_pub", "To:", nullptr));
        label_10->setText(QApplication::translate("add_pub", "At:", nullptr));
        pushButton_7->setText(QApplication::translate("add_pub", "Add new", nullptr));
        label_3->setText(QApplication::translate("add_pub", "Name:", nullptr));
        label_4->setText(QApplication::translate("add_pub", "File:", nullptr));
        txt_selected_file->setText(QApplication::translate("add_pub", "No file selected yet...", nullptr));
        pushButton_3->setText(QApplication::translate("add_pub", "Select File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class add_pub: public Ui_add_pub {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADD_PUB_H
