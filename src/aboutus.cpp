#include "aboutus.h"
#include "ui_aboutus.h"
#include <QStandardPaths>
#include <QSettings>

aboutUs::aboutUs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutUs)
{
    ui->setupUi(this);

    QString configFileName = "xfb.conf";
    QString writableConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString configFilePath = writableConfigPath + "/" + configFileName;
    QSettings settingsnew(configFilePath, QSettings::IniFormat);
    bool darkMode = settingsnew.value("DarkMode", false).toBool();
    qDebug() << "[StyleFix] OptionsDialog checking dark mode:" << darkMode;


    if (darkMode) {
        this->setStyleSheet(
            "QDialog { background-color: #353535;}"
            "QLabel  { color: #bbbbbb; }");
} else {
        this->setStyleSheet(
            "QDialog { background-color: #ffffff;}"
            "QLabel  {  color: #333333; }");
}

}

aboutUs::~aboutUs()
{
    delete ui;
}
