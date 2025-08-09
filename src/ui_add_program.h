/********************************************************************************
** Form generated from reading UI file 'add_program.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ADD_PROGRAM_H
#define UI_ADD_PROGRAM_H

#include <QtCore/QDate>
#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
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

class Ui_add_program
{
public:
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
    QWidget *layoutWidgetp1;
    QHBoxLayout *horizontalLayout;
    QLabel *label_3;
    QLineEdit *txt_name;
    QLabel *label;
    QPushButton *pushButton_2;
    QPushButton *pushButton;
    QLabel *label_2;
    QWidget *layoutWidgetp2;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_4;
    QLabel *txt_selected_file;
    QPushButton *pushButton_3;

    void setupUi(QDialog *add_program)
    {
        if (add_program->objectName().isEmpty())
            add_program->setObjectName("add_program");
        add_program->setStyleSheet(QString::fromUtf8("\n"
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
        add_program->resize(716, 686);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/48x48.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        add_program->setWindowIcon(icon);
        schedules = new QGroupBox(add_program);
        schedules->setObjectName("schedules");
        schedules->setEnabled(true);
        schedules->setGeometry(QRect(20, 140, 681, 441));
        dateTimeEdit = new QDateTimeEdit(schedules);
        dateTimeEdit->setObjectName("dateTimeEdit");
        dateTimeEdit->setGeometry(QRect(60, 210, 141, 27));
        dateTimeEdit->setDateTime(QDateTime(QDate(2014, 8, 25), QTime(0, 0, 0)));
        dateTimeEdit->setMinimumDateTime(QDateTime(QDate(1814, 8, 25), QTime(13, 0, 0)));
        dateTimeEdit->setMinimumDate(QDate(1814, 8, 25));
        dateTimeEdit->setMinimumTime(QTime(13, 0, 0));
        dateTimeEdit->setCalendarPopup(true);
        pushButton_4 = new QPushButton(schedules);
        pushButton_4->setObjectName("pushButton_4");
        pushButton_4->setGeometry(QRect(230, 210, 120, 27));
        pushButton_5 = new QPushButton(schedules);
        pushButton_5->setObjectName("pushButton_5");
        pushButton_5->setGeometry(QRect(500, 136, 121, 31));
        label_5 = new QLabel(schedules);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(50, 190, 101, 20));
        label_6 = new QLabel(schedules);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(50, 260, 161, 20));
        line = new QFrame(schedules);
        line->setObjectName("line");
        line->setGeometry(QRect(20, 170, 601, 16));
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        line_2 = new QFrame(schedules);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(20, 240, 601, 16));
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        cbox_dayOfTheWeek = new QComboBox(schedules);
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->addItem(QString());
        cbox_dayOfTheWeek->setObjectName("cbox_dayOfTheWeek");
        cbox_dayOfTheWeek->setEnabled(true);
        cbox_dayOfTheWeek->setGeometry(QRect(60, 280, 100, 27));
        cbox_dayOfTheWeek->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));
        hourMinute = new QTimeEdit(schedules);
        hourMinute->setObjectName("hourMinute");
        hourMinute->setEnabled(true);
        hourMinute->setGeometry(QRect(170, 280, 117, 27));
        hourMinute->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        hourMinute->setTimeSpec(Qt::LocalTime);
        pushButton_6 = new QPushButton(schedules);
        pushButton_6->setObjectName("pushButton_6");
        pushButton_6->setEnabled(true);
        pushButton_6->setGeometry(QRect(310, 280, 120, 27));
        line_3 = new QFrame(schedules);
        line_3->setObjectName("line_3");
        line_3->setGeometry(QRect(20, 310, 601, 16));
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);
        label_7 = new QLabel(schedules);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(50, 330, 171, 17));
        dateEdit = new QDateEdit(schedules);
        dateEdit->setObjectName("dateEdit");
        dateEdit->setEnabled(false);
        dateEdit->setGeometry(QRect(110, 350, 110, 27));
        label_8 = new QLabel(schedules);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(70, 360, 31, 17));
        label_9 = new QLabel(schedules);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(230, 360, 21, 17));
        dateEdit_2 = new QDateEdit(schedules);
        dateEdit_2->setObjectName("dateEdit_2");
        dateEdit_2->setEnabled(false);
        dateEdit_2->setGeometry(QRect(250, 350, 110, 27));
        label_10 = new QLabel(schedules);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(90, 400, 31, 17));
        timeEdit_2 = new QTimeEdit(schedules);
        timeEdit_2->setObjectName("timeEdit_2");
        timeEdit_2->setEnabled(false);
        timeEdit_2->setGeometry(QRect(110, 390, 111, 27));
        pushButton_7 = new QPushButton(schedules);
        pushButton_7->setObjectName("pushButton_7");
        pushButton_7->setEnabled(false);
        pushButton_7->setGeometry(QRect(240, 390, 121, 27));
        listWidget = new QListWidget(schedules);
        listWidget->setObjectName("listWidget");
        listWidget->setGeometry(QRect(20, 30, 601, 101));
        layoutWidgetp1 = new QWidget(add_program);
        layoutWidgetp1->setObjectName("layoutWidgetp1");
        layoutWidgetp1->setGeometry(QRect(30, 60, 671, 27));
        horizontalLayout = new QHBoxLayout(layoutWidgetp1);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_3 = new QLabel(layoutWidgetp1);
        label_3->setObjectName("label_3");

        horizontalLayout->addWidget(label_3);

        txt_name = new QLineEdit(layoutWidgetp1);
        txt_name->setObjectName("txt_name");

        horizontalLayout->addWidget(txt_name);

        label = new QLabel(add_program);
        label->setObjectName("label");
        label->setGeometry(QRect(90, 10, 601, 31));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);
        pushButton_2 = new QPushButton(add_program);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setGeometry(QRect(20, 610, 131, 51));
        pushButton = new QPushButton(add_program);
        pushButton->setObjectName("pushButton");
        pushButton->setGeometry(QRect(570, 610, 131, 51));
        label_2 = new QLabel(add_program);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(30, 0, 41, 41));
        label_2->setPixmap(QPixmap(QString::fromUtf8(":/icons/add-event-icon.png")));
        label_2->setScaledContents(true);
        layoutWidgetp2 = new QWidget(add_program);
        layoutWidgetp2->setObjectName("layoutWidgetp2");
        layoutWidgetp2->setGeometry(QRect(31, 90, 671, 27));
        horizontalLayout_2 = new QHBoxLayout(layoutWidgetp2);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_4 = new QLabel(layoutWidgetp2);
        label_4->setObjectName("label_4");

        horizontalLayout_2->addWidget(label_4);

        txt_selected_file = new QLabel(layoutWidgetp2);
        txt_selected_file->setObjectName("txt_selected_file");

        horizontalLayout_2->addWidget(txt_selected_file);

        pushButton_3 = new QPushButton(layoutWidgetp2);
        pushButton_3->setObjectName("pushButton_3");

        horizontalLayout_2->addWidget(pushButton_3);


        retranslateUi(add_program);

        QMetaObject::connectSlotsByName(add_program);
    } // setupUi

    void retranslateUi(QDialog *add_program)
    {
        add_program->setWindowTitle(QCoreApplication::translate("add_program", "Dialog", nullptr));
        schedules->setTitle(QCoreApplication::translate("add_program", "Schedule Options", nullptr));
        dateTimeEdit->setDisplayFormat(QCoreApplication::translate("add_program", "dd/MM/yyyy hh:mm", nullptr));
        pushButton_4->setText(QCoreApplication::translate("add_program", "Add new", nullptr));
        pushButton_5->setText(QCoreApplication::translate("add_program", "Delete selected", nullptr));
        label_5->setText(QCoreApplication::translate("add_program", "Date and time:", nullptr));
        label_6->setText(QCoreApplication::translate("add_program", "Day of the week and time:", nullptr));
        cbox_dayOfTheWeek->setItemText(0, QCoreApplication::translate("add_program", "Monday", nullptr));
        cbox_dayOfTheWeek->setItemText(1, QCoreApplication::translate("add_program", "Tuesday", nullptr));
        cbox_dayOfTheWeek->setItemText(2, QCoreApplication::translate("add_program", "Wednesday", nullptr));
        cbox_dayOfTheWeek->setItemText(3, QCoreApplication::translate("add_program", "Thursday", nullptr));
        cbox_dayOfTheWeek->setItemText(4, QCoreApplication::translate("add_program", "Friday", nullptr));
        cbox_dayOfTheWeek->setItemText(5, QCoreApplication::translate("add_program", "Saturday", nullptr));
        cbox_dayOfTheWeek->setItemText(6, QCoreApplication::translate("add_program", "Sunday", nullptr));

        hourMinute->setDisplayFormat(QCoreApplication::translate("add_program", "hh:mm", nullptr));
        pushButton_6->setText(QCoreApplication::translate("add_program", "Add new", nullptr));
        label_7->setText(QCoreApplication::translate("add_program", "Date interval and time:", nullptr));
        label_8->setText(QCoreApplication::translate("add_program", "From:", nullptr));
        label_9->setText(QCoreApplication::translate("add_program", "To:", nullptr));
        label_10->setText(QCoreApplication::translate("add_program", "At:", nullptr));
        pushButton_7->setText(QCoreApplication::translate("add_program", "Add new", nullptr));
        label_3->setText(QCoreApplication::translate("add_program", "Name:", nullptr));
        label->setText(QCoreApplication::translate("add_program", "Add new program", nullptr));
        pushButton_2->setText(QCoreApplication::translate("add_program", "Cancel", nullptr));
        pushButton->setText(QCoreApplication::translate("add_program", "Save", nullptr));
        label_2->setText(QString());
        label_4->setText(QCoreApplication::translate("add_program", "File:", nullptr));
        txt_selected_file->setText(QCoreApplication::translate("add_program", "No file selected yet...", nullptr));
        pushButton_3->setText(QCoreApplication::translate("add_program", "Select File", nullptr));
    } // retranslateUi

};

namespace Ui {
    class add_program: public Ui_add_program {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ADD_PROGRAM_H
