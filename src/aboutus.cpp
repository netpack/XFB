#include "aboutus.h"
#include "ui_aboutus.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QSettings>

aboutUs::aboutUs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutUs)
{
    ui->setupUi(this);

    // The version used to be typed into the .ui text, so About kept announcing
    // 3.14159 for five releases after that — and the number was baked into the
    // pt/fr translations too, where no version-bump script would ever have
    // found it. The text now carries a %1 that is filled in here, so About
    // follows the build in every language and there is nothing left to bump.
    ui->label->setText(ui->label->text().arg(QCoreApplication::applicationVersion()));

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
