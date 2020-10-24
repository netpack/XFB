/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.11.2
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
            optionsDialog->setObjectName(QStringLiteral("optionsDialog"));
        optionsDialog->resize(789, 386);
        QIcon icon;
        icon.addFile(QStringLiteral(":/48x48.png"), QSize(), QIcon::Normal, QIcon::Off);
        optionsDialog->setWindowIcon(icon);
        optionsDialog->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));
        gridLayout = new QGridLayout(optionsDialog);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalFrame = new QFrame(optionsDialog);
        verticalFrame->setObjectName(QStringLiteral("verticalFrame"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(verticalFrame->sizePolicy().hasHeightForWidth());
        verticalFrame->setSizePolicy(sizePolicy);
        verticalFrame->setMinimumSize(QSize(721, 0));
        verticalFrame->setFrameShape(QFrame::StyledPanel);
        layout = new QVBoxLayout(verticalFrame);
        layout->setSpacing(1);
        layout->setObjectName(QStringLiteral("layout"));
        layout->setSizeConstraint(QLayout::SetMinimumSize);
        layout->setContentsMargins(-1, 2, -1, -1);
        SystemResouces = new QTabWidget(verticalFrame);
        SystemResouces->setObjectName(QStringLiteral("SystemResouces"));
        tab = new QWidget();
        tab->setObjectName(QStringLiteral("tab"));
        label_3 = new QLabel(tab);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(20, 260, 701, 17));
        layoutWidget = new QWidget(tab);
        layoutWidget->setObjectName(QStringLiteral("layoutWidget"));
        layoutWidget->setGeometry(QRect(20, 170, 381, 27));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_5 = new QLabel(layoutWidget);
        label_5->setObjectName(QStringLiteral("label_5"));

        horizontalLayout->addWidget(label_5);

        cbox_lang = new QComboBox(layoutWidget);
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->setObjectName(QStringLiteral("cbox_lang"));
        cbox_lang->setAutoFillBackground(false);
        cbox_lang->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));

        horizontalLayout->addWidget(cbox_lang);

        layoutWidget1 = new QWidget(tab);
        layoutWidget1->setObjectName(QStringLiteral("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(20, 10, 711, 112));
        verticalLayout = new QVBoxLayout(layoutWidget1);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        checkBox_disableSeekBar = new QCheckBox(layoutWidget1);
        checkBox_disableSeekBar->setObjectName(QStringLiteral("checkBox_disableSeekBar"));

        verticalLayout->addWidget(checkBox_disableSeekBar);

        checkBox_disableVolume = new QCheckBox(layoutWidget1);
        checkBox_disableVolume->setObjectName(QStringLiteral("checkBox_disableVolume"));

        verticalLayout->addWidget(checkBox_disableVolume);

        checkBox_Normalize_Soft = new QCheckBox(layoutWidget1);
        checkBox_Normalize_Soft->setObjectName(QStringLiteral("checkBox_Normalize_Soft"));

        verticalLayout->addWidget(checkBox_Normalize_Soft);

        checkBox_fullScreen = new QCheckBox(layoutWidget1);
        checkBox_fullScreen->setObjectName(QStringLiteral("checkBox_fullScreen"));

        verticalLayout->addWidget(checkBox_fullScreen);

        timeEdit_autoStartTime = new QTimeEdit(tab);
        timeEdit_autoStartTime->setObjectName(QStringLiteral("timeEdit_autoStartTime"));
        timeEdit_autoStartTime->setGeometry(QRect(370, 130, 118, 24));
        timeEdit_autoStopTime = new QTimeEdit(tab);
        timeEdit_autoStopTime->setObjectName(QStringLiteral("timeEdit_autoStopTime"));
        timeEdit_autoStopTime->setGeometry(QRect(570, 130, 118, 24));
        checkBox_enableStartStopTime = new QCheckBox(tab);
        checkBox_enableStartStopTime->setObjectName(QStringLiteral("checkBox_enableStartStopTime"));
        checkBox_enableStartStopTime->setGeometry(QRect(20, 130, 281, 21));
        label_23 = new QLabel(tab);
        label_23->setObjectName(QStringLiteral("label_23"));
        label_23->setGeometry(QRect(310, 130, 51, 21));
        label_23->setLayoutDirection(Qt::LeftToRight);
        label_23->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        label_24 = new QLabel(tab);
        label_24->setObjectName(QStringLiteral("label_24"));
        label_24->setGeometry(QRect(500, 130, 61, 21));
        label_24->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        SystemResouces->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QStringLiteral("tab_2"));
        label = new QLabel(tab_2);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(10, 10, 131, 16));
        txt_selected_db = new QLabel(tab_2);
        txt_selected_db->setObjectName(QStringLiteral("txt_selected_db"));
        txt_selected_db->setGeometry(QRect(140, 10, 451, 16));
        txt_selected_db->setWordWrap(false);
        label_2 = new QLabel(tab_2);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(10, 30, 291, 17));
        pushButton_3 = new QPushButton(tab_2);
        pushButton_3->setObjectName(QStringLiteral("pushButton_3"));
        pushButton_3->setGeometry(QRect(50, 120, 241, 31));
        label_4 = new QLabel(tab_2);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setGeometry(QRect(30, 80, 251, 31));
        QFont font;
        font.setPointSize(11);
        font.setBold(true);
        font.setWeight(75);
        label_4->setFont(font);
        label_4->setStyleSheet(QStringLiteral("color: rgb(255, 0, 4);"));
        f_bt_del_jingles_table = new QPushButton(tab_2);
        f_bt_del_jingles_table->setObjectName(QStringLiteral("f_bt_del_jingles_table"));
        f_bt_del_jingles_table->setGeometry(QRect(50, 160, 241, 31));
        f_bt_del_pub_table = new QPushButton(tab_2);
        f_bt_del_pub_table->setObjectName(QStringLiteral("f_bt_del_pub_table"));
        f_bt_del_pub_table->setGeometry(QRect(50, 200, 241, 31));
        SystemResouces->addTab(tab_2, QString());
        recording = new QWidget();
        recording->setObjectName(QStringLiteral("recording"));
        layoutWidget2 = new QWidget(recording);
        layoutWidget2->setObjectName(QStringLiteral("layoutWidget2"));
        layoutWidget2->setGeometry(QRect(100, 30, 531, 27));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget2);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_11 = new QLabel(layoutWidget2);
        label_11->setObjectName(QStringLiteral("label_11"));

        horizontalLayout_2->addWidget(label_11);

        cboxRecDev = new QComboBox(layoutWidget2);
        cboxRecDev->setObjectName(QStringLiteral("cboxRecDev"));
        cboxRecDev->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_2->addWidget(cboxRecDev);

        layoutWidget3 = new QWidget(recording);
        layoutWidget3->setObjectName(QStringLiteral("layoutWidget3"));
        layoutWidget3->setGeometry(QRect(100, 170, 531, 27));
        horizontalLayout_3 = new QHBoxLayout(layoutWidget3);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        label_12 = new QLabel(layoutWidget3);
        label_12->setObjectName(QStringLiteral("label_12"));

        horizontalLayout_3->addWidget(label_12);

        txt_savePath = new QLineEdit(layoutWidget3);
        txt_savePath->setObjectName(QStringLiteral("txt_savePath"));

        horizontalLayout_3->addWidget(txt_savePath);

        bt_browseSavePath = new QToolButton(layoutWidget3);
        bt_browseSavePath->setObjectName(QStringLiteral("bt_browseSavePath"));

        horizontalLayout_3->addWidget(bt_browseSavePath);

        layoutWidget4 = new QWidget(recording);
        layoutWidget4->setObjectName(QStringLiteral("layoutWidget4"));
        layoutWidget4->setGeometry(QRect(100, 200, 531, 27));
        horizontalLayout_4 = new QHBoxLayout(layoutWidget4);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_13 = new QLabel(layoutWidget4);
        label_13->setObjectName(QStringLiteral("label_13"));

        horizontalLayout_4->addWidget(label_13);

        txt_programsPath = new QLineEdit(layoutWidget4);
        txt_programsPath->setObjectName(QStringLiteral("txt_programsPath"));

        horizontalLayout_4->addWidget(txt_programsPath);

        bt_browse_programPath = new QToolButton(layoutWidget4);
        bt_browse_programPath->setObjectName(QStringLiteral("bt_browse_programPath"));

        horizontalLayout_4->addWidget(bt_browse_programPath);

        label_16 = new QLabel(recording);
        label_16->setObjectName(QStringLiteral("label_16"));
        label_16->setGeometry(QRect(30, 20, 61, 71));
        label_16->setPixmap(QPixmap(QString::fromUtf8(":/icons/hardinfo.png")));
        layoutWidget5 = new QWidget(recording);
        layoutWidget5->setObjectName(QStringLiteral("layoutWidget5"));
        layoutWidget5->setGeometry(QRect(100, 60, 531, 27));
        horizontalLayout_9 = new QHBoxLayout(layoutWidget5);
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        horizontalLayout_9->setContentsMargins(0, 0, 0, 0);
        label_17 = new QLabel(layoutWidget5);
        label_17->setObjectName(QStringLiteral("label_17"));

        horizontalLayout_9->addWidget(label_17);

        comboBox_codec = new QComboBox(layoutWidget5);
        comboBox_codec->setObjectName(QStringLiteral("comboBox_codec"));
        comboBox_codec->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_9->addWidget(comboBox_codec);

        layoutWidget6 = new QWidget(recording);
        layoutWidget6->setObjectName(QStringLiteral("layoutWidget6"));
        layoutWidget6->setGeometry(QRect(100, 90, 531, 27));
        horizontalLayout_10 = new QHBoxLayout(layoutWidget6);
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        horizontalLayout_10->setContentsMargins(0, 0, 0, 0);
        label_18 = new QLabel(layoutWidget6);
        label_18->setObjectName(QStringLiteral("label_18"));

        horizontalLayout_10->addWidget(label_18);

        comboBox_container = new QComboBox(layoutWidget6);
        comboBox_container->setObjectName(QStringLiteral("comboBox_container"));
        comboBox_container->setStyleSheet(QStringLiteral("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_10->addWidget(comboBox_container);

        label_19 = new QLabel(recording);
        label_19->setObjectName(QStringLiteral("label_19"));
        label_19->setGeometry(QRect(30, 160, 51, 51));
        label_19->setPixmap(QPixmap(QString::fromUtf8(":/icons/ic_menu_archive.png")));
        layoutWidget_2 = new QWidget(recording);
        layoutWidget_2->setObjectName(QStringLiteral("layoutWidget_2"));
        layoutWidget_2->setGeometry(QRect(100, 230, 531, 27));
        horizontalLayout_11 = new QHBoxLayout(layoutWidget_2);
        horizontalLayout_11->setObjectName(QStringLiteral("horizontalLayout_11"));
        horizontalLayout_11->setContentsMargins(0, 0, 0, 0);
        label_20 = new QLabel(layoutWidget_2);
        label_20->setObjectName(QStringLiteral("label_20"));

        horizontalLayout_11->addWidget(label_20);

        txt_musicPath = new QLineEdit(layoutWidget_2);
        txt_musicPath->setObjectName(QStringLiteral("txt_musicPath"));

        horizontalLayout_11->addWidget(txt_musicPath);

        bt_browse_musicPath = new QToolButton(layoutWidget_2);
        bt_browse_musicPath->setObjectName(QStringLiteral("bt_browse_musicPath"));

        horizontalLayout_11->addWidget(bt_browse_musicPath);

        layoutWidget_3 = new QWidget(recording);
        layoutWidget_3->setObjectName(QStringLiteral("layoutWidget_3"));
        layoutWidget_3->setGeometry(QRect(100, 260, 531, 27));
        horizontalLayout_12 = new QHBoxLayout(layoutWidget_3);
        horizontalLayout_12->setObjectName(QStringLiteral("horizontalLayout_12"));
        horizontalLayout_12->setContentsMargins(0, 0, 0, 0);
        label_21 = new QLabel(layoutWidget_3);
        label_21->setObjectName(QStringLiteral("label_21"));

        horizontalLayout_12->addWidget(label_21);

        txt_jinglePath = new QLineEdit(layoutWidget_3);
        txt_jinglePath->setObjectName(QStringLiteral("txt_jinglePath"));

        horizontalLayout_12->addWidget(txt_jinglePath);

        bt_browse_jinglePath = new QToolButton(layoutWidget_3);
        bt_browse_jinglePath->setObjectName(QStringLiteral("bt_browse_jinglePath"));

        horizontalLayout_12->addWidget(bt_browse_jinglePath);

        line = new QFrame(recording);
        line->setObjectName(QStringLiteral("line"));
        line->setGeometry(QRect(20, 130, 691, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        SystemResouces->addTab(recording, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName(QStringLiteral("tab_4"));
        cbox_enableNetworking = new QCheckBox(tab_4);
        cbox_enableNetworking->setObjectName(QStringLiteral("cbox_enableNetworking"));
        cbox_enableNetworking->setGeometry(QRect(30, 30, 321, 23));
        layoutWidget7 = new QWidget(tab_4);
        layoutWidget7->setObjectName(QStringLiteral("layoutWidget7"));
        layoutWidget7->setGeometry(QRect(50, 61, 351, 151));
        formLayout = new QFormLayout(layoutWidget7);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        label_6 = new QLabel(layoutWidget7);
        label_6->setObjectName(QStringLiteral("label_6"));

        formLayout->setWidget(0, QFormLayout::LabelRole, label_6);

        txt_server = new QLineEdit(layoutWidget7);
        txt_server->setObjectName(QStringLiteral("txt_server"));

        formLayout->setWidget(0, QFormLayout::FieldRole, txt_server);

        label_7 = new QLabel(layoutWidget7);
        label_7->setObjectName(QStringLiteral("label_7"));

        formLayout->setWidget(1, QFormLayout::LabelRole, label_7);

        txt_port = new QLineEdit(layoutWidget7);
        txt_port->setObjectName(QStringLiteral("txt_port"));

        formLayout->setWidget(1, QFormLayout::FieldRole, txt_port);

        label_8 = new QLabel(layoutWidget7);
        label_8->setObjectName(QStringLiteral("label_8"));

        formLayout->setWidget(2, QFormLayout::LabelRole, label_8);

        txt_user = new QLineEdit(layoutWidget7);
        txt_user->setObjectName(QStringLiteral("txt_user"));

        formLayout->setWidget(2, QFormLayout::FieldRole, txt_user);

        label_9 = new QLabel(layoutWidget7);
        label_9->setObjectName(QStringLiteral("label_9"));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_9);

        txt_password = new QLineEdit(layoutWidget7);
        txt_password->setObjectName(QStringLiteral("txt_password"));
        txt_password->setInputMethodHints(Qt::ImhHiddenText|Qt::ImhNoAutoUppercase|Qt::ImhNoPredictiveText|Qt::ImhSensitiveData);
        txt_password->setEchoMode(QLineEdit::Password);

        formLayout->setWidget(3, QFormLayout::FieldRole, txt_password);

        label_10 = new QLabel(layoutWidget7);
        label_10->setObjectName(QStringLiteral("label_10"));

        formLayout->setWidget(4, QFormLayout::LabelRole, label_10);

        cbox_role = new QComboBox(layoutWidget7);
        cbox_role->addItem(QString());
        cbox_role->addItem(QString());
        cbox_role->setObjectName(QStringLiteral("cbox_role"));

        formLayout->setWidget(4, QFormLayout::FieldRole, cbox_role);

        layoutWidget8 = new QWidget(tab_4);
        layoutWidget8->setObjectName(QStringLiteral("layoutWidget8"));
        layoutWidget8->setGeometry(QRect(50, 230, 631, 27));
        horizontalLayout_5 = new QHBoxLayout(layoutWidget8);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        label_14 = new QLabel(layoutWidget8);
        label_14->setObjectName(QStringLiteral("label_14"));

        horizontalLayout_5->addWidget(label_14);

        txt_FTPlocalTempFolder = new QLineEdit(layoutWidget8);
        txt_FTPlocalTempFolder->setObjectName(QStringLiteral("txt_FTPlocalTempFolder"));

        horizontalLayout_5->addWidget(txt_FTPlocalTempFolder);

        bt_browseFTPlocalFolder = new QToolButton(layoutWidget8);
        bt_browseFTPlocalFolder->setObjectName(QStringLiteral("bt_browseFTPlocalFolder"));

        horizontalLayout_5->addWidget(bt_browseFTPlocalFolder);

        layoutWidget9 = new QWidget(tab_4);
        layoutWidget9->setObjectName(QStringLiteral("layoutWidget9"));
        layoutWidget9->setGeometry(QRect(420, 60, 311, 27));
        horizontalLayout_6 = new QHBoxLayout(layoutWidget9);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        label_15 = new QLabel(layoutWidget9);
        label_15->setObjectName(QStringLiteral("label_15"));

        horizontalLayout_6->addWidget(label_15);

        cboxComHour = new QTimeEdit(layoutWidget9);
        cboxComHour->setObjectName(QStringLiteral("cboxComHour"));

        horizontalLayout_6->addWidget(cboxComHour);

        layoutWidget10 = new QWidget(tab_4);
        layoutWidget10->setObjectName(QStringLiteral("layoutWidget10"));
        layoutWidget10->setGeometry(QRect(50, 263, 631, 27));
        horizontalLayout_13 = new QHBoxLayout(layoutWidget10);
        horizontalLayout_13->setObjectName(QStringLiteral("horizontalLayout_13"));
        horizontalLayout_13->setContentsMargins(0, 0, 0, 0);
        label_22 = new QLabel(layoutWidget10);
        label_22->setObjectName(QStringLiteral("label_22"));

        horizontalLayout_13->addWidget(label_22);

        txt_takeOverlocalTempFolder = new QLineEdit(layoutWidget10);
        txt_takeOverlocalTempFolder->setObjectName(QStringLiteral("txt_takeOverlocalTempFolder"));

        horizontalLayout_13->addWidget(txt_takeOverlocalTempFolder);

        bt_browseTakeOverlocalFolder = new QToolButton(layoutWidget10);
        bt_browseTakeOverlocalFolder->setObjectName(QStringLiteral("bt_browseTakeOverlocalFolder"));

        horizontalLayout_13->addWidget(bt_browseTakeOverlocalFolder);

        SystemResouces->addTab(tab_4, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QStringLiteral("tab_3"));
        txt_terminal = new QPlainTextEdit(tab_3);
        txt_terminal->setObjectName(QStringLiteral("txt_terminal"));
        txt_terminal->setGeometry(QRect(10, 74, 721, 201));
        QFont font1;
        font1.setFamily(QStringLiteral("Ubuntu"));
        font1.setPointSize(11);
        txt_terminal->setFont(font1);
        txt_terminal->setStyleSheet(QLatin1String("background-color: rgb(59, 59, 59);\n"
"color: rgb(255, 255, 255);"));
        txt_terminal->setReadOnly(true);
        layoutWidget11 = new QWidget(tab_3);
        layoutWidget11->setObjectName(QStringLiteral("layoutWidget11"));
        layoutWidget11->setGeometry(QRect(10, 10, 721, 27));
        horizontalLayout_7 = new QHBoxLayout(layoutWidget11);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        bt_uname = new QPushButton(layoutWidget11);
        bt_uname->setObjectName(QStringLiteral("bt_uname"));

        horizontalLayout_7->addWidget(bt_uname);

        bt_pwd = new QPushButton(layoutWidget11);
        bt_pwd->setObjectName(QStringLiteral("bt_pwd"));

        horizontalLayout_7->addWidget(bt_pwd);

        bt_free = new QPushButton(layoutWidget11);
        bt_free->setObjectName(QStringLiteral("bt_free"));

        horizontalLayout_7->addWidget(bt_free);

        bt_df = new QPushButton(layoutWidget11);
        bt_df->setObjectName(QStringLiteral("bt_df"));

        horizontalLayout_7->addWidget(bt_df);

        layoutWidget12 = new QWidget(tab_3);
        layoutWidget12->setObjectName(QStringLiteral("layoutWidget12"));
        layoutWidget12->setGeometry(QRect(10, 40, 721, 27));
        horizontalLayout_8 = new QHBoxLayout(layoutWidget12);
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        horizontalLayout_8->setContentsMargins(0, 0, 0, 0);
        bt_update_youtubedl = new QPushButton(layoutWidget12);
        bt_update_youtubedl->setObjectName(QStringLiteral("bt_update_youtubedl"));

        horizontalLayout_8->addWidget(bt_update_youtubedl);

        bt_edit_settings = new QPushButton(layoutWidget12);
        bt_edit_settings->setObjectName(QStringLiteral("bt_edit_settings"));

        horizontalLayout_8->addWidget(bt_edit_settings);

        SystemResouces->addTab(tab_3, QString());

        layout->addWidget(SystemResouces);


        gridLayout->addWidget(verticalFrame, 0, 0, 1, 5);

        pushButton = new QPushButton(optionsDialog);
        pushButton->setObjectName(QStringLiteral("pushButton"));

        gridLayout->addWidget(pushButton, 1, 4, 1, 1);

        bt_save_settings = new QPushButton(optionsDialog);
        bt_save_settings->setObjectName(QStringLiteral("bt_save_settings"));

        gridLayout->addWidget(bt_save_settings, 1, 3, 1, 1);

        pushButton_2 = new QPushButton(optionsDialog);
        pushButton_2->setObjectName(QStringLiteral("pushButton_2"));

        gridLayout->addWidget(pushButton_2, 1, 0, 1, 1);


        retranslateUi(optionsDialog);

        SystemResouces->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(optionsDialog);
    } // setupUi

    void retranslateUi(QDialog *optionsDialog)
    {
        optionsDialog->setWindowTitle(QApplication::translate("optionsDialog", "Options", nullptr));
        label_3->setText(QApplication::translate("optionsDialog", "Note: you need to restart XFB for some of the changes to take effect", nullptr));
        label_5->setText(QApplication::translate("optionsDialog", "Language:", nullptr));
        cbox_lang->setItemText(0, QApplication::translate("optionsDialog", "English", nullptr));
        cbox_lang->setItemText(1, QApplication::translate("optionsDialog", "Fran\303\247ais", nullptr));
        cbox_lang->setItemText(2, QApplication::translate("optionsDialog", "Portug\303\273es", nullptr));

#ifndef QT_NO_TOOLTIP
        checkBox_disableSeekBar->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>This wil disable the track's seek bar so that the user is not able to seek trougth the song</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        checkBox_disableSeekBar->setText(QApplication::translate("optionsDialog", "Disable the Seek Bar", nullptr));
        checkBox_disableVolume->setText(QApplication::translate("optionsDialog", "Disable the Volume Slider", nullptr));
#ifndef QT_NO_TOOLTIP
        checkBox_Normalize_Soft->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>This will boost the volume of the output softly for low and medium sounds. Will not boost when the volume of the track is already high.</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        checkBox_Normalize_Soft->setText(QApplication::translate("optionsDialog", "Enable Soft Live Sound Normalization (experimental)", nullptr));
        checkBox_fullScreen->setText(QApplication::translate("optionsDialog", "Start in FullScreen", nullptr));
#ifndef QT_NO_TOOLTIP
        checkBox_enableStartStopTime->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>This wil disable the track's seek bar so that the user is not able to seek trougth the song</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        checkBox_enableStartStopTime->setText(QApplication::translate("optionsDialog", "Enable Auto Start / Stop (Server only)", nullptr));
        label_23->setText(QApplication::translate("optionsDialog", "Start:", nullptr));
        label_24->setText(QApplication::translate("optionsDialog", "Stop:", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab), QApplication::translate("optionsDialog", "General", nullptr));
        label->setText(QApplication::translate("optionsDialog", "Selected database:", nullptr));
        txt_selected_db->setText(QApplication::translate("optionsDialog", "[NO DATABASE LOADED]", nullptr));
        label_2->setText(QApplication::translate("optionsDialog", "Database Version: 1.0a jellyfish", nullptr));
        pushButton_3->setText(QApplication::translate("optionsDialog", "Delete Music Table", nullptr));
        label_4->setText(QApplication::translate("optionsDialog", "Careful !", nullptr));
        f_bt_del_jingles_table->setText(QApplication::translate("optionsDialog", "Delete Jingles Table", nullptr));
        f_bt_del_pub_table->setText(QApplication::translate("optionsDialog", "Delete Pub Table", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_2), QApplication::translate("optionsDialog", "Database", nullptr));
        label_11->setText(QApplication::translate("optionsDialog", "Default recording device:", nullptr));
        label_12->setText(QApplication::translate("optionsDialog", "Default recording path:", nullptr));
        bt_browseSavePath->setText(QApplication::translate("optionsDialog", "...", nullptr));
        label_13->setText(QApplication::translate("optionsDialog", "Default program path:  ", nullptr));
        bt_browse_programPath->setText(QApplication::translate("optionsDialog", "...", nullptr));
        label_16->setText(QString());
        label_17->setText(QApplication::translate("optionsDialog", "Codec:", nullptr));
        label_18->setText(QApplication::translate("optionsDialog", "Container:", nullptr));
        label_19->setText(QString());
        label_20->setText(QApplication::translate("optionsDialog", "Default music path:       ", nullptr));
        bt_browse_musicPath->setText(QApplication::translate("optionsDialog", "...", nullptr));
        label_21->setText(QApplication::translate("optionsDialog", "Default Jingle path:        ", nullptr));
        bt_browse_jinglePath->setText(QApplication::translate("optionsDialog", "...", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(recording), QApplication::translate("optionsDialog", "Recording and Paths", nullptr));
        cbox_enableNetworking->setText(QApplication::translate("optionsDialog", "Enable Networking", nullptr));
        label_6->setText(QApplication::translate("optionsDialog", "Server URL:", nullptr));
        label_7->setText(QApplication::translate("optionsDialog", "Port:", nullptr));
        label_8->setText(QApplication::translate("optionsDialog", "User:", nullptr));
        label_9->setText(QApplication::translate("optionsDialog", "Password:", nullptr));
        label_10->setText(QApplication::translate("optionsDialog", "Role:", nullptr));
        cbox_role->setItemText(0, QApplication::translate("optionsDialog", "Client", nullptr));
        cbox_role->setItemText(1, QApplication::translate("optionsDialog", "Server", nullptr));

        label_14->setText(QApplication::translate("optionsDialog", "FTP local temp folder", nullptr));
        bt_browseFTPlocalFolder->setText(QApplication::translate("optionsDialog", "...", nullptr));
        label_15->setText(QApplication::translate("optionsDialog", "Comunications hour:", nullptr));
        label_22->setText(QApplication::translate("optionsDialog", "TakeOver temp folder:", nullptr));
        bt_browseTakeOverlocalFolder->setText(QApplication::translate("optionsDialog", "...", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_4), QApplication::translate("optionsDialog", "Network", nullptr));
#ifndef QT_NO_TOOLTIP
        bt_uname->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>Shows operating system information</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        bt_uname->setText(QApplication::translate("optionsDialog", "uname", nullptr));
#ifndef QT_NO_TOOLTIP
        bt_pwd->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>Print working directory (in console)</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        bt_pwd->setText(QApplication::translate("optionsDialog", "pwd", nullptr));
#ifndef QT_NO_TOOLTIP
        bt_free->setToolTip(QApplication::translate("optionsDialog", "<html><head/><body><p>Show the amount of free RAM resources (in MB)</p></body></html>", nullptr));
#endif // QT_NO_TOOLTIP
        bt_free->setText(QApplication::translate("optionsDialog", "free", nullptr));
#ifndef QT_NO_TOOLTIP
        bt_df->setToolTip(QApplication::translate("optionsDialog", "Shows the free/used HD space", nullptr));
#endif // QT_NO_TOOLTIP
        bt_df->setText(QApplication::translate("optionsDialog", "df", nullptr));
        bt_update_youtubedl->setText(QApplication::translate("optionsDialog", "youtube-dl troubleshoot", nullptr));
        bt_edit_settings->setText(QApplication::translate("optionsDialog", "Manually edit settings.conf", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab_3), QApplication::translate("optionsDialog", "System Resources", nullptr));
        pushButton->setText(QApplication::translate("optionsDialog", "Close (and save)", nullptr));
        bt_save_settings->setText(QApplication::translate("optionsDialog", "Save Settings", nullptr));
        pushButton_2->setText(QApplication::translate("optionsDialog", "Close (without saving)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class optionsDialog: public Ui_optionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONSDIALOG_H
