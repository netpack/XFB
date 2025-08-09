/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
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
    QWidget *layoutWidgeto1;
    QHBoxLayout *horizontalLayout;
    QLabel *label_5;
    QComboBox *cbox_lang;
    QWidget *layoutWidgeto2;
    QVBoxLayout *verticalLayout;
    QCheckBox *checkBox_disableSeekBar;
    QCheckBox *checkBox_disableVolume;
    QCheckBox *checkBox_Normalize_Soft;
    QCheckBox *checkBox_fullScreen;
    QCheckBox *checkBox_darkMode;
    QWidget *tab_2;
    QLabel *label;
    QLabel *txt_selected_db;
    QLabel *label_2;
    QPushButton *pushButton_3;
    QLabel *label_4;
    QPushButton *f_bt_del_jingles_table;
    QPushButton *f_bt_del_pub_table;
    QWidget *recording;
    QWidget *layoutWidgeto3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_11;
    QComboBox *cboxRecDev;
    QWidget *layoutWidgeto4;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_12;
    QLineEdit *txt_savePath;
    QToolButton *bt_browseSavePath;
    QWidget *layoutWidgeto5;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_13;
    QLineEdit *txt_programsPath;
    QToolButton *bt_browse_programPath;
    QLabel *label_16;
    QWidget *layoutWidgeto6;
    QHBoxLayout *horizontalLayout_9;
    QLabel *label_17;
    QComboBox *comboBox_codec;
    QWidget *layoutWidgeto7;
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
    QWidget *layoutWidgetoo1;
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
    QWidget *layoutWidgetoo2;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_14;
    QLineEdit *txt_FTPlocalTempFolder;
    QToolButton *bt_browseFTPlocalFolder;
    QWidget *layoutWidgetoo3;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_15;
    QTimeEdit *cboxComHour;
    QWidget *layoutWidgetoo4;
    QHBoxLayout *horizontalLayout_13;
    QLabel *label_22;
    QLineEdit *txt_takeOverlocalTempFolder;
    QToolButton *bt_browseTakeOverlocalFolder;
    QWidget *tab_3;
    QPlainTextEdit *txt_terminal;
    QWidget *layoutWidgetoo5;
    QHBoxLayout *horizontalLayout_7;
    QPushButton *bt_uname;
    QPushButton *bt_pwd;
    QPushButton *bt_free;
    QPushButton *bt_df;
    QWidget *layoutWidgetoo6;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *bt_update_youtubedl;
    QPushButton *bt_edit_settings;
    QPushButton *pushButton;
    QPushButton *bt_save_settings;
    QPushButton *pushButton_2;

    void setupUi(QDialog *optionsDialog)
    {
        if (optionsDialog->objectName().isEmpty())
            optionsDialog->setObjectName("optionsDialog");
        optionsDialog->resize(789, 406);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/48x48.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        optionsDialog->setWindowIcon(icon);
        gridLayout = new QGridLayout(optionsDialog);
        gridLayout->setObjectName("gridLayout");
        verticalFrame = new QFrame(optionsDialog);
        verticalFrame->setObjectName("verticalFrame");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(verticalFrame->sizePolicy().hasHeightForWidth());
        verticalFrame->setSizePolicy(sizePolicy);
        verticalFrame->setMinimumSize(QSize(721, 0));
        verticalFrame->setFrameShape(QFrame::Shape::StyledPanel);
        layout = new QVBoxLayout(verticalFrame);
        layout->setSpacing(1);
        layout->setObjectName("layout");
        layout->setSizeConstraint(QLayout::SizeConstraint::SetMinimumSize);
        layout->setContentsMargins(-1, 2, -1, -1);
        SystemResouces = new QTabWidget(verticalFrame);
        SystemResouces->setObjectName("SystemResouces");
        tab = new QWidget();
        tab->setObjectName("tab");
        label_3 = new QLabel(tab);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(20, 260, 701, 17));
        layoutWidgeto1 = new QWidget(tab);
        layoutWidgeto1->setObjectName("layoutWidgeto1");
        layoutWidgeto1->setGeometry(QRect(20, 140, 381, 34));
        horizontalLayout = new QHBoxLayout(layoutWidgeto1);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        label_5 = new QLabel(layoutWidgeto1);
        label_5->setObjectName("label_5");

        horizontalLayout->addWidget(label_5);

        cbox_lang = new QComboBox(layoutWidgeto1);
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->addItem(QString());
        cbox_lang->setObjectName("cbox_lang");
        cbox_lang->setAutoFillBackground(false);
        cbox_lang->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout->addWidget(cbox_lang);

        layoutWidgeto2 = new QWidget(tab);
        layoutWidgeto2->setObjectName("layoutWidgeto2");
        layoutWidgeto2->setGeometry(QRect(20, 10, 711, 112));
        verticalLayout = new QVBoxLayout(layoutWidgeto2);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        checkBox_disableSeekBar = new QCheckBox(layoutWidgeto2);
        checkBox_disableSeekBar->setObjectName("checkBox_disableSeekBar");

        verticalLayout->addWidget(checkBox_disableSeekBar);

        checkBox_disableVolume = new QCheckBox(layoutWidgeto2);
        checkBox_disableVolume->setObjectName("checkBox_disableVolume");

        verticalLayout->addWidget(checkBox_disableVolume);

        checkBox_Normalize_Soft = new QCheckBox(layoutWidgeto2);
        checkBox_Normalize_Soft->setObjectName("checkBox_Normalize_Soft");

        verticalLayout->addWidget(checkBox_Normalize_Soft);

        checkBox_fullScreen = new QCheckBox(layoutWidgeto2);
        checkBox_fullScreen->setObjectName("checkBox_fullScreen");

        verticalLayout->addWidget(checkBox_fullScreen);

        checkBox_darkMode = new QCheckBox(layoutWidgeto2);
        checkBox_darkMode->setObjectName("checkBox_darkMode");

        verticalLayout->addWidget(checkBox_darkMode);

        SystemResouces->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        label = new QLabel(tab_2);
        label->setObjectName("label");
        label->setGeometry(QRect(10, 10, 131, 16));
        txt_selected_db = new QLabel(tab_2);
        txt_selected_db->setObjectName("txt_selected_db");
        txt_selected_db->setGeometry(QRect(140, 10, 451, 16));
        txt_selected_db->setWordWrap(false);
        label_2 = new QLabel(tab_2);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(10, 30, 291, 17));
        pushButton_3 = new QPushButton(tab_2);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setGeometry(QRect(50, 120, 241, 31));
        label_4 = new QLabel(tab_2);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(30, 80, 251, 31));
        QFont font;
        font.setPointSize(11);
        font.setBold(true);
        label_4->setFont(font);
        label_4->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 4);"));
        f_bt_del_jingles_table = new QPushButton(tab_2);
        f_bt_del_jingles_table->setObjectName("f_bt_del_jingles_table");
        f_bt_del_jingles_table->setGeometry(QRect(50, 160, 241, 31));
        f_bt_del_pub_table = new QPushButton(tab_2);
        f_bt_del_pub_table->setObjectName("f_bt_del_pub_table");
        f_bt_del_pub_table->setGeometry(QRect(50, 200, 241, 31));
        SystemResouces->addTab(tab_2, QString());
        recording = new QWidget();
        recording->setObjectName("recording");
        layoutWidgeto3 = new QWidget(recording);
        layoutWidgeto3->setObjectName("layoutWidgeto3");
        layoutWidgeto3->setGeometry(QRect(100, 30, 531, 34));
        horizontalLayout_2 = new QHBoxLayout(layoutWidgeto3);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        label_11 = new QLabel(layoutWidgeto3);
        label_11->setObjectName("label_11");

        horizontalLayout_2->addWidget(label_11);

        cboxRecDev = new QComboBox(layoutWidgeto3);
        cboxRecDev->setObjectName("cboxRecDev");
        cboxRecDev->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_2->addWidget(cboxRecDev);

        layoutWidgeto4 = new QWidget(recording);
        layoutWidgeto4->setObjectName("layoutWidgeto4");
        layoutWidgeto4->setGeometry(QRect(100, 170, 531, 36));
        horizontalLayout_3 = new QHBoxLayout(layoutWidgeto4);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        label_12 = new QLabel(layoutWidgeto4);
        label_12->setObjectName("label_12");

        horizontalLayout_3->addWidget(label_12);

        txt_savePath = new QLineEdit(layoutWidgeto4);
        txt_savePath->setObjectName("txt_savePath");

        horizontalLayout_3->addWidget(txt_savePath);

        bt_browseSavePath = new QToolButton(layoutWidgeto4);
        bt_browseSavePath->setObjectName("bt_browseSavePath");

        horizontalLayout_3->addWidget(bt_browseSavePath);

        layoutWidgeto5 = new QWidget(recording);
        layoutWidgeto5->setObjectName("layoutWidgeto5");
        layoutWidgeto5->setGeometry(QRect(100, 200, 531, 36));
        horizontalLayout_4 = new QHBoxLayout(layoutWidgeto5);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        label_13 = new QLabel(layoutWidgeto5);
        label_13->setObjectName("label_13");

        horizontalLayout_4->addWidget(label_13);

        txt_programsPath = new QLineEdit(layoutWidgeto5);
        txt_programsPath->setObjectName("txt_programsPath");

        horizontalLayout_4->addWidget(txt_programsPath);

        bt_browse_programPath = new QToolButton(layoutWidgeto5);
        bt_browse_programPath->setObjectName("bt_browse_programPath");

        horizontalLayout_4->addWidget(bt_browse_programPath);

        label_16 = new QLabel(recording);
        label_16->setObjectName("label_16");
        label_16->setGeometry(QRect(30, 20, 61, 71));
        label_16->setPixmap(QPixmap(QString::fromUtf8(":/icons/hardinfo.png")));
        layoutWidgeto6 = new QWidget(recording);
        layoutWidgeto6->setObjectName("layoutWidgeto6");
        layoutWidgeto6->setGeometry(QRect(100, 60, 531, 34));
        horizontalLayout_9 = new QHBoxLayout(layoutWidgeto6);
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        horizontalLayout_9->setContentsMargins(0, 0, 0, 0);
        label_17 = new QLabel(layoutWidgeto6);
        label_17->setObjectName("label_17");

        horizontalLayout_9->addWidget(label_17);

        comboBox_codec = new QComboBox(layoutWidgeto6);
        comboBox_codec->setObjectName("comboBox_codec");
        comboBox_codec->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_9->addWidget(comboBox_codec);

        layoutWidgeto7 = new QWidget(recording);
        layoutWidgeto7->setObjectName("layoutWidgeto7");
        layoutWidgeto7->setGeometry(QRect(100, 90, 531, 34));
        horizontalLayout_10 = new QHBoxLayout(layoutWidgeto7);
        horizontalLayout_10->setObjectName("horizontalLayout_10");
        horizontalLayout_10->setContentsMargins(0, 0, 0, 0);
        label_18 = new QLabel(layoutWidgeto7);
        label_18->setObjectName("label_18");

        horizontalLayout_10->addWidget(label_18);

        comboBox_container = new QComboBox(layoutWidgeto7);
        comboBox_container->setObjectName("comboBox_container");
        comboBox_container->setStyleSheet(QString::fromUtf8("selection-color: rgb(87, 118, 255);"));

        horizontalLayout_10->addWidget(comboBox_container);

        label_19 = new QLabel(recording);
        label_19->setObjectName("label_19");
        label_19->setGeometry(QRect(30, 160, 51, 51));
        label_19->setPixmap(QPixmap(QString::fromUtf8(":/icons/ic_menu_archive.png")));
        layoutWidget_2 = new QWidget(recording);
        layoutWidget_2->setObjectName("layoutWidget_2");
        layoutWidget_2->setGeometry(QRect(100, 230, 531, 36));
        horizontalLayout_11 = new QHBoxLayout(layoutWidget_2);
        horizontalLayout_11->setObjectName("horizontalLayout_11");
        horizontalLayout_11->setContentsMargins(0, 0, 0, 0);
        label_20 = new QLabel(layoutWidget_2);
        label_20->setObjectName("label_20");

        horizontalLayout_11->addWidget(label_20);

        txt_musicPath = new QLineEdit(layoutWidget_2);
        txt_musicPath->setObjectName("txt_musicPath");

        horizontalLayout_11->addWidget(txt_musicPath);

        bt_browse_musicPath = new QToolButton(layoutWidget_2);
        bt_browse_musicPath->setObjectName("bt_browse_musicPath");

        horizontalLayout_11->addWidget(bt_browse_musicPath);

        layoutWidget_3 = new QWidget(recording);
        layoutWidget_3->setObjectName("layoutWidget_3");
        layoutWidget_3->setGeometry(QRect(100, 260, 531, 36));
        horizontalLayout_12 = new QHBoxLayout(layoutWidget_3);
        horizontalLayout_12->setObjectName("horizontalLayout_12");
        horizontalLayout_12->setContentsMargins(0, 0, 0, 0);
        label_21 = new QLabel(layoutWidget_3);
        label_21->setObjectName("label_21");

        horizontalLayout_12->addWidget(label_21);

        txt_jinglePath = new QLineEdit(layoutWidget_3);
        txt_jinglePath->setObjectName("txt_jinglePath");

        horizontalLayout_12->addWidget(txt_jinglePath);

        bt_browse_jinglePath = new QToolButton(layoutWidget_3);
        bt_browse_jinglePath->setObjectName("bt_browse_jinglePath");

        horizontalLayout_12->addWidget(bt_browse_jinglePath);

        line = new QFrame(recording);
        line->setObjectName("line");
        line->setGeometry(QRect(20, 130, 691, 16));
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);
        SystemResouces->addTab(recording, QString());
        tab_4 = new QWidget();
        tab_4->setObjectName("tab_4");
        cbox_enableNetworking = new QCheckBox(tab_4);
        cbox_enableNetworking->setObjectName("cbox_enableNetworking");
        cbox_enableNetworking->setGeometry(QRect(20, 10, 321, 23));
        layoutWidgetoo1 = new QWidget(tab_4);
        layoutWidgetoo1->setObjectName("layoutWidgetoo1");
        layoutWidgetoo1->setGeometry(QRect(40, 41, 351, 186));
        formLayout = new QFormLayout(layoutWidgetoo1);
        formLayout->setObjectName("formLayout");
        formLayout->setContentsMargins(0, 0, 0, 0);
        label_6 = new QLabel(layoutWidgetoo1);
        label_6->setObjectName("label_6");

        formLayout->setWidget(0, QFormLayout::LabelRole, label_6);

        txt_server = new QLineEdit(layoutWidgetoo1);
        txt_server->setObjectName("txt_server");

        formLayout->setWidget(0, QFormLayout::FieldRole, txt_server);

        label_7 = new QLabel(layoutWidgetoo1);
        label_7->setObjectName("label_7");

        formLayout->setWidget(1, QFormLayout::LabelRole, label_7);

        txt_port = new QLineEdit(layoutWidgetoo1);
        txt_port->setObjectName("txt_port");

        formLayout->setWidget(1, QFormLayout::FieldRole, txt_port);

        label_8 = new QLabel(layoutWidgetoo1);
        label_8->setObjectName("label_8");

        formLayout->setWidget(2, QFormLayout::LabelRole, label_8);

        txt_user = new QLineEdit(layoutWidgetoo1);
        txt_user->setObjectName("txt_user");

        formLayout->setWidget(2, QFormLayout::FieldRole, txt_user);

        label_9 = new QLabel(layoutWidgetoo1);
        label_9->setObjectName("label_9");

        formLayout->setWidget(3, QFormLayout::LabelRole, label_9);

        txt_password = new QLineEdit(layoutWidgetoo1);
        txt_password->setObjectName("txt_password");
        txt_password->setInputMethodHints(Qt::InputMethodHint::ImhHiddenText|Qt::InputMethodHint::ImhNoAutoUppercase|Qt::InputMethodHint::ImhNoPredictiveText|Qt::InputMethodHint::ImhSensitiveData);
        txt_password->setEchoMode(QLineEdit::EchoMode::Password);

        formLayout->setWidget(3, QFormLayout::FieldRole, txt_password);

        label_10 = new QLabel(layoutWidgetoo1);
        label_10->setObjectName("label_10");

        formLayout->setWidget(4, QFormLayout::LabelRole, label_10);

        cbox_role = new QComboBox(layoutWidgetoo1);
        cbox_role->addItem(QString());
        cbox_role->addItem(QString());
        cbox_role->setObjectName("cbox_role");

        formLayout->setWidget(4, QFormLayout::FieldRole, cbox_role);

        layoutWidgetoo2 = new QWidget(tab_4);
        layoutWidgetoo2->setObjectName("layoutWidgetoo2");
        layoutWidgetoo2->setGeometry(QRect(40, 197, 631, 36));
        horizontalLayout_5 = new QHBoxLayout(layoutWidgetoo2);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(0, 0, 0, 0);
        label_14 = new QLabel(layoutWidgetoo2);
        label_14->setObjectName("label_14");

        horizontalLayout_5->addWidget(label_14);

        txt_FTPlocalTempFolder = new QLineEdit(layoutWidgetoo2);
        txt_FTPlocalTempFolder->setObjectName("txt_FTPlocalTempFolder");

        horizontalLayout_5->addWidget(txt_FTPlocalTempFolder);

        bt_browseFTPlocalFolder = new QToolButton(layoutWidgetoo2);
        bt_browseFTPlocalFolder->setObjectName("bt_browseFTPlocalFolder");

        horizontalLayout_5->addWidget(bt_browseFTPlocalFolder);

        layoutWidgetoo3 = new QWidget(tab_4);
        layoutWidgetoo3->setObjectName("layoutWidgetoo3");
        layoutWidgetoo3->setGeometry(QRect(410, 40, 311, 34));
        horizontalLayout_6 = new QHBoxLayout(layoutWidgetoo3);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        label_15 = new QLabel(layoutWidgetoo3);
        label_15->setObjectName("label_15");

        horizontalLayout_6->addWidget(label_15);

        cboxComHour = new QTimeEdit(layoutWidgetoo3);
        cboxComHour->setObjectName("cboxComHour");

        horizontalLayout_6->addWidget(cboxComHour);

        layoutWidgetoo4 = new QWidget(tab_4);
        layoutWidgetoo4->setObjectName("layoutWidgetoo4");
        layoutWidgetoo4->setGeometry(QRect(40, 230, 631, 36));
        horizontalLayout_13 = new QHBoxLayout(layoutWidgetoo4);
        horizontalLayout_13->setObjectName("horizontalLayout_13");
        horizontalLayout_13->setContentsMargins(0, 0, 0, 0);
        label_22 = new QLabel(layoutWidgetoo4);
        label_22->setObjectName("label_22");

        horizontalLayout_13->addWidget(label_22);

        txt_takeOverlocalTempFolder = new QLineEdit(layoutWidgetoo4);
        txt_takeOverlocalTempFolder->setObjectName("txt_takeOverlocalTempFolder");

        horizontalLayout_13->addWidget(txt_takeOverlocalTempFolder);

        bt_browseTakeOverlocalFolder = new QToolButton(layoutWidgetoo4);
        bt_browseTakeOverlocalFolder->setObjectName("bt_browseTakeOverlocalFolder");

        horizontalLayout_13->addWidget(bt_browseTakeOverlocalFolder);

        SystemResouces->addTab(tab_4, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        txt_terminal = new QPlainTextEdit(tab_3);
        txt_terminal->setObjectName("txt_terminal");
        txt_terminal->setGeometry(QRect(10, 74, 721, 221));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("Ubuntu")});
        font1.setPointSize(11);
        txt_terminal->setFont(font1);
        txt_terminal->setStyleSheet(QString::fromUtf8("background-color: rgb(59, 59, 59);\n"
"color: rgb(255, 255, 255);"));
        txt_terminal->setReadOnly(true);
        layoutWidgetoo5 = new QWidget(tab_3);
        layoutWidgetoo5->setObjectName("layoutWidgetoo5");
        layoutWidgetoo5->setGeometry(QRect(10, 10, 721, 36));
        horizontalLayout_7 = new QHBoxLayout(layoutWidgetoo5);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        horizontalLayout_7->setContentsMargins(0, 0, 0, 0);
        bt_uname = new QPushButton(layoutWidgetoo5);
        bt_uname->setObjectName("bt_uname");

        horizontalLayout_7->addWidget(bt_uname);

        bt_pwd = new QPushButton(layoutWidgetoo5);
        bt_pwd->setObjectName("bt_pwd");

        horizontalLayout_7->addWidget(bt_pwd);

        bt_free = new QPushButton(layoutWidgetoo5);
        bt_free->setObjectName("bt_free");

        horizontalLayout_7->addWidget(bt_free);

        bt_df = new QPushButton(layoutWidgetoo5);
        bt_df->setObjectName("bt_df");

        horizontalLayout_7->addWidget(bt_df);

        layoutWidgetoo6 = new QWidget(tab_3);
        layoutWidgetoo6->setObjectName("layoutWidgetoo6");
        layoutWidgetoo6->setGeometry(QRect(10, 40, 721, 36));
        horizontalLayout_8 = new QHBoxLayout(layoutWidgetoo6);
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        horizontalLayout_8->setContentsMargins(0, 0, 0, 0);
        bt_update_youtubedl = new QPushButton(layoutWidgetoo6);
        bt_update_youtubedl->setObjectName("bt_update_youtubedl");

        horizontalLayout_8->addWidget(bt_update_youtubedl);

        bt_edit_settings = new QPushButton(layoutWidgetoo6);
        bt_edit_settings->setObjectName("bt_edit_settings");

        horizontalLayout_8->addWidget(bt_edit_settings);

        SystemResouces->addTab(tab_3, QString());

        layout->addWidget(SystemResouces);


        gridLayout->addWidget(verticalFrame, 0, 0, 1, 5);

        pushButton = new QPushButton(optionsDialog);
        pushButton->setObjectName("pushButton");

        gridLayout->addWidget(pushButton, 1, 4, 1, 1);

        bt_save_settings = new QPushButton(optionsDialog);
        bt_save_settings->setObjectName("bt_save_settings");

        gridLayout->addWidget(bt_save_settings, 1, 3, 1, 1);

        pushButton_2 = new QPushButton(optionsDialog);
        pushButton_2->setObjectName("pushButton_2");

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
        checkBox_darkMode->setText(QCoreApplication::translate("optionsDialog", "Dark Mode", nullptr));
        SystemResouces->setTabText(SystemResouces->indexOf(tab), QCoreApplication::translate("optionsDialog", "General", nullptr));
        label->setText(QCoreApplication::translate("optionsDialog", "Selected database:", nullptr));
        txt_selected_db->setText(QCoreApplication::translate("optionsDialog", "[NO DATABASE LOADED]", nullptr));
        label_2->setText(QCoreApplication::translate("optionsDialog", "Database Version: 1.0", nullptr));
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

        label_14->setText(QCoreApplication::translate("optionsDialog", "FTP local temp folder ", nullptr));
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
        bt_update_youtubedl->setText(QCoreApplication::translate("optionsDialog", "Troubleshoot", nullptr));
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
