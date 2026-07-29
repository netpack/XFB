#ifndef PADBOARD_H
#define PADBOARD_H

#include <QColor>
#include <QDialog>
#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

class QAudioOutput;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QMediaPlayer;
class QPushButton;
class QSlider;
class QSpinBox;
class QSqlQueryModel;
class QStackedWidget;
class QTableView;
class QToolButton;

/**
 * @brief One pad of the pad board.
 *
 * A pad holds a file, a label and a colour. Everything else is playback
 * behaviour the operator can set per pad, because a station uses the same
 * grid for very different things: a 2-second stab that must restart on
 * every hit, a bed that has to loop under a live link, a jingle that the
 * second press should cut.
 */
struct PadConfig
{
    QString label;
    QString path;
    QColor  color;                      ///< invalid → the theme's default pad colour
    bool    loop = false;
    int     volume = 100;               ///< 0..100, applied on top of the board's master volume
    bool    restartOnRetrigger = false; ///< false: a second press stops the pad

    bool isEmpty() const { return path.isEmpty() && label.isEmpty(); }
};

/**
 * @brief A single, touch-sized pad: label, colour, playback and progress.
 *
 * Each pad owns its own player, so pads are polyphonic — hitting a second
 * pad does not interrupt the first, which is the whole point of a cart
 * wall. The player is created on first use (or when the board is first
 * shown) and then kept, so repeat hits start instantly.
 */
class PadButton : public QWidget
{
    Q_OBJECT

public:
    PadButton(int row, int col, QWidget *parent = nullptr);

    int row() const { return m_row; }
    int col() const { return m_col; }

    const PadConfig &config() const { return m_cfg; }
    void setConfig(const PadConfig &cfg);

    /** In edit mode a press opens the pad's settings instead of playing it. */
    void setEditMode(bool on);
    /** Board-wide volume, 0..100, multiplied with the pad's own volume. */
    void setMasterVolume(int percent);

    /** Load the media now so the first hit does not wait for the decoder. */
    void preload();

    bool isPlaying() const { return m_playing; }

public slots:
    void trigger();
    void stop();

signals:
    void configureRequested(int row, int col);
    void configChanged(int row, int col);
    void message(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void ensurePlayer();
    /** Moves focus to the pad at (row, col) of the same bank, if there is one. */
    bool focusNeighbour(int row, int col);
    void applyVolume();
    void applyConfigToPlayer();
    void refreshAccessibility();
    QColor baseColor() const;
    QString displayLabel() const;
    /** The file path carried by a drag, or an empty string. */
    static QString pathFromMimeData(const class QMimeData *mime);

    int m_row;
    int m_col;
    PadConfig m_cfg;
    bool m_editMode = false;
    bool m_missing = false;
    bool m_dragHover = false;
    bool m_hover = false;
    bool m_playing = false;
    int  m_master = 100;
    qint64 m_position = 0;
    qint64 m_duration = 0;

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_output = nullptr;
};

/**
 * @brief Pad settings: label, file, colour and playback behaviour.
 */
class PadEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PadEditDialog(const PadConfig &cfg, QWidget *parent = nullptr);

    PadConfig config() const;
    /** True when the operator pressed "Clear pad". */
    bool cleared() const { return m_cleared; }

private:
    void pickFile();
    void pickFromLibrary();
    void pickColor();
    void setColor(const QColor &c);
    void updateColorButton();

    PadConfig m_cfg;
    bool m_cleared = false;

    QLineEdit *m_label = nullptr;
    QLineEdit *m_path = nullptr;
    QPushButton *m_colorButton = nullptr;
    QCheckBox *m_loop = nullptr;
    QSlider *m_volume = nullptr;
    QLabel *m_volumeLabel = nullptr;
    QComboBox *m_retrigger = nullptr;
};

/**
 * @brief Picks a track from the XFB database (musics, jingles, adverts,
 *        programs) and returns its path plus a suggested pad label.
 */
class PadLibraryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PadLibraryDialog(QWidget *parent = nullptr);

    QString selectedPath() const { return m_path; }
    QString selectedLabel() const { return m_label; }

private:
    void reload();
    void takeCurrentRow();

    QComboBox *m_source = nullptr;
    QLineEdit *m_filter = nullptr;
    QTableView *m_view = nullptr;
    QSqlQueryModel *m_model = nullptr;
    QLabel *m_hint = nullptr;
    QPushButton *m_okButton = nullptr;

    QString m_path;
    QString m_label;
};

/**
 * @brief The "Pads" tab: a grid of labelled, coloured, instantly playable
 *        pads — the touch-screen cart wall.
 *
 * Pads are organised in banks so a station can keep, say, sweepers on one
 * bank and beds on another. Every bank keeps its own live pads, so a bed
 * looping on one bank carries on while the operator reaches for a stinger
 * on another. Grid size, banks and every pad are persisted in xfb.conf as
 * soon as they change, so nothing is lost on a crash.
 *
 * Pads take files from three places: dragged in from the library views
 * (musics / jingles / adverts / programs), picked from the same library
 * through a search dialog, or chosen anywhere on disk.
 */
class PadBoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PadBoardWidget(QWidget *parent = nullptr);

public slots:
    /** Stops every pad of every bank (the panic button). */
    void stopAll();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void buildUi();
    void rebuildGrid();
    void loadSettings();
    void saveSettings();
    void editPad(int bank, int row, int col);
    void setStatus(const QString &text);
    void setBank(int bank);
    void renameBank();
    void preloadBank(int bank);
    /** The pad that emitted the signal currently being handled. */
    PadButton *padAt(int bank, int row, int col) const;

    /** Pads are stored per (row, column) so resizing the grid keeps them. */
    static quint32 cellKey(int row, int col) { return (quint32(row) << 16) | quint32(col); }

    QComboBox      *m_bankBox = nullptr;
    QSpinBox       *m_rowSpin = nullptr;
    QSpinBox       *m_colSpin = nullptr;
    QToolButton    *m_editToggle = nullptr;
    QSlider        *m_volume = nullptr;
    QLabel         *m_volumeLabel = nullptr;
    QLabel         *m_status = nullptr;
    QStackedWidget *m_stack = nullptr;

    /// [bank][row * m_cols + col]
    QVector<QVector<PadButton *>> m_pads;
    QVector<QString> m_bankNames;
    QVector<QHash<quint32, PadConfig>> m_bankPads;
    QVector<bool> m_bankPreloaded;

    int  m_bank = 0;
    int  m_rows = 4;
    int  m_cols = 6;
    int  m_master = 100;
    bool m_loading = false;
};

#endif // PADBOARD_H
