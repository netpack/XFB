/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPTIONSDIALOG_H
#define UI_OPTIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTimeEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_optionsDialog
{
public:
    QGridLayout *gridLayout;
    QFrame *verticalFrame;
    QVBoxLayout *layout;
    QTabWidget *SystemResouces;
    QWidget *tab;
    QLabel *label_3;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QLabel *label_5;
    QComboBox *cbox_lang;
    QWidget *layoutWidget1;
    QVBoxLayout *verticalLayout;
    QCheckBox *checkBox_disableSeekBar;
    QCheckBox *checkBox_disableVolume;
    QCheckBox *checkBox_Normalize_Soft;
    QCheckBox *checkBox_fullScreen;
    QTimeEdit *timeEdit_autoStartTime;
    QTimeEdit *timeEdit_autoStopTime;
    QCheckBox *checkBox_enableStartStopTime;
    QLabel *label_23;
    QLabel *label_24;
    QWidget *tab_2;
    QLabel *label;
    QLabel *txt_selected_db;
    QLabel *label_2;
    QPushButton *pushButton_3;
    QLabel *label_4;
    QPushButton *f_bt_del_jingles_table;
    QPushButton *f_bt_del_pub_table;
    QWidget *recording;
    QWidget *layoutWidget2;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_11;
    QComboBox *cboxRecDev;
    QWidget *layoutWidget3;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_12;
    QLineEdit *txt_savePath;
    QToolButton *bt_browseSavePath;
    QWidget *layoutWidget4;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_13;
    QLineEdit *txt_programsPath;
    QToolButton *bt_browse_programPath;
    QLabel *label_16;
    QWidget *layoutWidget5;
    QHBoxLayout *horizontalLayout_9;
    QLabel *label_17;
    QComboBox *comboBox_codec;
    QWidget *layoutWidget6;
    QHBoxLayout *horizontalLayout_10;
    QLabel *label_18;
    QComboBox *comboBox_container;
    QLabel *label_19;
    QWidget *layoutWidget_2;
    QHBoxLayout *horizontalLayout_11;
    QLabel *label_20;
    QLineEdit *txt_musicPath;
    QToolButton *bt_browse_musicPath;
    QWidget *layoutWidget_3;
    QHBoxLayout *horizontalLayout_12;
    QLabel *label_21;
    QLineEdit *txt_jinglePath;
    QToolButton *bt_browse_jinglePath;
    QFrame *line;
    QWidget *tab_4;
    QCheckBox *cbox_enableNetworking;
    QWidget *layoutWidget7;
    QFormLayout *formLayout;
    QLabel *label_6;
    QLineEdit *txt_server;
    QLabel *label_7;
    QLineEdit *txt_port;
    QLabel *label_8;
    QLineEdit *txt_user;
    QLabel *label_9;
    QLineEdit *txt_password;
    QLabel *label_10;
    QComboBox *cbox_role;
    QWidget *layoutWidget8;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_14;
    QLineEdit *txt_FTPlocalTempFolder;
    QToolButton *bt_browseFTPlocalFolder;
    QWidget *layoutWidget9;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_15;
    QTimeEdit *cboxComHour;
    QWidget *layoutWidget10;
    QHBoxLayout *horizontalLayout_13;
    QLabel *label_22;
    QLineEdit *txt_takeOverlocalTempFolder;
    QToolButton *bt_browseTakeOverlocalFolder;
    QWidget *tab_3;
    QPlainTextEdit *txt_terminal;
    QWidget *layoutWidget11;
    QHBoxLayout *horizontalLayout_7;
    QPushButton *bt_uname;
    QPushButton *bt_pwd;
    QPushButton *bt_free;
    QPushButton *bt_df;
    QWidget *layoutWidget12;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *bt_update_youtubedl;
    QPushButton *bt_edit_settings;
    QPushButton *pushButton;
    QPushButton *bt_save_settings;
    QPushButton *pushButton_2;

    void setupUi(QDialog *optionsDialog)
    {
        if (optionsDialog->objectName().isEmpty())
            optionsDialog->setObjectName(QString::fromUtf8("optionsDialog"));
        optionsDialog->resize(789, 386);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        optionsDialog->setWindowIcon(icon);
        optionsDialog->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        gridLayout = new QGridLayout(optionsDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalFrame = new QFrame(optionsDialog);
        verticalFrame->setObjectName(QString::fromUtf8("verticalFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(verticalFrame->sizePolicy().hasHeightForWidth());
        verticalFrame->setSizePolicy(sizePolicy);
        verticalFrame->setMinimumSize(QSize(721, 0));
        verticalFrame->setFrameShape(QFrame::StyledPanel);
        layout = new QVBoxLayout(verticalFrame);
        layout->setSpacing(1);
        layout->setObjectName(QString::fromUtf8("layout"));
        layout->setSizeConstraint(QLayout::SetMinimumSize);
        layout->setContentsMargins(-1, 2, -1, -1);
        SystemResouces = new QTabWidget(verticalFrame);
        SystemResouces->setObjectName(QString::fromUtf8("SystemResouces"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        label_3 = new QLabel(tab);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(20, 260, 701, 17));
        layoutWidget = new QWidget(tab);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(20, 170, 381, 27));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_5 = new QLabel(layoutWidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        horizontalLayout->addWidget(label_5);

        cbox_lang = new QComboBox(layoutWidget);
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->setObjectName(QString::fromUtf8("cbox_lang"));
        cbox_lang->setAutoFillBackground(false);
        cbox_lang->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout->addWidget(cbox_lang);

        layoutWidget1 = new QWidget(tab);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(20, 10, 711, 112));
        verticalLayout = new QVBoxLayout(layoutWidget1);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        checkBox_disableSeekBar = new QCheckBox(layoutWidget1);
        checkBox_disableSeekBar->setObjectName(QString::fromUtf8("checkBox_disableSeekBar"));

        verticalLayout->addWidget(checkBox_disableSeekBar);

        checkBox_disableVolume = new QCheckBox(layoutWidget1);
        checkBox_disableVolume->setObjectName(QString::fromUtf8("checkBox_disableVolume"));

        verticalLayout->addWidget(checkBox_disableVolume);

        checkBox_Normalize_Soft = new QCheckBox(layoutWidget1);
        checkBox_Normalize_Soft->setObjectName(QString::fromUtf8("checkBox_Normalize_Soft"));

        verticalLayout->addWidget(checkBox_Normalize_Soft);

        checkBox_fullScreen = new QCheckBox(layoutWidget1);
        checkBox_fullScreen->setObjectName(QString::fromUtf8("checkBox_fullScreen"));

        verticalLayout->addWidget(checkBox_fullScreen);

        timeEdit_autoStartTime = new QTimeEdit(tab);
        timeEdit_autoStartTime->setObjectName(QString::fromUtf8("timeEdit_autoStartTime"));
        timeEdit_autoStartTime->setGeometry(QRect(370, 130, 118, 24));
        timeEdit_autoStopTime = new QTimeEdit(tab);
        timeEdit_autoStopTime->setObjectName(QString::fromUtf8("timeEdit_autoStopTime"));
        timeEdit_autoStopTime->setGeometry(QRect(570, 130, 118, 24));
        checkBox_enableStartStopTime = new QCheckBox(tab);
        checkBox_enableStartStopTime->setObjectName(QString::fromUtf8("checkBox_enableStartStopTime"));
        checkBox_enableStartStopTime->setGeometry(QRect(20, 130, 281, 21));
        label_23 = new QLabel(tab);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setGeometry(QRect(310, 130, 51, 21));
        label_23->setLayoutDirection(Qt::LeftToRight);
        label_23->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        label_24 = new QLabel(tab);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setGeometry(QRect(500, 130, 61, 21));
        label_24->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SystemResouces->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        label = new QLabel(tab_2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 10, 131, 16));
        txt_selected_db = new QLabel(tab_2);
        txt_selected_db->setObjectName(QString::fromUtf8("txt_selected_db"));
        txt_selected_db->setGeometry(QRect(140, 10, 451, 16));
        txt_selected_db->setWordWrap(false);
        label_2 = new QLabel(tab_2);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(10, 30, 291, 17));
        pushButton_3 = new QPushButton(tab_2);
        pushButton_3->setObjectName(QString::fromUtf8("pushButton_3"));
        pushButton_3->setGeometry(QRect(50, 120, 241, 31));
        label_4 = new QLabel(tab_2);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(30, 80, 251, 31));
        QFont font;
        font.setPointSize(11);
        font.setBold(true);
        font.setWeight(75);
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 4);"));
        f_bt_del_jingles_table = new QPushButton(tab_2);
        f_bt_del_jingles_table->setObjectName(QString::fromUtf8("f_bt_del_jingles_table"));
        f_bt_del_jingles_table->setGeometry(QRect(50, 160, 241, 31));
        f_bt_del_pub_table = new QPushButton(tab_2);
        f_bt_del_pub_table->setObjectName(QString::fromUtf8("f_bt_del_pub_table"));
        f_bt_del_pub_table->setGeometry(QRect(50, 200, 241, 31));
        SystemResouces->addTab(tab_2, QString());
        recording = new QWidget();
        recording->setObjectName(QString::fromUtf8("recording"));
        layoutWidget2 = new QWidget(recording);
        layoutWidget2->setObjectName(QString::fromUtf8("layoutWidget2"));
        layoutWidget2->setGeometry(QRect(100, 30, 531, 27));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget2);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_11 = new QLabel(layoutWidget2);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        horizontalLayout_2->addWidget(label_11);

        cboxRecDev = new QComboBox(layoutWidget2);
        cboxRecDev->setObjectName(QString::fromUtf8("cboxRecDev"));
        cboxRecDev->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_2->addWidget(cboxRecDev);

        layoutWidget3 = new QWidget(recording);
        layoutWidget3->setObjectName(QString::fromUtf8("layoutWidget3"));
        layoutWidget3->setGeometry(QRect(100, 170, 531, 27));
        horizontalLayout_3 = new QHBoxLayout(layoutWidget3);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        label_12 = new QLabel(layoutWidget3);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        horizontalLayout_3->addWidget(label_12);

        txt_savePath = new QLineEdit(layoutWidget3);
        txt_savePath->setObjectName(QString::fromUtf8("txt_savePath"));

        horizontalLayout_3->addWidget(txt_savePath);

        bt_browseSavePath = new QToolButton(layoutWidget3);
        bt_browseSavePath->setObjectName(QString::fromUtf8("bt_browseSavePath"));

        horizontalLayout_3->addWidget(bt_browseSavePath);

        layoutWidget4 = new QWidget(recording);
        layoutWidget4->setObjectName(QString::fromUtf8("layoutWidget4"));
        layoutWidget4->setGeometry(QRect(100, 200, 531, 27));
        horizontalLayout_4 = new QHBoxLayout(layoutWidget4);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_13 = new QLabel(layoutWidget4);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        horizontalLayout_4->addWidget(label_13);

        txt_programsPath = new QLineEdit(layoutWidget4);
        txt_programsPath->setObjectName(QString::fromUtf8("txt_programsPath"));

        horizontalLayout_4->addWidget(txt_programsPath);

        bt_browse_programPath = new QToolButton(layoutWidget4);
        bt_browse_programPath->setObjectName(QString::fromUtf8("bt_browse_programPath"));

        horizontalLayout_4->addWidget(bt_browse_programPath);

        label_16 = new QLabel(recording);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setGeometry(QRect(30, 20, 61, 71));
        label_16->setPixmap(QPixmap(QString::fromUtf8(":/icons/hardinfo.png")));
        layoutWidget5 = new QWidget(recording);
        layoutWidget5->setObjectName(QString::fromUtf8("layoutWidget5"));
        layoutWidget5->setGeometry(QRect(100, 60, 531, 27));
        horizontalLayout_9 = new QHBoxLayout(layoutWidget5);
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        horizontalLayout_9->setContentsMargins(0, 0, 0, 0);
        label_17 = new QLabel(layoutWidget5);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        horizontalLayout_9->addWidget(label_17);

        comboBox_codec = new QComboBox(layoutWidget5);
        comboBox_codec->setObjectName(QString::fromUtf8("comboBox_codec"));
        comboBox_codec->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_9->addWidget(comboBox_codec);

        layoutWidget6 = new QWidget(recording);
        layoutWidget6->setObjectName(QString::fromUtf8("layoutWidget6"));
        layoutWidget6->setGeometry(QRect(100, 90, 531, 27));
        horizontalLayout_10 = new QHBoxLayout(layoutWidget6);
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        horizontalLayout_10->setContentsMargins(0, 0, 0, 0);
        label_18 = new QLabel(layoutWidget6);
        label_18->setObjectName(QString::fromUtf8("label_18"));

        horizontalLayout_10->addWidget(label_18);

        comboBox_container = new QComboBox(layoutWidget6);
        comboBox_container->setObjectName(QString::fromUtf8("comboBox_container"));
        comboBox_container->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_10->addWidget(comboBox_container);

        label_19 = new QLabel(recording);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setGeometry(QRect(30, 160, 51, 51));
        label_19->setPixmap(QPixmap(QString::fromUtf8(":/icons/ic_menu_archive.png")));
        layoutWidget_2 = new QWidget(recording);
        layoutWidget_2->setObjectName(QString::fromUtf8("layoutWidget_2"));
        layoutWidget_2->setGeometry(QRect(100, 230, 531, 27));
        horizontalLayout_11 = new QHBoxLayout(layoutWidget_2);
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        horizontalLayout_11->setContentsMargins(0, 0, 0, 0);
        label_20 = new QLabel(layoutWidget_2);
        label_20->setObjectName(QString::fromUtf8("label_20"));

        horizontalLayout_11->addWidget(label_20);

        txt_musicPath = new QLineEdit(layoutWidget_2);
        txt_musicPath->setObjectName(QString::fromUtf8("txt_musicPath"));

        horizontalLayout_11->addWidget(txt_musicPath);

        bt_browse_musicPath = new QToolButton(layoutWidget_2);
        bt_browse_musicPath->setObjectName(QString::fromUtf8("bt_browse_musicPath"));

        horizontalLayout_11->addWidget(bt_browse_musicPath);

        layoutWidget_3 = new QWidget(recording);
        layoutWidget_3->setObjectName(QString::fromUtf8("layoutWidget_3"));
        layoutWidget_3->setGeometry(QRect(100, 260, 531, 27));
        horizontalLayout_12 = new QHBoxLayout(layoutWidget_3);
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        horizontalLayout_12->setContentsMargins(0, 0, 0, 0);
        label_21 = new QLabel(layoutWidget_3);
        label_21->setObjectName(QString::fromUtf8("label_21"));

        horizontalLayout_12->addWidget(label_21);

        txt_jinglePath = new QLineEdit(layoutWidget_3);
        txt_jinglePath->setObjectName(QString::fromUtf8("txt_jinglePath"));

        horizontalLayout_12->addWidget(txt_jinglePath);

        bt_browse_jinglePath = new QToolButton(layoutWidget_3);
        bt_browse_jinglePath->setObjectName(QString::fromUtf8("bt_browse_jinglePath"));

        horizontalLayout_12->addWidget(bt_browse_jinglePath);

        line = new QFrame(recording);
        line->setObjectName(QString::fromUtf8("line"));
        line->setGeometry(QRect(20, 130, 691, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        SystemResouces->addTab(recording, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QString::fromUtf8("tab_4"));
        cbox_enableNetworking = new QCheckBox(tab_4);
        cbox_enableNetworking->setObjectName(QString::fromUtf8("cbox_enableNetworking"));
        cbox_enableNetworking->setGeometry(QRect(30, 30, 321, 23));
        layoutWidget7 = new QWidget(tab_4);
        layoutWidget7->setObjectName(QString::fromUtf8("layoutWidget7"));
        layoutWidget7->setGeometry(QRect(50, 61, 351, 151));
        formLayout = new QFormLayout(layoutWidget7);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        label_6 = new QLabel(layoutWidget7);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_6);

        txt_server = new QLineEdit(layoutWidget7);
        txt_server->setObjectName(QString::fromUtf8("txt_server"));

        formLayout->setWidget(0, QFormLayout::FieldRole, txt_server);

        label_7 = new QLabel(layoutWidget7);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_7);

        txt_port = new QLineEdit(layoutWidget7);
        txt_port->setObjectName(QString::fromUtf8("txt_port"));

        formLayout->setWidget(1, QFormLayout::FieldRole, txt_port);

        label_8 = new QLabel(layoutWidget7);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_8);

        txt_user = new QLineEdit(layoutWidget7);
        txt_user->setObjectName(QString::fromUtf8("txt_user"));

        formLayout->setWidget(2, QFormLayout::FieldRole, txt_user);

        label_9 = new QLabel(layoutWidget7);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_9);

        txt_password = new QLineEdit(layoutWidget7);
        txt_password->setObjectName(QString::fromUtf8("txt_password"));
        txt_password->setInputMethodHints(Qt::ImhHiddenText|Qt::ImhNoAutoUppercase|Qt::ImhNoPredictiveText|Qt::ImhSensitiveData);
        txt_password->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(3, QFormLayout::FieldRole, txt_password);

        label_10 = new QLabel(layoutWidget7);
        label_10->setObjectName(QString::fromUtf8("label_10"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_10);

        cbox_role = new QComboBox(layoutWidget7);
        cbox_role->addItem(QString());
        cbox_role->addItem(QString());
        cbox_role->setObjectName(QString::fromUtf8("cbox_role"));

        formLayout->setWidget(4, QFormLayout::FieldRole, cbox_role);

        layoutWidget8 = new QWidget(tab_4);
        layoutWidget8->setObjectName(QString::fromUtf8("layoutWidget8"));
        layoutWidget8->setGeometry(QRect(50, 230, 631, 27));
        horizontalLayout_5 = new QHBoxLayout(layoutWidget8);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        label_14 = new QLabel(layoutWidget8);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        horizontalLayout_5->addWidget(label_14);

        txt_FTPlocalTempFolder = new QLineEdit(layoutWidget8);
        txt_FTPlocalTempFolder->setObjectName(QString::fromUtf8("txt_FTPlocalTempFolder"));

        horizontalLayout_5->addWidget(txt_FTPlocalTempFolder);

        bt_browseFTPlocalFolder = new QToolButton(layoutWidget8);
        bt_browseFTPlocalFolder->setObjectName(QString::fromUtf8("bt_browseFTPlocalFolder"));

        horizontalLayout_5->addWidget(bt_browseFTPlocalFolder);

        layoutWidget9 = new QWidget(tab_4);
        layoutWidget9->setObjectName(QString::fromUtf8("layoutWidget9"));
        layoutWidget9->setGeometry(QRect(420, 60, 311, 27));
        horizontalLayout_6 = new QHBoxLayout(layoutWidget9);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        label_15 = new QLabel(layoutWidget9);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        horizontalLayout_6->addWidget(label_15);

        cboxComHour = new QTimeEdit(layoutWidget9);
        cboxComHour->setObjectName(QString::fromUtf8("cboxComHour"));

        horizontalLayout_6->addWidget(cboxComHour);

        layoutWidget10 = new QWidget(tab_4);
        layoutWidget10->setObjectName(QString::fromUtf8("layoutWidget10"));
        layoutWidget10->setGeometry(QRect(50, 263, 631, 27));
        horizontalLayout_13 = new QHBoxLayout(layoutWidget10);
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        horizontalLayout_13->setContentsMargins(0, 0, 0, 0);
        label_22 = new QLabel(layoutWidget10);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        horizontalLayout_13->addWidget(label_22);

        txt_takeOverlocalTempFolder = new QLineEdit(layoutWidget10);
        txt_takeOverlocalTempFolder->setObjectName(QString::fromUtf8("txt_takeOverlocalTempFolder"));

        horizontalLayout_13->addWidget(txt_takeOverlocalTempFolder);

        bt_browseTakeOverlocalFolder = new QToolButton(layoutWidget10);
        bt_browseTakeOverlocalFolder->setObjectName(QString::fromUtf8("bt_browseTakeOverlocalFolder"));

        horizontalLayout_13->addWidget(bt_browseTakeOverlocalFolder);

        SystemResouces->addTab(tab_4, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        txt_terminal = new QPlainTextEdit(tab_3);
        txt_terminal->setObjectName(QString::fromUtf8("txt_terminal"));
        txt_terminal->setGeometry(QRect(10, 74, 721, 201));
        QFont font1;
        font1.setFamily(QString::fromUtf8("Ubuntu"));
        font1.setPointSize(11);
        txt_terminal->setFont(font1);
        txt_terminal->setStyleSheet(QString::fromUtf8("background-color: rgb(59, 59, 59);\n"
"color: rgb(255, 255, 255);"));
        txt_terminal->setReadOnly(true);
        layoutWidget11 = new QWidget(tab_3);
        layoutWidget11->setObjectName(QString::fromUtf8("layoutWidget11"));
        layoutWidget11->setGeometry(QRect(10, 10, 721, 27));
        horizontalLayout_7 = new QHBoxLayout(layoutWidget11);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        bt_uname = new QPushButton(layoutWidget11);
        bt_uname->setObjectName(QString::fromUtf8("bt_uname"));

        horizontalLayout_7->addWidget(bt_uname);

        bt_pwd = new QPushButton(layoutWidget11);
        bt_pwd->setObjectName(QString::fromUtf8("bt_pwd"));

        horizontalLayout_7->addWidget(bt_pwd);

        bt_free = new QPushButton(layoutWidget11);
        bt_free->setObjectName(QString::fromUtf8("bt_free"));

        horizontalLayout_7->addWidget(bt_free);

        bt_df = new QPushButton(layoutWidget11);
        bt_df->setObjectName(QString::fromUtf8("bt_df"));

        horizontalLayout_7->addWidget(bt_df);

        layoutWidget12 = new QWidget(tab_3);
        layoutWidget12->setObjectName(QString::fromUtf8("layoutWidget12"));
        layoutWidget12->setGeometry(QRect(10, 40, 721, 27));
        horizontalLayout_8 = new QHBoxLayout(layoutWidget12);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        horizontalLayout_8->setContentsMargins(0, 0, 0, 0);
        bt_update_youtubedl = new QPushButton(layoutWidget12);
        bt_update_youtubedl->setObjectName(QString::fromUtf8("bt_update_youtubedl"));

        horizontalLayout_8->addWidget(bt_update_youtubedl);

        bt_edit_settings = new QPushButton(layoutWidget12);
        bt_edit_settings->setObjectName(QString::fromUtf8("bt_edit_settings"));

        horizontalLayout_8->addWidget(bt_edit_settings);

        SystemResouces->addTab(tab_3, QString());

        layout->addWidget(SystemResouces);


        gridLayout->addWidget(verticalFrame, 0, 0, 1, 5);

        pushButton = new QPushButton(optionsDialog);
        pushButton->setObjectName(QString::fromUtf8("pushButton"));

        gridLayout->addWidget(pushButton, 1, 4, 1, 1);

        bt_save_settings = new QPushButton(optionsDialog);
        bt_save_settings->setObjectName(QString::fromUtf8("bt_save_settings"));

        gridLayout->addWidget(bt_save_settings, 1, 3, 1, 1);

        pushButton_2 = new QPushButton(optionsDialog);
        pushButton_2->setObjectName(QString::fromUtf8("pushButton_2"));

        gridLayout->addWidget(pushButton_2, 1, 0, 1, 1);


        retranslateUi(optionsDialog);

        SystemResouces->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(optionsDialog);
    } // setupUi

    void retranslateUi(QDialog *optionsDialog)
    {
        optionsDialog->setWindowTitle(QCoreApplication::translate("optionsDialog", "Options", nullptr));
        label_3->setText(QCoreApplication::translate("optionsDialog", "Note: you need to restart XFB for some of the changes to take effect", nullptr));
        label_5->setText(QCoreApplication::translate("optionsDialog", "Language:", nullptr));
        cbox_lang->setItemText(0, QCoreApplication::translate("optionsDialog", "English", nullptr));
        cbox_lang->setItemText(1, QCoreApplication::translate("optionsDialog", "Fran\303\247ais", nullptr));
        cbox_lang->setItemText(2, QCoreApplication::translate("optionsDialog", "Portug\303\273es", nullptr));

#if QT_CONFIG(tooltip)
        checkBox_disableSeekBar->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>This wil disable the track's seek bar so that the user is not able to seek trougth the song</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        checkBox_disableSeekBar->setText(QCoreApplication::translate("optionsDialog", "Disable the Seek Bar", nullptr));
        checkBox_disableVolume->setText(QCoreApplication::translate("optionsDialog", "Disable the Volume Slider", nullptr));
#if QT_CONFIG(tooltip)
        checkBox_Normalize_Soft->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>This will boost the volume of the output softly for low and medium sounds. Will not boost when the volume of the track is already high.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        checkBox_Normalize_Soft->setText(QCoreApplication::translate("optionsDialog", "Enable Soft Live Sound Normalization (experimental)", nullptr));
        checkBox_fullScreen->setText(QCoreApplication::translate("optionsDialog", "Start in FullScreen", nullptr));
#if QT_CONFIG(tooltip)
        checkBox_enableStartStopTime->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>This wil disable the track's seek bar so that the user is not able to seek trougth the song</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        checkBox_enableStartStopTime->setText(QCoreApplication::translate("optionsDialog", "Enable Auto Start / Stop (Server only)", nullptr));
        label_23->setText(QCoreApplication::translate("optionsDialog", "Start:", nullptr));
        label_24->setText(QCoreApplication::translate("optionsDialog", "Stop:", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab), QCoreApplication::translate("optionsDialog", "General", nullptr));
        label->setText(QCoreApplication::translate("optionsDialog", "Selected database:", nullptr));
        txt_selected_db->setText(QCoreApplication::translate("optionsDialog", "[NO DATABASE LOADED]", nullptr));
        label_2->setText(QCoreApplication::translate("optionsDialog", "Database Version: 1.0a jellyfish", nullptr));
        pushButton_3->setText(QCoreApplication::translate("optionsDialog", "Delete Music Table", nullptr));
        label_4->setText(QCoreApplication::translate("optionsDialog", "Careful !", nullptr));
        f_bt_del_jingles_table->setText(QCoreApplication::translate("optionsDialog", "Delete Jingles Table", nullptr));
        f_bt_del_pub_table->setText(QCoreApplication::translate("optionsDialog", "Delete Pub Table", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_2), QCoreApplication::translate("optionsDialog", "Database", nullptr));
        label_11->setText(QCoreApplication::translate("optionsDialog", "Default recording device:", nullptr));
        label_12->setText(QCoreApplication::translate("optionsDialog", "Default recording path:", nullptr));
        bt_browseSavePath->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        label_13->setText(QCoreApplication::translate("optionsDialog", "Default program path:  ", nullptr));
        bt_browse_programPath->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        label_16->setText(QString());
        label_17->setText(QCoreApplication::translate("optionsDialog", "Codec:", nullptr));
        label_18->setText(QCoreApplication::translate("optionsDialog", "Container:", nullptr));
        label_19->setText(QString());
        label_20->setText(QCoreApplication::translate("optionsDialog", "Default music path:       ", nullptr));
        bt_browse_musicPath->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        label_21->setText(QCoreApplication::translate("optionsDialog", "Default Jingle path:        ", nullptr));
        bt_browse_jinglePath->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(recording), QCoreApplication::translate("optionsDialog", "Recording and Paths", nullptr));
        cbox_enableNetworking->setText(QCoreApplication::translate("optionsDialog", "Enable Networking", nullptr));
        label_6->setText(QCoreApplication::translate("optionsDialog", "Server URL:", nullptr));
        label_7->setText(QCoreApplication::translate("optionsDialog", "Port:", nullptr));
        label_8->setText(QCoreApplication::translate("optionsDialog", "User:", nullptr));
        label_9->setText(QCoreApplication::translate("optionsDialog", "Password:", nullptr));
        label_10->setText(QCoreApplication::translate("optionsDialog", "Role:", nullptr));
        cbox_role->setItemText(0, QCoreApplication::translate("optionsDialog", "Client", nullptr));
        cbox_role->setItemText(1, QCoreApplication::translate("optionsDialog", "Server", nullptr));

        label_14->setText(QCoreApplication::translate("optionsDialog", "FTP local temp folder", nullptr));
        bt_browseFTPlocalFolder->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        label_15->setText(QCoreApplication::translate("optionsDialog", "Comunications hour:", nullptr));
        label_22->setText(QCoreApplication::translate("optionsDialog", "TakeOver temp folder:", nullptr));
        bt_browseTakeOverlocalFolder->setText(QCoreApplication::translate("optionsDialog", "...", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_4), QCoreApplication::translate("optionsDialog", "Network", nullptr));
#if QT_CONFIG(tooltip)
        bt_uname->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>Shows operating system information</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        bt_uname->setText(QCoreApplication::translate("optionsDialog", "uname", nullptr));
#if QT_CONFIG(tooltip)
        bt_pwd->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>Print working directory (in console)</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        bt_pwd->setText(QCoreApplication::translate("optionsDialog", "pwd", nullptr));
#if QT_CONFIG(tooltip)
        bt_free->setToolTip(QCoreApplication::translate("optionsDialog", "<html><head/><body><p>Show the amount of free RAM resources (in MB)</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        bt_free->setText(QCoreApplication::translate("optionsDialog", "free", nullptr));
#if QT_CONFIG(tooltip)
        bt_df->setToolTip(QCoreApplication::translate("optionsDialog", "Shows the free/used HD space", nullptr));
#endif // QT_CONFIG(tooltip)
        bt_df->setText(QCoreApplication::translate("optionsDialog", "df", nullptr));
        bt_update_youtubedl->setText(QCoreApplication::translate("optionsDialog", "youtube-dl troubleshoot", nullptr));
        bt_edit_settings->setText(QCoreApplication::translate("optionsDialog", "Manually edit settings.conf", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_3), QCoreApplication::translate("optionsDialog", "System Resources", nullptr));
        pushButton->setText(QCoreApplication::translate("optionsDialog", "Close (and save)", nullptr));
        bt_save_settings->setText(QCoreApplication::translate("optionsDialog", "Save Settings", nullptr));
        pushButton_2->setText(QCoreApplication::translate("optionsDialog", "Close (without saving)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class optionsDialog: public Ui_optionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONSDIALOG_H
