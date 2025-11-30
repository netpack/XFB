#include "ProgressIndicatorWidget.h"
#include <QApplication>
#include <QStyle>
#include <QDateTime>
#include <QDebug>
#include <algorithm>

ProgressIndicatorWidget::ProgressIndicatorWidget(QWidget* parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_titleLayout(nullptr)
    , m_progressLayout(nullptr)
    , m_buttonLayout(nullptr)
    , m_timeLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_messageLabel(nullptr)
    , m_elapsedTimeLabel(nullptr)
    , m_estimatedTimeLabel(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_elapsedTimer(new QTimer(this))
    , m_spinnerMovie(nullptr)
    , m_spinnerLabel(nullptr)
    , m_startTime(0)
    , m_showElapsedTime(true)
    , m_showEstimatedTime(true)
    , m_isIndeterminate(false)
{
    setupUI();
    
    // Setup elapsed time timer
    m_elapsedTimer->setInterval(1000); // Update every second
    connect(m_elapsedTimer, &QTimer::timeout, this, &ProgressIndicatorWidget::updateElapsedTime);
    
    // Initially hidden
    hide();
}

ProgressIndicatorWidget::~ProgressIndicatorWidget()
{
    if (m_spinnerMovie) {
        m_spinnerMovie->stop();
        delete m_spinnerMovie;
    }
}

void ProgressIndicatorWidget::showProgress(const QString& title, 
                                         const QString& message, 
                                         int minimum, 
                                         int maximum)
{
    m_isIndeterminate = false;
    
    // Setup progress bar for determinate progress
    m_progressBar->setRange(minimum, maximum);
    m_progressBar->setValue(minimum);
    m_progressBar->setVisible(true);
    
    // Hide spinner
    if (m_spinnerLabel) {
        m_spinnerLabel->setVisible(false);
    }
    if (m_spinnerMovie) {
        m_spinnerMovie->stop();
    }
    
    // Set labels
    m_titleLabel->setText(title);
    m_messageLabel->setText(message);
    
    // Reset timing
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_progressHistory.clear();
    m_progressHistory.append({m_startTime, minimum});
    
    // Start elapsed timer
    m_elapsedTimer->start();
    
    // Update time displays
    updateTimeDisplays();
    
    // Show the widget
    show();
    
    qDebug() << "ProgressIndicatorWidget: Showing determinate progress:" << title;
}

void ProgressIndicatorWidget::showIndeterminateProgress(const QString& title, const QString& message)
{
    m_isIndeterminate = true;
    
    // Setup progress bar for indeterminate progress
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(true);
    
    // Show spinner if available
    if (m_spinnerLabel && m_spinnerMovie) {
        m_spinnerLabel->setVisible(true);
        m_spinnerMovie->start();
    }
    
    // Set labels
    m_titleLabel->setText(title);
    m_messageLabel->setText(message);
    
    // Reset timing
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_progressHistory.clear();
    
    // Start elapsed timer
    m_elapsedTimer->start();
    
    // Update time displays (only elapsed time for indeterminate)
    updateTimeDisplays();
    
    // Show the widget
    show();
    
    qDebug() << "ProgressIndicatorWidget: Showing indeterminate progress:" << title;
}

void ProgressIndicatorWidget::updateProgress(int value, const QString& message)
{
    if (!isVisible()) {
        return;
    }
    
    // Update progress bar
    if (!m_isIndeterminate) {
        m_progressBar->setValue(value);
        
        // Record progress for time estimation
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        m_progressHistory.append({currentTime, value});
        
        // Keep only recent history for better estimation
        if (m_progressHistory.size() > MAX_HISTORY_SIZE) {
            m_progressHistory.removeFirst();
        }
        
        // Check if completed
        if (value >= m_progressBar->maximum()) {
            emit progressCompleted();
        }
        
        emit valueChanged(value);
    }
    
    // Update message if provided
    if (!message.isEmpty()) {
        m_messageLabel->setText(message);
    }
    
    // Update time displays
    updateTimeDisplays();
    
    // Process events to keep UI responsive
    QApplication::processEvents();
}

void ProgressIndicatorWidget::updateMessage(const QString& message)
{
    if (isVisible()) {
        m_messageLabel->setText(message);
        QApplication::processEvents();
    }
}

void ProgressIndicatorWidget::hideProgress()
{
    if (isVisible()) {
        m_elapsedTimer->stop();
        
        if (m_spinnerMovie) {
            m_spinnerMovie->stop();
        }
        
        hide();
        
        qDebug() << "ProgressIndicatorWidget: Progress hidden";
    }
}

bool ProgressIndicatorWidget::isProgressVisible() const
{
    return isVisible();
}

void ProgressIndicatorWidget::setCancelEnabled(bool enabled)
{
    if (m_cancelButton) {
        m_cancelButton->setVisible(enabled);
    }
}

void ProgressIndicatorWidget::setShowElapsedTime(bool show)
{
    m_showElapsedTime = show;
    updateTimeDisplays();
}

void ProgressIndicatorWidget::setShowEstimatedTime(bool show)
{
    m_showEstimatedTime = show;
    updateTimeDisplays();
}

int ProgressIndicatorWidget::currentValue() const
{
    return m_progressBar ? m_progressBar->value() : 0;
}

int ProgressIndicatorWidget::maximumValue() const
{
    return m_progressBar ? m_progressBar->maximum() : 100;
}

void ProgressIndicatorWidget::reset()
{
    if (m_progressBar) {
        m_progressBar->reset();
    }
    
    m_titleLabel->clear();
    m_messageLabel->clear();
    
    if (m_elapsedTimeLabel) {
        m_elapsedTimeLabel->clear();
    }
    
    if (m_estimatedTimeLabel) {
        m_estimatedTimeLabel->clear();
    }
    
    m_progressHistory.clear();
    m_startTime = 0;
    
    hideProgress();
}

void ProgressIndicatorWidget::setRange(int minimum, int maximum)
{
    if (m_progressBar) {
        m_progressBar->setRange(minimum, maximum);
        m_isIndeterminate = (minimum == 0 && maximum == 0);
    }
}

void ProgressIndicatorWidget::setValue(int value)
{
    updateProgress(value);
}

void ProgressIndicatorWidget::onCancelClicked()
{
    qDebug() << "ProgressIndicatorWidget: Cancel requested";
    emit cancelRequested();
}

void ProgressIndicatorWidget::updateElapsedTime()
{
    if (m_startTime > 0) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        int elapsedSeconds = static_cast<int>((currentTime - m_startTime) / 1000);
        
        if (m_elapsedTimeLabel && m_showElapsedTime) {
            m_elapsedTimeLabel->setText(tr("Elapsed: %1").arg(formatTime(elapsedSeconds)));
        }
        
        // Update estimated time for determinate progress
        if (!m_isIndeterminate && m_showEstimatedTime) {
            updateEstimatedTime();
        }
    }
}

void ProgressIndicatorWidget::updateEstimatedTime()
{
    if (m_estimatedTimeLabel && !m_isIndeterminate && m_progressHistory.size() >= 2) {
        int estimatedSeconds = calculateEstimatedTime();
        if (estimatedSeconds > 0) {
            m_estimatedTimeLabel->setText(tr("Remaining: %1").arg(formatTime(estimatedSeconds)));
        } else {
            m_estimatedTimeLabel->setText(tr("Remaining: Calculating..."));
        }
    }
}

void ProgressIndicatorWidget::setupUI()
{
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(10, 5, 10, 5);
    m_mainLayout->setSpacing(5);
    
    // Title layout
    m_titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    m_titleLayout->addWidget(m_titleLabel);
    m_titleLayout->addStretch();
    m_mainLayout->addLayout(m_titleLayout);
    
    // Message label
    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet("color: #666666; font-size: 11px;");
    m_mainLayout->addWidget(m_messageLabel);
    
    // Progress layout
    m_progressLayout = new QHBoxLayout();
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimumHeight(20);
    m_progressBar->setTextVisible(true);
    m_progressLayout->addWidget(m_progressBar);
    
    // Spinner for indeterminate progress (optional)
    m_spinnerLabel = new QLabel(this);
    m_spinnerLabel->setFixedSize(20, 20);
    m_spinnerLabel->setVisible(false);
    
    // Try to create a spinner animation
    try {
        // This would need a spinner.gif resource file
        m_spinnerMovie = new QMovie(":/icons/spinner.gif", QByteArray(), this);
        if (m_spinnerMovie->isValid()) {
            m_spinnerLabel->setMovie(m_spinnerMovie);
        } else {
            // Fallback: use a simple text indicator
            delete m_spinnerMovie;
            m_spinnerMovie = nullptr;
            m_spinnerLabel->setText("...");
            m_spinnerLabel->setAlignment(Qt::AlignCenter);
        }
    } catch (...) {
        // Fallback if resource is not available
        m_spinnerMovie = nullptr;
        m_spinnerLabel->setText("...");
        m_spinnerLabel->setAlignment(Qt::AlignCenter);
    }
    
    m_progressLayout->addWidget(m_spinnerLabel);
    m_mainLayout->addLayout(m_progressLayout);
    
    // Time layout
    m_timeLayout = new QHBoxLayout();
    
    m_elapsedTimeLabel = new QLabel(this);
    m_elapsedTimeLabel->setStyleSheet("color: #888888; font-size: 10px;");
    m_timeLayout->addWidget(m_elapsedTimeLabel);
    
    m_timeLayout->addStretch();
    
    m_estimatedTimeLabel = new QLabel(this);
    m_estimatedTimeLabel->setStyleSheet("color: #888888; font-size: 10px;");
    m_timeLayout->addWidget(m_estimatedTimeLabel);
    
    m_mainLayout->addLayout(m_timeLayout);
    
    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_cancelButton->setMaximumWidth(80);
    connect(m_cancelButton, &QPushButton::clicked, this, &ProgressIndicatorWidget::onCancelClicked);
    m_buttonLayout->addWidget(m_cancelButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Set widget properties
    setFixedHeight(120);
    setStyleSheet(
        "ProgressIndicatorWidget {"
        "    background-color: #f0f0f0;"
        "    border: 1px solid #cccccc;"
        "    border-radius: 5px;"
        "}"
    );
}

void ProgressIndicatorWidget::updateTimeDisplays()
{
    if (m_elapsedTimeLabel) {
        m_elapsedTimeLabel->setVisible(m_showElapsedTime);
    }
    
    if (m_estimatedTimeLabel) {
        m_estimatedTimeLabel->setVisible(m_showEstimatedTime && !m_isIndeterminate);
    }
}

QString ProgressIndicatorWidget::formatTime(int seconds) const
{
    if (seconds < 60) {
        return tr("%1s").arg(seconds);
    } else if (seconds < 3600) {
        int minutes = seconds / 60;
        int remainingSeconds = seconds % 60;
        return tr("%1m %2s").arg(minutes).arg(remainingSeconds);
    } else {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        return tr("%1h %2m").arg(hours).arg(minutes);
    }
}

int ProgressIndicatorWidget::calculateEstimatedTime() const
{
    if (m_progressHistory.size() < 2 || m_isIndeterminate) {
        return -1;
    }
    
    // Use linear regression on recent progress data
    const auto& oldest = m_progressHistory.first();
    const auto& newest = m_progressHistory.last();
    
    qint64 timeDiff = newest.first - oldest.first; // milliseconds
    int progressDiff = newest.second - oldest.second;
    
    if (timeDiff <= 0 || progressDiff <= 0) {
        return -1;
    }
    
    int currentValue = newest.second;
    int maxValue = maximumValue();
    int remainingProgress = maxValue - currentValue;
    
    if (remainingProgress <= 0) {
        return 0;
    }
    
    // Calculate rate: progress per millisecond
    double rate = static_cast<double>(progressDiff) / timeDiff;
    
    // Estimate remaining time in milliseconds
    double estimatedMs = remainingProgress / rate;
    
    // Convert to seconds and add some buffer
    int estimatedSeconds = static_cast<int>(estimatedMs / 1000.0) + 1;
    
    return std::max(0, estimatedSeconds);
}