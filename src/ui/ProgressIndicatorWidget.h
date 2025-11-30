#ifndef PROGRESSINDICATORWIDGET_H
#define PROGRESSINDICATORWIDGET_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMovie>

/**
 * @brief Modern progress indicator widget for long-running operations
 * 
 * This widget provides a user-friendly progress indicator with support for
 * determinate and indeterminate progress, status messages, and cancellation.
 * It's designed to be embedded in the main UI for better user experience.
 * 
 * @since XFB 2.0
 */
class ProgressIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressIndicatorWidget(QWidget* parent = nullptr);
    ~ProgressIndicatorWidget();

    /**
     * @brief Show progress for a determinate operation
     * @param title Operation title
     * @param message Initial status message
     * @param minimum Minimum progress value
     * @param maximum Maximum progress value
     */
    void showProgress(const QString& title, 
                     const QString& message, 
                     int minimum = 0, 
                     int maximum = 100);

    /**
     * @brief Show progress for an indeterminate operation
     * @param title Operation title
     * @param message Status message
     */
    void showIndeterminateProgress(const QString& title, const QString& message);

    /**
     * @brief Update progress value and message
     * @param value Current progress value
     * @param message Optional new status message
     */
    void updateProgress(int value, const QString& message = QString());

    /**
     * @brief Update only the status message
     * @param message New status message
     */
    void updateMessage(const QString& message);

    /**
     * @brief Hide the progress indicator
     */
    void hideProgress();

    /**
     * @brief Check if progress is currently being shown
     * @return true if progress is visible
     */
    bool isProgressVisible() const;

    /**
     * @brief Enable or disable the cancel button
     * @param enabled true to enable cancellation
     */
    void setCancelEnabled(bool enabled);

    /**
     * @brief Set whether to show elapsed time
     * @param show true to show elapsed time
     */
    void setShowElapsedTime(bool show);

    /**
     * @brief Set whether to show estimated remaining time
     * @param show true to show estimated time
     */
    void setShowEstimatedTime(bool show);

    /**
     * @brief Get current progress value
     * @return Current progress value
     */
    int currentValue() const;

    /**
     * @brief Get maximum progress value
     * @return Maximum progress value
     */
    int maximumValue() const;

public slots:
    /**
     * @brief Reset the progress indicator
     */
    void reset();

    /**
     * @brief Set the progress range
     * @param minimum Minimum value
     * @param maximum Maximum value
     */
    void setRange(int minimum, int maximum);

    /**
     * @brief Set the current progress value
     * @param value Progress value
     */
    void setValue(int value);

signals:
    /**
     * @brief Emitted when the user clicks the cancel button
     */
    void cancelRequested();

    /**
     * @brief Emitted when progress is completed (reaches maximum)
     */
    void progressCompleted();

    /**
     * @brief Emitted when progress value changes
     * @param value New progress value
     */
    void valueChanged(int value);

private slots:
    /**
     * @brief Handle cancel button click
     */
    void onCancelClicked();

    /**
     * @brief Update elapsed time display
     */
    void updateElapsedTime();

    /**
     * @brief Update estimated remaining time
     */
    void updateEstimatedTime();

private:
    /**
     * @brief Setup the UI layout
     */
    void setupUI();

    /**
     * @brief Update time displays
     */
    void updateTimeDisplays();

    /**
     * @brief Format time duration for display
     * @param seconds Duration in seconds
     * @return Formatted time string
     */
    QString formatTime(int seconds) const;

    /**
     * @brief Calculate estimated remaining time
     * @return Estimated seconds remaining
     */
    int calculateEstimatedTime() const;

private:
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_titleLayout;
    QHBoxLayout* m_progressLayout;
    QHBoxLayout* m_buttonLayout;
    QHBoxLayout* m_timeLayout;
    
    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QLabel* m_elapsedTimeLabel;
    QLabel* m_estimatedTimeLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_cancelButton;
    
    QTimer* m_elapsedTimer;
    QMovie* m_spinnerMovie;
    QLabel* m_spinnerLabel;
    
    qint64 m_startTime;
    bool m_showElapsedTime;
    bool m_showEstimatedTime;
    bool m_isIndeterminate;
    
    // For time estimation
    QList<QPair<qint64, int>> m_progressHistory; // timestamp, value pairs
    static constexpr int MAX_HISTORY_SIZE = 10;
};

#endif // PROGRESSINDICATORWIDGET_H