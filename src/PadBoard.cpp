#include "PadBoard.h"

#include "services/AirLog.h"

#include <QAudioOutput>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMediaPlayer>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <utility>

namespace
{
constexpr int kBankCount = 8;
constexpr int kMinRows = 1;
constexpr int kMaxRows = 8;
constexpr int kMinCols = 1;
constexpr int kMaxCols = 12;

/** Colours offered as one-press swatches in the pad editor. */
const char *const kSwatches[] = {
    "#e14d4d", "#e0872f", "#e3c53d", "#5bb75b", "#3fa9a0",
    "#3d86d6", "#7a5cd6", "#c556a8", "#6e7c8c", "#3a3f47",
};

QString settingsFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + QStringLiteral("/xfb.conf");
}

/** Black or white, whichever stays readable on the given background. */
QColor readableOn(const QColor &c)
{
    const double luma = 0.299 * c.redF() + 0.587 * c.greenF() + 0.114 * c.blueF();
    return luma > 0.55 ? QColor(25, 25, 25) : QColor(245, 245, 245);
}

QString formatMs(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    const qint64 total = ms / 1000;
    return QStringLiteral("%1:%2")
        .arg(total / 60)
        .arg(total % 60, 2, 10, QLatin1Char('0'));
}
} // namespace

// ================================================================= PadButton

PadButton::PadButton(int row, int col, QWidget *parent)
    : QWidget(parent)
    , m_row(row)
    , m_col(col)
{
    setAcceptDrops(true);
    // The board hands exactly one pad the tab stop; until it does, a pad on
    // its own still answers the mouse and a programmatic setFocus().
    setFocusPolicy(Qt::ClickFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setCursor(Qt::PointingHandCursor);
    refreshAccessibility();
}

QSize PadButton::sizeHint() const
{
    return QSize(140, 92);
}

QSize PadButton::minimumSizeHint() const
{
    // Big enough to stay a comfortable finger target on a touch screen.
    return QSize(88, 60);
}

void PadButton::setConfig(const PadConfig &cfg)
{
    const bool sourceChanged = cfg.path != m_cfg.path;
    m_cfg = cfg;
    m_missing = !m_cfg.path.isEmpty() && !QFile::exists(m_cfg.path);

    if (sourceChanged) {
        // A pad that changed file must drop the loaded media, otherwise the
        // next hit would still play the old one.
        if (m_player) {
            m_player->stop();
            m_player->setSource(QUrl());
        }
        m_position = 0;
        m_duration = 0;
    }
    applyConfigToPlayer();
    refreshAccessibility();
    update();
}

void PadButton::setEditMode(bool on)
{
    if (m_editMode == on)
        return;
    m_editMode = on;
    refreshAccessibility();
    update();
}

void PadButton::setMasterVolume(int percent)
{
    m_master = qBound(0, percent, 100);
    applyVolume();
}

void PadButton::setTabStop(bool on)
{
    // StrongFocus puts the pad in the tab chain, ClickFocus keeps it out of
    // it. Either way setFocus() still works, which is all the arrows need.
    setFocusPolicy(on ? Qt::StrongFocus : Qt::ClickFocus);
}

void PadButton::ensurePlayer()
{
    if (m_player)
        return;

    m_output = new QAudioOutput(this);
    m_player = new QMediaPlayer(this);
    m_player->setAudioOutput(m_output);

    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        m_position = pos;
        if (m_airHandle > 0)
            AirLog::instance()->heartbeat(m_airHandle, pos);
        if (m_playing)
            update();
    });
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 dur) {
        m_duration = dur;
        update();
    });
    connect(m_player, &QMediaPlayer::playbackStateChanged, this,
            [this](QMediaPlayer::PlaybackState state) {
        m_playing = (state == QMediaPlayer::PlayingState);
        if (state == QMediaPlayer::StoppedState) {
            // As-run log: a pad that reached its end played out; one the
            // operator hit again, or hit stop on, was cut. m_padStopping is
            // what tells the two apart.
            if (m_airHandle > 0) {
                AirLog::instance()->close(m_airHandle, m_position,
                                          m_padStopping ? QStringLiteral("stopped")
                                                        : QStringLiteral("end"));
                m_airHandle = 0;
            }
            m_position = 0;
        }
        refreshAccessibility();
        update();
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &text) {
        emit message(tr("Pad \"%1\": %2").arg(displayLabel(), text));
    });

    applyConfigToPlayer();
}

void PadButton::applyConfigToPlayer()
{
    if (!m_player)
        return;
    if (!m_cfg.path.isEmpty() && !m_missing) {
        const QUrl wanted = QUrl::fromLocalFile(m_cfg.path);
        if (m_player->source() != wanted)
            m_player->setSource(wanted);
    }
    m_player->setLoops(m_cfg.loop ? QMediaPlayer::Infinite : 1);
    applyVolume();
}

void PadButton::applyVolume()
{
    if (m_output)
        m_output->setVolume(float(m_master) / 100.0f * float(m_cfg.volume) / 100.0f);
}

void PadButton::preload()
{
    if (m_cfg.path.isEmpty())
        return;
    m_missing = !QFile::exists(m_cfg.path);
    if (m_missing) {
        update();
        return;
    }
    ensurePlayer();
}

void PadButton::trigger()
{
    if (m_cfg.path.isEmpty()) {
        emit message(tr("This pad is empty. Turn on \"Edit pads\" or right-click it to assign a file."));
        return;
    }

    m_missing = !QFile::exists(m_cfg.path);
    if (m_missing) {
        emit message(tr("File not found: %1").arg(m_cfg.path));
        update();
        return;
    }

    ensurePlayer();
    if (!m_player)
        return;

    if (m_playing && !m_cfg.restartOnRetrigger) {
        stop();
        return;
    }

    applyConfigToPlayer();
    // stop() rewinds, so a pad hit again always fires from the top — which
    // is what a stab or a stinger has to do. The rewind closes the previous
    // as-run row through the state handler before the new one opens.
    m_padStopping = true;
    m_player->stop();
    m_padStopping = false;
    m_player->play();

    // As-run log: a pad is always the operator's own doing, and the file
    // behind it may well not be in the library at all.
    AirLog::Entry entry = AirLog::entryForPath(m_cfg.path, false);
    if (entry.source == QLatin1String("fallback")) {
        entry.source = QStringLiteral("pad");
        entry.sourceId = -1;
        entry.title = displayLabel();
    }
    if (m_duration > 0)
        entry.plannedMs = m_duration;
    m_airHandle = AirLog::instance()->open(entry);
}

void PadButton::stop()
{
    if (!m_player)
        return;
    m_padStopping = true;
    m_player->stop();
    m_padStopping = false;
}

void PadButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    if (m_editMode)
        emit configureRequested(m_row, m_col);
    else
        trigger();
    event->accept();
}

void PadButton::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_editMode)
            emit configureRequested(m_row, m_col);
        else
            trigger();
        event->accept();
        return;
    case Qt::Key_Escape:
        stop();
        event->accept();
        return;
    case Qt::Key_F2:
        emit configureRequested(m_row, m_col);
        event->accept();
        return;
    // Tabbing through two dozen pads to reach the one below is no way to
    // work; the arrows walk the grid the way it looks, and Tab is left
    // alone so it enters and leaves the whole grid in one press.
    case Qt::Key_Left:
        if (focusNeighbour(m_row, m_col - 1)) { event->accept(); return; }
        break;
    case Qt::Key_Right:
        if (focusNeighbour(m_row, m_col + 1)) { event->accept(); return; }
        break;
    case Qt::Key_Up:
        if (focusNeighbour(m_row - 1, m_col)) { event->accept(); return; }
        break;
    case Qt::Key_Down:
        if (focusNeighbour(m_row + 1, m_col)) { event->accept(); return; }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void PadButton::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    emit focused(m_row, m_col);
}

bool PadButton::focusNeighbour(int row, int col)
{
    QWidget *page = parentWidget();
    if (!page || row < 0 || col < 0)
        return false;

    const QList<PadButton *> pads = page->findChildren<PadButton *>(QString(), Qt::FindDirectChildrenOnly);
    for (PadButton *pad : pads) {
        if (pad->row() == row && pad->col() == col) {
            pad->setFocus(Qt::TabFocusReason);
            return true;
        }
    }
    return false;
}

void PadButton::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    if (!m_cfg.path.isEmpty()) {
        QAction *playAction = menu.addAction(m_playing ? tr("Stop") : tr("Play"));
        connect(playAction, &QAction::triggered, this, [this]() {
            if (m_playing)
                stop();
            else
                trigger();
        });
        menu.addSeparator();
    }

    QAction *edit = menu.addAction(tr("Edit pad..."));
    connect(edit, &QAction::triggered, this, [this]() {
        emit configureRequested(m_row, m_col);
    });

    if (!m_cfg.isEmpty()) {
        QAction *clear = menu.addAction(tr("Clear pad"));
        connect(clear, &QAction::triggered, this, [this]() {
            stop();
            setConfig(PadConfig());
            emit configChanged(m_row, m_col);
        });
    }

    menu.exec(event->globalPos());
    event->accept();
}

void PadButton::enterEvent(QEnterEvent *event)
{
    m_hover = true;
    update();
    QWidget::enterEvent(event);
}

void PadButton::leaveEvent(QEvent *event)
{
    m_hover = false;
    update();
    QWidget::leaveEvent(event);
}

QString PadButton::pathFromMimeData(const QMimeData *mime)
{
    if (!mime)
        return QString();

    // Files dragged in from the desktop or a file manager.
    if (mime->hasUrls()) {
        const QList<QUrl> urls = mime->urls();
        for (const QUrl &url : urls) {
            const QString local = url.toLocalFile();
            if (!local.isEmpty())
                return local;
        }
    }

    // The library views carry the path in a private format. player.cpp
    // builds it two different ways: the music view puts the path in the
    // payload, the jingle/advert/program views put it in the format name.
    static const QString kFormat = QStringLiteral("drag_to_music_playlist");
    if (mime->hasFormat(kFormat)) {
        const QString path = QString::fromUtf8(mime->data(kFormat));
        if (!path.isEmpty())
            return path;
    }
    const QStringList formats = mime->formats();
    for (const QString &format : formats) {
        if (format != kFormat && QFile::exists(format))
            return format;
    }

    if (mime->hasText() && QFile::exists(mime->text()))
        return mime->text();

    return QString();
}

void PadButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (pathFromMimeData(event->mimeData()).isEmpty()) {
        event->ignore();
        return;
    }
    m_dragHover = true;
    update();
    event->acceptProposedAction();
}

void PadButton::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_dragHover = false;
    update();
    QWidget::dragLeaveEvent(event);
}

void PadButton::dropEvent(QDropEvent *event)
{
    m_dragHover = false;
    const QString path = pathFromMimeData(event->mimeData());
    if (path.isEmpty()) {
        update();
        event->ignore();
        return;
    }

    PadConfig cfg = m_cfg;
    cfg.path = path;
    if (cfg.label.isEmpty())
        cfg.label = QFileInfo(path).completeBaseName();
    stop();
    setConfig(cfg);
    emit configChanged(m_row, m_col);
    emit message(tr("Pad loaded: %1").arg(QFileInfo(path).fileName()));
    event->acceptProposedAction();
}

QColor PadButton::baseColor() const
{
    if (m_cfg.color.isValid())
        return m_cfg.color;
    // No colour chosen: follow the theme so pads never clash with it.
    return palette().color(QPalette::Button);
}

QString PadButton::displayLabel() const
{
    if (!m_cfg.label.isEmpty())
        return m_cfg.label;
    if (!m_cfg.path.isEmpty())
        return QFileInfo(m_cfg.path).completeBaseName();
    return QString();
}

void PadButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(2.5, 2.5, -2.5, -2.5);
    const qreal radius = 9.0;
    const bool empty = m_cfg.path.isEmpty() && m_cfg.label.isEmpty();
    const QColor accent = palette().color(QPalette::Highlight);

    QPainterPath shape;
    shape.addRoundedRect(box, radius, radius);

    QColor fill = baseColor();
    if (m_playing)
        fill = fill.lighter(140);
    else if (m_hover)
        fill = fill.lighter(112);

    if (empty) {
        p.fillPath(shape, palette().color(QPalette::Base));
        QPen pen(palette().color(QPalette::Mid));
        pen.setStyle(Qt::DashLine);
        pen.setWidthF(1.5);
        p.setPen(pen);
        p.drawPath(shape);
    } else {
        QLinearGradient gradient(box.topLeft(), box.bottomLeft());
        gradient.setColorAt(0.0, fill.lighter(110));
        gradient.setColorAt(1.0, fill.darker(115));
        p.fillPath(shape, gradient);
        p.setPen(QPen(fill.darker(150), 1.0));
        p.drawPath(shape);
    }

    // Playing / drag-target / focus outlines, in that order of importance.
    if (m_dragHover) {
        p.setPen(QPen(accent, 3.0));
        p.drawPath(shape);
    } else if (m_playing) {
        p.setPen(QPen(QColor(255, 255, 255, 200), 2.5));
        p.drawPath(shape);
    } else if (hasFocus()) {
        QPen pen(accent, 2.0);
        pen.setStyle(Qt::DotLine);
        p.setPen(pen);
        p.drawPath(shape);
    }

    const QColor ink = empty ? palette().color(QPalette::Mid) : readableOn(fill);

    // Progress: the played part of the pad, drawn as a bar along the bottom.
    if (m_playing && m_duration > 0) {
        const double ratio = qBound(0.0, double(m_position) / double(m_duration), 1.0);
        QRectF track(box.left() + 8, box.bottom() - 9, box.width() - 16, 4);
        QPainterPath trackPath;
        trackPath.addRoundedRect(track, 2, 2);
        p.fillPath(trackPath, QColor(ink.red(), ink.green(), ink.blue(), 60));
        QRectF done = track;
        done.setWidth(track.width() * ratio);
        QPainterPath donePath;
        donePath.addRoundedRect(done, 2, 2);
        p.fillPath(donePath, ink);
    }

    QRectF textArea = box.adjusted(7, 6, -7, -6);
    if (m_playing && m_duration > 0)
        textArea.setBottom(textArea.bottom() - 8);

    if (empty) {
        QFont font = p.font();
        font.setPointSizeF(qMax(9.0, font.pointSizeF()));
        p.setFont(font);
        p.setPen(ink);
        p.drawText(textArea, Qt::AlignCenter | Qt::TextWordWrap,
                   m_editMode ? tr("+\nAdd a sound") : tr("Empty"));
        return;
    }

    // Bottom line: how long is left while playing, the file name otherwise.
    QString footer;
    if (m_playing && m_duration > 0)
        footer = QStringLiteral("-%1").arg(formatMs(m_duration - m_position));
    else if (m_missing)
        footer = tr("file missing");
    else if (m_cfg.loop)
        footer = tr("loop");

    QFont footerFont = p.font();
    footerFont.setPointSizeF(qMax(7.5, footerFont.pointSizeF() - 1.5));
    const QFontMetricsF footerMetrics(footerFont);
    QRectF footerRect;
    if (!footer.isEmpty()) {
        footerRect = QRectF(textArea.left(), textArea.bottom() - footerMetrics.height(),
                            textArea.width(), footerMetrics.height());
        textArea.setBottom(footerRect.top() - 1);
    }

    QFont labelFont = p.font();
    labelFont.setBold(true);
    labelFont.setPointSizeF(qMax(8.5, labelFont.pointSizeF() + (height() > 90 ? 1.0 : 0.0)));
    p.setFont(labelFont);
    p.setPen(m_missing ? QColor(220, 70, 70) : ink);
    p.drawText(textArea, Qt::AlignCenter | Qt::TextWordWrap, displayLabel());

    if (!footer.isEmpty()) {
        p.setFont(footerFont);
        p.setPen(QColor(ink.red(), ink.green(), ink.blue(), 190));
        p.drawText(footerRect, Qt::AlignCenter, footer);
    }

    // A pencil in the corner is the only hint edit mode needs — the pad
    // itself must stay readable at arm's length on a touch screen.
    if (m_editMode) {
        p.setFont(footerFont);
        p.setPen(QColor(ink.red(), ink.green(), ink.blue(), 190));
        p.drawText(box.adjusted(0, 3, -6, 0), Qt::AlignTop | Qt::AlignRight,
                   QStringLiteral("✎"));
    }
}

void PadButton::refreshAccessibility()
{
    const QString label = displayLabel();
    const QString position = tr("Pad row %1, column %2").arg(m_row + 1).arg(m_col + 1);

    if (label.isEmpty()) {
        setAccessibleName(tr("%1: empty").arg(position));
        setAccessibleDescription(tr("Press Enter to assign a sound to this pad."));
        setToolTip(tr("Empty pad. Drop a track here, or right-click to assign one."));
        return;
    }

    setAccessibleName(m_playing ? tr("%1: %2, playing").arg(position, label)
                                : tr("%1: %2").arg(position, label));
    setAccessibleDescription(m_editMode
        ? tr("Press Enter to edit this pad. The arrow keys move around the grid.")
        : tr("Press Enter to play, press again to stop. F2 edits the pad, "
             "and the arrow keys move around the grid."));

    QString tip = label;
    if (!m_cfg.path.isEmpty())
        tip += QLatin1Char('\n') + m_cfg.path;
    if (m_missing)
        tip += QLatin1Char('\n') + tr("(the file is missing)");
    setToolTip(tip);
}

// ============================================================= PadEditDialog

PadEditDialog::PadEditDialog(const PadConfig &cfg, QWidget *parent)
    : QDialog(parent)
    , m_cfg(cfg)
{
    setWindowTitle(tr("Pad"));

    auto *form = new QFormLayout;

    m_label = new QLineEdit(m_cfg.label, this);
    m_label->setPlaceholderText(tr("What the pad says on the grid"));
    form->addRow(tr("Label:"), m_label);

    m_path = new QLineEdit(m_cfg.path, this);
    m_path->setPlaceholderText(tr("No file assigned"));
    auto *fileRow = new QHBoxLayout;
    fileRow->addWidget(m_path, 1);
    auto *browse = new QPushButton(tr("File..."), this);
    browse->setToolTip(tr("Pick any audio file on this computer"));
    connect(browse, &QPushButton::clicked, this, &PadEditDialog::pickFile);
    fileRow->addWidget(browse);
    auto *library = new QPushButton(tr("Library..."), this);
    library->setToolTip(tr("Pick a track from the XFB database: music, jingles, adverts or programs"));
    connect(library, &QPushButton::clicked, this, &PadEditDialog::pickFromLibrary);
    fileRow->addWidget(library);
    form->addRow(tr("Sound:"), fileRow);

    m_colorButton = new QPushButton(this);
    m_colorButton->setMinimumWidth(120);
    connect(m_colorButton, &QPushButton::clicked, this, &PadEditDialog::pickColor);
    auto *colorRow = new QHBoxLayout;
    colorRow->addWidget(m_colorButton);
    colorRow->addStretch(1);
    form->addRow(tr("Colour:"), colorRow);

    // The swatches sit on their own row: they are fixed-size, so sharing a row
    // with the colour button squeezed its label the moment the text grew
    // (a translated "Theme colour" no longer fitted).
    auto *swatchRow = new QHBoxLayout;
    for (const char *swatch : kSwatches) {
        const QColor color(swatch);
        auto *button = new QPushButton(this);
        button->setFixedSize(24, 24);
        button->setToolTip(color.name());
        button->setStyleSheet(QStringLiteral("background-color:%1;border:1px solid #555;border-radius:4px;")
                                  .arg(color.name()));
        connect(button, &QPushButton::clicked, this, [this, color]() { setColor(color); });
        swatchRow->addWidget(button);
    }
    auto *noColor = new QPushButton(tr("Theme"), this);
    noColor->setToolTip(tr("Use the colour of the current theme instead of a fixed one"));
    connect(noColor, &QPushButton::clicked, this, [this]() { setColor(QColor()); });
    swatchRow->addWidget(noColor);
    swatchRow->addStretch(1);
    form->addRow(QString(), swatchRow);

    m_loop = new QCheckBox(tr("Loop until stopped"), this);
    m_loop->setChecked(m_cfg.loop);
    m_loop->setToolTip(tr("Keep repeating the sound — for beds under a live link"));
    form->addRow(QString(), m_loop);

    m_retrigger = new QComboBox(this);
    m_retrigger->addItem(tr("Stop it"), false);
    m_retrigger->addItem(tr("Play it again from the start"), true);
    m_retrigger->setCurrentIndex(m_cfg.restartOnRetrigger ? 1 : 0);
    form->addRow(tr("Pressing it while playing:"), m_retrigger);

    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    m_volume->setValue(m_cfg.volume);
    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setMinimumWidth(42);
    auto showVolume = [this](int value) {
        m_volumeLabel->setText(QStringLiteral("%1 %").arg(value));
    };
    showVolume(m_cfg.volume);
    connect(m_volume, &QSlider::valueChanged, this, showVolume);
    auto *volumeRow = new QHBoxLayout;
    volumeRow->addWidget(m_volume, 1);
    volumeRow->addWidget(m_volumeLabel);
    form->addRow(tr("Volume:"), volumeRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    // Qt's own translations are not shipped with XFB, so the standard buttons
    // would stay English in a translated UI. Name them ourselves.
    buttons->button(QDialogButtonBox::Ok)->setText(tr("OK"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    QPushButton *clear = buttons->addButton(tr("Clear pad"), QDialogButtonBox::DestructiveRole);
    connect(clear, &QPushButton::clicked, this, [this]() {
        m_cleared = true;
        accept();
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);

    updateColorButton();
    resize(560, sizeHint().height());
}

void PadEditDialog::pickFile()
{
    const QString start = m_path->text().isEmpty()
                              ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
                              : QFileInfo(m_path->text()).absolutePath();
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Choose a sound for this pad"), start,
        tr("Audio files (*.mp3 *.wav *.ogg *.opus *.flac *.m4a *.aac *.wma *.aiff);;All files (*)"));
    if (file.isEmpty())
        return;
    m_path->setText(file);
    if (m_label->text().isEmpty())
        m_label->setText(QFileInfo(file).completeBaseName());
}

void PadEditDialog::pickFromLibrary()
{
    PadLibraryDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted || dialog.selectedPath().isEmpty())
        return;
    m_path->setText(dialog.selectedPath());
    if (m_label->text().isEmpty())
        m_label->setText(dialog.selectedLabel());
}

void PadEditDialog::pickColor()
{
    const QColor start = m_cfg.color.isValid() ? m_cfg.color : palette().color(QPalette::Button);
    const QColor chosen = QColorDialog::getColor(start, this, tr("Pad colour"));
    if (chosen.isValid())
        setColor(chosen);
}

void PadEditDialog::setColor(const QColor &c)
{
    m_cfg.color = c;
    updateColorButton();
}

void PadEditDialog::updateColorButton()
{
    if (!m_cfg.color.isValid()) {
        m_colorButton->setText(tr("Theme colour"));
        m_colorButton->setStyleSheet(QString());
        return;
    }
    m_colorButton->setText(m_cfg.color.name());
    m_colorButton->setStyleSheet(QStringLiteral("background-color:%1;color:%2;")
                                     .arg(m_cfg.color.name(), readableOn(m_cfg.color).name()));
}

PadConfig PadEditDialog::config() const
{
    PadConfig cfg = m_cfg;
    cfg.label = m_label->text().trimmed();
    cfg.path = m_path->text().trimmed();
    cfg.loop = m_loop->isChecked();
    cfg.volume = m_volume->value();
    cfg.restartOnRetrigger = m_retrigger->currentData().toBool();
    return cfg;
}

// ========================================================== PadLibraryDialog

PadLibraryDialog::PadLibraryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Choose from the library"));

    m_source = new QComboBox(this);
    m_source->addItem(tr("Music"), QStringLiteral("musics"));
    m_source->addItem(tr("Jingles"), QStringLiteral("jingles"));
    m_source->addItem(tr("Adverts"), QStringLiteral("pub"));
    m_source->addItem(tr("Programs"), QStringLiteral("programs"));

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Search..."));
    m_filter->setClearButtonEnabled(true);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("From:"), this));
    top->addWidget(m_source);
    top->addWidget(m_filter, 1);

    m_model = new QSqlQueryModel(this);
    m_view = new QTableView(this);
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(true);

    m_hint = new QLabel(this);
    m_hint->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_okButton->setText(tr("OK"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    m_okButton->setEnabled(false);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        takeCurrentRow();
        if (!m_path.isEmpty())
            accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(top);
    layout->addWidget(m_view, 1);
    layout->addWidget(m_hint);
    layout->addWidget(buttons);

    connect(m_source, &QComboBox::currentIndexChanged, this, [this](int) { reload(); });
    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &) { reload(); });
    connect(m_view, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex &) {
        takeCurrentRow();
        if (!m_path.isEmpty())
            accept();
    });
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) {
        m_okButton->setEnabled(m_view->currentIndex().isValid());
    });

    resize(760, 480);
    reload();
}

void PadLibraryDialog::reload()
{
    QSqlDatabase db = QSqlDatabase::database(QStringLiteral("xfb_connection"));
    if (!db.isValid() || !db.isOpen()) {
        m_model->clear();
        m_hint->setText(tr("The XFB database is not available."));
        return;
    }

    // The table name comes from our own combo box, never from typed text.
    const QString table = m_source->currentData().toString();
    const bool isMusic = (table == QLatin1String("musics"));
    const QString pattern = QStringLiteral("%%%1%%").arg(m_filter->text());

    QSqlQuery query(db);
    if (isMusic) {
        query.prepare(QStringLiteral(
            "SELECT artist, song, path FROM musics "
            "WHERE artist LIKE :f OR song LIKE :f ORDER BY artist, song LIMIT 500"));
    } else {
        query.prepare(QStringLiteral(
            "SELECT name, path FROM %1 "
            "WHERE name LIKE :f OR path LIKE :f ORDER BY name LIMIT 500").arg(table));
    }
    query.bindValue(QStringLiteral(":f"), pattern);

    if (!query.exec()) {
        m_model->clear();
        m_hint->setText(tr("Could not read the library: %1").arg(query.lastError().text()));
        m_okButton->setEnabled(false);
        return;
    }

    m_model->setQuery(std::move(query));
    if (isMusic) {
        m_model->setHeaderData(0, Qt::Horizontal, tr("Artist"));
        m_model->setHeaderData(1, Qt::Horizontal, tr("Song"));
        m_model->setHeaderData(2, Qt::Horizontal, tr("File"));
    } else {
        m_model->setHeaderData(0, Qt::Horizontal, tr("Name"));
        m_model->setHeaderData(1, Qt::Horizontal, tr("File"));
    }
    m_view->resizeColumnsToContents();
    m_okButton->setEnabled(false);

    if (m_model->lastError().isValid())
        m_hint->setText(tr("Could not read the library: %1").arg(m_model->lastError().text()));
    else
        m_hint->setText(tr("%n result(s). Double-click a row to use it.", "", m_model->rowCount()));
}

void PadLibraryDialog::takeCurrentRow()
{
    m_path.clear();
    m_label.clear();

    const QModelIndex current = m_view->currentIndex();
    if (!current.isValid())
        return;

    const int row = current.row();
    const int lastColumn = m_model->columnCount() - 1;
    m_path = m_model->data(m_model->index(row, lastColumn)).toString();

    if (lastColumn == 2) { // music: artist + song
        const QString artist = m_model->data(m_model->index(row, 0)).toString();
        const QString song = m_model->data(m_model->index(row, 1)).toString();
        m_label = artist.isEmpty() ? song : QStringLiteral("%1 - %2").arg(artist, song);
    } else {
        m_label = m_model->data(m_model->index(row, 0)).toString();
    }
    if (m_label.isEmpty() && !m_path.isEmpty())
        m_label = QFileInfo(m_path).completeBaseName();
}

// =========================================================== PadBoardWidget

PadBoardWidget::PadBoardWidget(QWidget *parent)
    : QWidget(parent)
{
    m_bankNames.resize(kBankCount);
    m_bankPads.resize(kBankCount);
    m_bankPreloaded.fill(false, kBankCount);
    m_pads.resize(kBankCount);

    buildUi();
    loadSettings();
    rebuildGrid();
}

void PadBoardWidget::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    // --- Toolbar ---
    auto *bar = new QHBoxLayout;
    bar->setSpacing(8);

    bar->addWidget(new QLabel(tr("Bank:"), this));
    m_bankBox = new QComboBox(this);
    m_bankBox->setMinimumWidth(140);
    m_bankBox->setToolTip(tr("Banks keep separate sets of pads. Pads playing on another bank carry on."));
    bar->addWidget(m_bankBox);

    auto *rename = new QToolButton(this);
    rename->setText(tr("Rename"));
    rename->setToolTip(tr("Rename the current bank"));
    connect(rename, &QToolButton::clicked, this, &PadBoardWidget::renameBank);
    bar->addWidget(rename);

    bar->addSpacing(10);
    bar->addWidget(new QLabel(tr("Grid:"), this));
    m_rowSpin = new QSpinBox(this);
    m_rowSpin->setRange(kMinRows, kMaxRows);
    m_rowSpin->setSuffix(tr(" rows"));
    m_rowSpin->setToolTip(tr("How many rows of pads"));
    bar->addWidget(m_rowSpin);
    m_colSpin = new QSpinBox(this);
    m_colSpin->setRange(kMinCols, kMaxCols);
    m_colSpin->setSuffix(tr(" columns"));
    m_colSpin->setToolTip(tr("How many pads per row"));
    bar->addWidget(m_colSpin);

    bar->addSpacing(10);
    m_editToggle = new QToolButton(this);
    m_editToggle->setText(tr("Edit pads"));
    m_editToggle->setCheckable(true);
    m_editToggle->setToolTip(tr("While this is on, pressing a pad opens its settings instead of "
                                "playing it — the way to set pads up on a touch screen."));
    bar->addWidget(m_editToggle);

    auto *stopAllButton = new QToolButton(this);
    stopAllButton->setText(tr("Stop all"));
    stopAllButton->setToolTip(tr("Stop every pad that is playing, on every bank"));
    connect(stopAllButton, &QToolButton::clicked, this, &PadBoardWidget::stopAll);
    bar->addWidget(stopAllButton);

    bar->addStretch(1);

    bar->addWidget(new QLabel(tr("Volume:"), this));
    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    m_volume->setFixedWidth(140);
    m_volume->setToolTip(tr("Volume of every pad, on top of each pad's own volume"));
    bar->addWidget(m_volume);
    m_volumeLabel = new QLabel(this);
    m_volumeLabel->setMinimumWidth(42);
    bar->addWidget(m_volumeLabel);

    layout->addLayout(bar);

    // --- Pad grid (one page per bank, so banks keep playing) ---
    m_stack = new QStackedWidget(this);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_stack);
    layout->addWidget(scroll, 1);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_status);

    connect(m_bankBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_loading || index < 0)
            return;
        setBank(index);
    });
    connect(m_rowSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (m_loading)
            return;
        m_rows = value;
        rebuildGrid();
        saveSettings();
    });
    connect(m_colSpin, &QSpinBox::valueChanged, this, [this](int value) {
        if (m_loading)
            return;
        m_cols = value;
        rebuildGrid();
        saveSettings();
    });
    connect(m_editToggle, &QToolButton::toggled, this, [this](bool on) {
        for (const QVector<PadButton *> &bank : std::as_const(m_pads)) {
            for (PadButton *pad : bank)
                pad->setEditMode(on);
        }
        setStatus(on ? tr("Edit mode: pressing a pad opens its settings. "
                          "Turn it off to play pads again.")
                     : QString());
    });
    connect(m_volume, &QSlider::valueChanged, this, [this](int value) {
        m_master = value;
        m_volumeLabel->setText(QStringLiteral("%1 %").arg(value));
        for (const QVector<PadButton *> &bank : std::as_const(m_pads)) {
            for (PadButton *pad : bank)
                pad->setMasterVolume(value);
        }
        if (!m_loading)
            saveSettings();
    });

    setAccessibleName(tr("Pads"));
    setAccessibleDescription(tr("A grid of pads that play a sound when pressed."));
}

void PadBoardWidget::rebuildGrid()
{
    // Pads that are playing are about to be destroyed with their page; make
    // sure nothing is left running headless.
    for (const QVector<PadButton *> &bank : std::as_const(m_pads)) {
        for (PadButton *pad : bank)
            pad->stop();
    }

    while (m_stack->count() > 0) {
        QWidget *page = m_stack->widget(0);
        m_stack->removeWidget(page);
        page->deleteLater();
    }
    m_pads.clear();
    m_pads.resize(kBankCount);
    m_bankPreloaded.fill(false, kBankCount);

    // A grid that just shrank can leave the tab stop outside it.
    m_rovingRow = qBound(0, m_rovingRow, m_rows - 1);
    m_rovingCol = qBound(0, m_rovingCol, m_cols - 1);

    for (int bank = 0; bank < kBankCount; ++bank) {
        auto *page = new QWidget(m_stack);
        auto *grid = new QGridLayout(page);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(6);

        for (int row = 0; row < m_rows; ++row) {
            grid->setRowStretch(row, 1);
            for (int col = 0; col < m_cols; ++col) {
                grid->setColumnStretch(col, 1);
                auto *pad = new PadButton(row, col, page);
                pad->setEditMode(m_editToggle->isChecked());
                pad->setMasterVolume(m_master);
                pad->setConfig(m_bankPads[bank].value(cellKey(row, col)));
                // Every bank gets its stop on the same cell, so switching
                // bank leaves Tab where the operator left it.
                pad->setTabStop(row == m_rovingRow && col == m_rovingCol);

                connect(pad, &PadButton::configureRequested, this,
                        [this, bank](int r, int c) { editPad(bank, r, c); });
                connect(pad, &PadButton::configChanged, this,
                        [this, bank](int r, int c) {
                    PadButton *changed = padAt(bank, r, c);
                    if (!changed)
                        return;
                    const PadConfig cfg = changed->config();
                    if (cfg.isEmpty())
                        m_bankPads[bank].remove(cellKey(r, c));
                    else
                        m_bankPads[bank].insert(cellKey(r, c), cfg);
                    saveSettings();
                });
                connect(pad, &PadButton::message, this, &PadBoardWidget::setStatus);
                connect(pad, &PadButton::focused, this, &PadBoardWidget::setRovingCell);

                grid->addWidget(pad, row, col);
                m_pads[bank].append(pad);
            }
        }
        m_stack->addWidget(page);
    }

    m_bank = qBound(0, m_bank, kBankCount - 1);
    m_stack->setCurrentIndex(m_bank);
    if (isVisible())
        preloadBank(m_bank);
}

PadButton *PadBoardWidget::padAt(int bank, int row, int col) const
{
    if (bank < 0 || bank >= m_pads.size())
        return nullptr;
    const int index = row * m_cols + col;
    if (index < 0 || index >= m_pads[bank].size())
        return nullptr;
    return m_pads[bank].at(index);
}

void PadBoardWidget::setRovingCell(int row, int col)
{
    if (row == m_rovingRow && col == m_rovingCol)
        return;
    m_rovingRow = row;
    m_rovingCol = col;
    applyRovingTabStop();
}

void PadBoardWidget::applyRovingTabStop()
{
    // Only one pad of the grid answers Tab: a keyboard user reaches the pads
    // in one press and leaves them in the next, instead of walking all
    // two dozen of them. The arrows are what move around inside the grid.
    for (const QVector<PadButton *> &bank : std::as_const(m_pads)) {
        for (PadButton *pad : bank)
            pad->setTabStop(pad->row() == m_rovingRow && pad->col() == m_rovingCol);
    }
}

void PadBoardWidget::setBank(int bank)
{
    m_bank = qBound(0, bank, kBankCount - 1);
    m_stack->setCurrentIndex(m_bank);
    preloadBank(m_bank);
    if (!m_loading)
        saveSettings();
}

void PadBoardWidget::preloadBank(int bank)
{
    if (bank < 0 || bank >= m_bankPreloaded.size() || m_bankPreloaded[bank])
        return;
    m_bankPreloaded[bank] = true;
    for (PadButton *pad : std::as_const(m_pads[bank]))
        pad->preload();
}

void PadBoardWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    preloadBank(m_bank);
}

void PadBoardWidget::renameBank()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, tr("Rename the bank"), tr("Name of this bank:"), QLineEdit::Normal,
        m_bankNames.value(m_bank), &ok);
    if (!ok)
        return;
    m_bankNames[m_bank] = name.trimmed();
    m_bankBox->setItemText(m_bank, m_bankNames[m_bank].isEmpty()
                                       ? tr("Bank %1").arg(m_bank + 1)
                                       : m_bankNames[m_bank]);
    saveSettings();
}

void PadBoardWidget::editPad(int bank, int row, int col)
{
    PadButton *pad = padAt(bank, row, col);
    if (!pad)
        return;

    PadEditDialog dialog(pad->config(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    pad->stop();
    const PadConfig cfg = dialog.cleared() ? PadConfig() : dialog.config();
    pad->setConfig(cfg);

    if (cfg.isEmpty())
        m_bankPads[bank].remove(cellKey(row, col));
    else
        m_bankPads[bank].insert(cellKey(row, col), cfg);
    saveSettings();
}

void PadBoardWidget::stopAll()
{
    for (const QVector<PadButton *> &bank : std::as_const(m_pads)) {
        for (PadButton *pad : bank)
            pad->stop();
    }
    setStatus(tr("All pads stopped."));
}

void PadBoardWidget::setStatus(const QString &text)
{
    if (!m_status)
        return;
    m_status->setText(text);
    if (text.isEmpty())
        return;
    // Messages are transient; the label goes quiet again on its own so it
    // never shows a stale error next to a working pad.
    QPointer<QLabel> label = m_status;
    const QString shown = text;
    QTimer::singleShot(8000, this, [label, shown]() {
        if (label && label->text() == shown)
            label->clear();
    });
}

void PadBoardWidget::loadSettings()
{
    m_loading = true;

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("PadBoard"));

    m_rows = qBound(kMinRows, settings.value(QStringLiteral("Rows"), 4).toInt(), kMaxRows);
    m_cols = qBound(kMinCols, settings.value(QStringLiteral("Cols"), 6).toInt(), kMaxCols);
    m_master = qBound(0, settings.value(QStringLiteral("Volume"), 100).toInt(), 100);
    m_bank = qBound(0, settings.value(QStringLiteral("Bank"), 0).toInt(), kBankCount - 1);

    for (int bank = 0; bank < kBankCount; ++bank) {
        settings.beginGroup(QStringLiteral("Bank%1").arg(bank));
        m_bankNames[bank] = settings.value(QStringLiteral("Name")).toString();

        const QStringList cells = settings.value(QStringLiteral("Cells")).toStringList();
        for (const QString &cell : cells) {
            const QStringList parts = cell.split(QLatin1Char('_'));
            if (parts.size() != 2)
                continue;
            const int row = parts.at(0).toInt();
            const int col = parts.at(1).toInt();

            PadConfig cfg;
            cfg.label = settings.value(cell + QStringLiteral("_Label")).toString();
            cfg.path = settings.value(cell + QStringLiteral("_Path")).toString();
            const QString colorName = settings.value(cell + QStringLiteral("_Color")).toString();
            if (!colorName.isEmpty())
                cfg.color = QColor(colorName);
            cfg.loop = settings.value(cell + QStringLiteral("_Loop"), false).toBool();
            cfg.volume = qBound(0, settings.value(cell + QStringLiteral("_Volume"), 100).toInt(), 100);
            cfg.restartOnRetrigger = settings.value(cell + QStringLiteral("_Restart"), false).toBool();

            if (!cfg.isEmpty())
                m_bankPads[bank].insert(cellKey(row, col), cfg);
        }
        settings.endGroup();
    }
    settings.endGroup();

    m_bankBox->clear();
    for (int bank = 0; bank < kBankCount; ++bank) {
        m_bankBox->addItem(m_bankNames[bank].isEmpty() ? tr("Bank %1").arg(bank + 1)
                                                       : m_bankNames[bank]);
    }
    m_bankBox->setCurrentIndex(m_bank);
    m_rowSpin->setValue(m_rows);
    m_colSpin->setValue(m_cols);
    m_volume->setValue(m_master);
    m_volumeLabel->setText(QStringLiteral("%1 %").arg(m_master));

    m_loading = false;
}

void PadBoardWidget::saveSettings()
{
    if (m_loading)
        return;

    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("PadBoard"));

    settings.setValue(QStringLiteral("Rows"), m_rows);
    settings.setValue(QStringLiteral("Cols"), m_cols);
    settings.setValue(QStringLiteral("Volume"), m_master);
    settings.setValue(QStringLiteral("Bank"), m_bank);

    for (int bank = 0; bank < kBankCount; ++bank) {
        settings.beginGroup(QStringLiteral("Bank%1").arg(bank));
        // Pads move and get cleared; rewriting the whole bank is the only
        // way to be sure no key of a removed pad survives.
        settings.remove(QString());
        if (!m_bankNames.value(bank).isEmpty())
            settings.setValue(QStringLiteral("Name"), m_bankNames.value(bank));

        QStringList cells;
        for (auto it = m_bankPads[bank].constBegin(); it != m_bankPads[bank].constEnd(); ++it) {
            const int row = int(it.key() >> 16);
            const int col = int(it.key() & 0xffff);
            const QString cell = QStringLiteral("%1_%2").arg(row).arg(col);
            const PadConfig &cfg = it.value();

            cells << cell;
            settings.setValue(cell + QStringLiteral("_Label"), cfg.label);
            settings.setValue(cell + QStringLiteral("_Path"), cfg.path);
            settings.setValue(cell + QStringLiteral("_Color"),
                              cfg.color.isValid() ? cfg.color.name() : QString());
            settings.setValue(cell + QStringLiteral("_Loop"), cfg.loop);
            settings.setValue(cell + QStringLiteral("_Volume"), cfg.volume);
            settings.setValue(cell + QStringLiteral("_Restart"), cfg.restartOnRetrigger);
        }
        // An empty bank writes nothing at all, so xfb.conf stays readable.
        if (!cells.isEmpty()) {
            cells.sort();
            settings.setValue(QStringLiteral("Cells"), cells);
        }
        settings.endGroup();
    }
    settings.endGroup();
}
