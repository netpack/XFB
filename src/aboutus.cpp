#include "aboutus.h"
#include "ui_aboutus.h"
#include "donation.h"
#include "QrCode.h"
#include <QCoreApplication>
#include <QImage>
#include <QPixmap>
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
    // The support links come from donation.h for the same reason.
    ui->label->setText(ui->label->text().arg(QCoreApplication::applicationVersion(),
                                             QString::fromLatin1(Donation::kPatreonUrl),
                                             QString::fromLatin1(Donation::kGitHubSponsorsUrl)));
    ui->label->setOpenExternalLinks(true);

    // The QR code is drawn from the Patreon link rather than shipped as an
    // image: the old one was a picture of a PayPal code, and a picture cannot
    // follow the link when it changes.
    const QrCode::Matrix qr = QrCode::encode(QByteArray(Donation::kPatreonUrl));
    if (qr.isValid()) {
        constexpr int quiet = 2; // modules of white border; scanners need some
        const int side = qr.size + 2 * quiet;
        QImage img(side, side, QImage::Format_RGB32);
        img.fill(Qt::white);
        for (int y = 0; y < qr.size; ++y)
            for (int x = 0; x < qr.size; ++x)
                if (qr.at(x, y))
                    img.setPixel(x + quiet, y + quiet, qRgb(0, 0, 0));
        const int target = qMin(ui->label_5->width(), ui->label_5->height());
        const int scale = qMax(1, target / side);
        const qreal dpr = devicePixelRatioF();
        QPixmap pix = QPixmap::fromImage(img.scaled(int(side * scale * dpr), int(side * scale * dpr),
                                                    Qt::IgnoreAspectRatio, Qt::FastTransformation));
        pix.setDevicePixelRatio(dpr);
        ui->label_5->setPixmap(pix);
        ui->label_5->setAlignment(Qt::AlignCenter);
        ui->label_5->setAccessibleName(tr("QR code for XFB on Patreon"));
    }

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
