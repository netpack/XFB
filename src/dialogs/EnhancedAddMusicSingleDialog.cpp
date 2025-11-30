#include "EnhancedAddMusicSingleDialog.h"
#include "../repositories/MusicRepository.h"
#include "../services/InputValidator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateEdit>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QApplication>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

// Style sheet constants
const QString EnhancedAddMusicSingleDialog::VALID_FIELD_STYLE = 
    "QLineEdit, QComboBox { border: 2px solid #4CAF50; background-color: #E8F5E8; }";
const QString EnhancedAddMusicSingleDialog::INVALID_FIELD_STYLE = 
    "QLineEdit, QComboBox { border: 2px solid #F44336; background-color: #FFEBEE; }";
const QString EnhancedAddMusicSingleDialog::NEUTRAL_FIELD_STYLE = 
    "QLineEdit, QComboBox { border: 1px solid #CCCCCC; }";

EnhancedAddMusicSingleDialog::EnhancedAddMusicSingleDialog(MusicRepository* repository, QWidget* parent)
    : QDialog(parent)
    , m_repository(repository)
    , m_validator(std::make_unique<InputValidator>())
    , m_validationTimer(new QTimer(this))
    , m_metadataWatcher(new QFutureWatcher<QVariantMap>(this))
    , m_isValidating(false)
    , m_isExtracting(false)
    , m_isAdding(false)
    , m_addedMusicId(0)
    , m_formValid(false)
{
    setWindowTitle(tr("Add Music File"));
    setModal(true);
    resize(600, 500);
    
    setupUI();
    setupValidation();
    setupConnections();
    loadGenres();
    loadCountries();
    
    // Initialize form state
    resetForm();
    
    qDebug() << "EnhancedAddMusicSingleDialog: Initialized";
}

EnhancedAddMusicSingleDialog::~EnhancedAddMusicSingleDialog()
{
    if (m_metadataWatcher && m_metadataWatcher->isRunning()) {
        m_metadataWatcher->cancel();
        m_metadataWatcher->waitForFinished();
    }
}

void EnhancedAddMusicSingleDialog::setFilePath(const QString& filePath)
{
    if (!filePath.isEmpty() && QFileInfo::exists(filePath)) {
        m_filePathEdit->setText(filePath);
        onFilePathChanged();
    }
}

int EnhancedAddMusicSingleDialog::addedMusicId() const
{
    return m_addedMusicId;
}

void EnhancedAddMusicSingleDialog::accept()
{
    if (validateInputs() && addMusic()) {
        QDialog::accept();
    }
}

void EnhancedAddMusicSingleDialog::reject()
{
    if (m_isAdding || m_isExtracting) {
        int ret = QMessageBox::question(this, tr("Cancel Operation"),
                                       tr("An operation is in progress. Are you sure you want to cancel?"),
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            return;
        }
        
        // Cancel ongoing operations
        if (m_metadataWatcher && m_metadataWatcher->isRunning()) {
            m_metadataWatcher->cancel();
        }
    }
    
    QDialog::reject();
}

void EnhancedAddMusicSingleDialog::onBrowseFileClicked()
{
    QStringList extensions = getSupportedExtensions();
    QString filter = tr("Audio Files (%1);;All Files (*.*)").arg(extensions.join(" "));
    
    QString startDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (!m_filePathEdit->text().isEmpty()) {
        QFileInfo info(m_filePathEdit->text());
        if (info.exists()) {
            startDir = info.absolutePath();
        }
    }
    
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select Music File"), startDir, filter);
    
    if (!filePath.isEmpty()) {
        setFilePath(filePath);
    }
}

void EnhancedAddMusicSingleDialog::onFilePathChanged()
{
    QString filePath = m_filePathEdit->text().trimmed();
    
    if (filePath.isEmpty()) {
        m_fileInfoLabel->clear();
        m_extractMetadataButton->setEnabled(false);
        clearFieldError(m_filePathEdit);
        return;
    }
    
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        showFieldError(m_filePathEdit, tr("File does not exist"));
        m_fileInfoLabel->setText(tr("File not found"));
        m_extractMetadataButton->setEnabled(false);
        return;
    }
    
    if (!isSupportedAudioFile(filePath)) {
        showFieldError(m_filePathEdit, tr("Unsupported audio format"));
        m_fileInfoLabel->setText(tr("Unsupported format"));
        m_extractMetadataButton->setEnabled(false);
        return;
    }
    
    // File is valid
    clearFieldError(m_filePathEdit);
    
    // Update file info display
    QString fileSize = formatFileSize(fileInfo.size());
    QString fileFormat = fileInfo.suffix().toUpper();
    m_fileInfoLabel->setText(tr("Size: %1, Format: %2").arg(fileSize, fileFormat));
    
    m_extractMetadataButton->setEnabled(true);
    
    // Auto-extract metadata if enabled
    if (MetadataExtractor::isMetadataExtractionAvailable()) {
        extractMetadata();
    }
    
    // Trigger validation
    onInputChanged();
}

void EnhancedAddMusicSingleDialog::onInputChanged()
{
    // Debounce validation to avoid excessive calls
    m_validationTimer->stop();
    m_validationTimer->start(300); // 300ms delay
}

void EnhancedAddMusicSingleDialog::onManageGenresClicked()
{
    // This would open the genre management dialog
    // For now, just reload genres
    loadGenres();
}

void EnhancedAddMusicSingleDialog::onMetadataExtractionCompleted()
{
    if (!m_metadataWatcher->isFinished()) {
        return;
    }
    
    try {
        QVariantMap metadata = m_metadataWatcher->result();
        applyMetadata(metadata);
        
        hideProgress();
        m_isExtracting = false;
        
        qDebug() << "EnhancedAddMusicSingleDialog: Metadata extraction completed";
        
    } catch (const std::exception& e) {
        hideProgress();
        m_isExtracting = false;
        
        qWarning() << "EnhancedAddMusicSingleDialog: Metadata extraction failed:" << e.what();
        QMessageBox::warning(this, tr("Metadata Extraction"), 
                           tr("Failed to extract metadata: %1").arg(e.what()));
    }
}

void EnhancedAddMusicSingleDialog::onValidationTimer()
{
    updateValidationStatus();
}

void EnhancedAddMusicSingleDialog::onAddButtonClicked()
{
    accept();
}

void EnhancedAddMusicSingleDialog::onCancelButtonClicked()
{
    reject();
}

void EnhancedAddMusicSingleDialog::onClearFormClicked()
{
    int ret = QMessageBox::question(this, tr("Clear Form"),
                                   tr("Are you sure you want to clear all fields?"),
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        resetForm();
    }
}

void EnhancedAddMusicSingleDialog::onAutoFillClicked()
{
    if (m_filePathEdit->text().isEmpty()) {
        QMessageBox::information(this, tr("Auto Fill"), tr("Please select a file first."));
        return;
    }
    
    extractMetadata();
}

void EnhancedAddMusicSingleDialog::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // File selection group
    m_fileGroup = new QGroupBox(tr("File Selection"), this);
    m_fileLayout = new QFormLayout(m_fileGroup);
    
    // File path input
    auto* filePathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText(tr("Select an audio file..."));
    m_browseButton = new QPushButton(tr("Browse..."), this);
    m_browseButton->setMaximumWidth(100);
    
    filePathLayout->addWidget(m_filePathEdit);
    filePathLayout->addWidget(m_browseButton);
    
    m_fileLayout->addRow(tr("File Path:"), filePathLayout);
    
    // File info and metadata extraction
    m_fileInfoLabel = new QLabel(this);
    m_fileInfoLabel->setStyleSheet("color: #666666; font-size: 11px;");
    m_fileLayout->addRow(tr("File Info:"), m_fileInfoLabel);
    
    m_extractMetadataButton = new QPushButton(tr("Extract Metadata"), this);
    m_extractMetadataButton->setEnabled(false);
    m_fileLayout->addRow("", m_extractMetadataButton);
    
    m_mainLayout->addWidget(m_fileGroup);
    
    // Music information group
    m_musicInfoGroup = new QGroupBox(tr("Music Information"), this);
    m_musicInfoLayout = new QFormLayout(m_musicInfoGroup);
    
    // Required fields
    m_titleEdit = new QLineEdit(this);
    m_titleEdit->setPlaceholderText(tr("Enter song title..."));
    m_musicInfoLayout->addRow(tr("Title *:"), m_titleEdit);
    
    m_artistEdit = new QLineEdit(this);
    m_artistEdit->setPlaceholderText(tr("Enter artist name..."));
    m_musicInfoLayout->addRow(tr("Artist *:"), m_artistEdit);
    
    // Optional fields
    m_albumEdit = new QLineEdit(this);
    m_albumEdit->setPlaceholderText(tr("Enter album name..."));
    m_musicInfoLayout->addRow(tr("Album:"), m_albumEdit);
    
    m_genre1Combo = new QComboBox(this);
    m_genre1Combo->setEditable(true);
    m_musicInfoLayout->addRow(tr("Primary Genre:"), m_genre1Combo);
    
    m_genre2Combo = new QComboBox(this);
    m_genre2Combo->setEditable(true);
    m_musicInfoLayout->addRow(tr("Secondary Genre:"), m_genre2Combo);
    
    m_countryCombo = new QComboBox(this);
    m_countryCombo->setEditable(true);
    m_musicInfoLayout->addRow(tr("Country:"), m_countryCombo);
    
    m_dateEdit = new QDateEdit(QDate::currentDate(), this);
    m_dateEdit->setCalendarPopup(true);
    m_musicInfoLayout->addRow(tr("Date Added:"), m_dateEdit);
    
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(80);
    m_notesEdit->setPlaceholderText(tr("Optional notes..."));
    m_musicInfoLayout->addRow(tr("Notes:"), m_notesEdit);
    
    m_mainLayout->addWidget(m_musicInfoGroup);
    
    // Validation status
    m_validationStatusLabel = new QLabel(this);
    m_validationStatusLabel->setWordWrap(true);
    m_validationStatusLabel->setStyleSheet("color: #666666; font-size: 11px; padding: 5px;");
    m_mainLayout->addWidget(m_validationStatusLabel);
    
    // Progress indication
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    m_progressLabel = new QLabel(this);
    m_progressLabel->setVisible(false);
    m_progressLabel->setStyleSheet("color: #666666; font-size: 11px;");
    
    m_mainLayout->addWidget(m_progressLabel);
    m_mainLayout->addWidget(m_progressBar);
    
    // Buttons
    m_buttonLayout = new QHBoxLayout();
    
    m_clearButton = new QPushButton(tr("Clear"), this);
    m_manageGenresButton = new QPushButton(tr("Manage Genres"), this);
    m_autoFillButton = new QPushButton(tr("Auto Fill"), this);
    
    m_buttonLayout->addWidget(m_clearButton);
    m_buttonLayout->addWidget(m_manageGenresButton);
    m_buttonLayout->addWidget(m_autoFillButton);
    m_buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_addButton = new QPushButton(tr("Add Music"), this);
    m_addButton->setDefault(true);
    m_addButton->setEnabled(false);
    
    m_buttonLayout->addWidget(m_cancelButton);
    m_buttonLayout->addWidget(m_addButton);
    
    m_mainLayout->addLayout(m_buttonLayout);
    
    // Set tab order for better accessibility
    setTabOrder(m_filePathEdit, m_browseButton);
    setTabOrder(m_browseButton, m_titleEdit);
    setTabOrder(m_titleEdit, m_artistEdit);
    setTabOrder(m_artistEdit, m_albumEdit);
    setTabOrder(m_albumEdit, m_genre1Combo);
    setTabOrder(m_genre1Combo, m_genre2Combo);
    setTabOrder(m_genre2Combo, m_countryCombo);
    setTabOrder(m_countryCombo, m_dateEdit);
    setTabOrder(m_dateEdit, m_notesEdit);
    setTabOrder(m_notesEdit, m_addButton);
    setTabOrder(m_addButton, m_cancelButton);
}

void EnhancedAddMusicSingleDialog::setupValidation()
{
    m_validationTimer->setSingleShot(true);
    m_validationTimer->setInterval(300);
    
    // Set up input validators
    if (m_validator) {
        // File path validation will be handled in onFilePathChanged
        
        // Title validation (required, max length)
        m_titleEdit->setMaxLength(255);
        
        // Artist validation (required, max length)
        m_artistEdit->setMaxLength(255);
        
        // Album validation (optional, max length)
        m_albumEdit->setMaxLength(255);
    }
}

void EnhancedAddMusicSingleDialog::setupConnections()
{
    // File selection
    connect(m_browseButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onBrowseFileClicked);
    connect(m_filePathEdit, &QLineEdit::textChanged, this, &EnhancedAddMusicSingleDialog::onFilePathChanged);
    connect(m_extractMetadataButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onAutoFillClicked);
    
    // Input validation
    connect(m_titleEdit, &QLineEdit::textChanged, this, &EnhancedAddMusicSingleDialog::onInputChanged);
    connect(m_artistEdit, &QLineEdit::textChanged, this, &EnhancedAddMusicSingleDialog::onInputChanged);
    connect(m_albumEdit, &QLineEdit::textChanged, this, &EnhancedAddMusicSingleDialog::onInputChanged);
    connect(m_genre1Combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &EnhancedAddMusicSingleDialog::onInputChanged);
    connect(m_genre2Combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &EnhancedAddMusicSingleDialog::onInputChanged);
    
    // Validation timer
    connect(m_validationTimer, &QTimer::timeout, this, &EnhancedAddMusicSingleDialog::onValidationTimer);
    
    // Metadata extraction
    connect(m_metadataWatcher, &QFutureWatcher<QVariantMap>::finished,
            this, &EnhancedAddMusicSingleDialog::onMetadataExtractionCompleted);
    
    // Buttons
    connect(m_addButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onAddButtonClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onCancelButtonClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onClearFormClicked);
    connect(m_manageGenresButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onManageGenresClicked);
    connect(m_autoFillButton, &QPushButton::clicked, this, &EnhancedAddMusicSingleDialog::onAutoFillClicked);
}

void EnhancedAddMusicSingleDialog::loadGenres()
{
    if (!m_repository) {
        return;
    }
    
    try {
        // This would load genres from the repository
        // For now, add some default genres
        QStringList genres = {"Rock", "Pop", "Jazz", "Classical", "Electronic", "Hip Hop", "Country", "Blues"};
        
        m_genre1Combo->clear();
        m_genre2Combo->clear();
        
        m_genre1Combo->addItem(""); // Empty option
        m_genre2Combo->addItem(""); // Empty option
        
        for (const QString& genre : genres) {
            m_genre1Combo->addItem(genre);
            m_genre2Combo->addItem(genre);
        }
        
    } catch (const std::exception& e) {
        qWarning() << "EnhancedAddMusicSingleDialog: Failed to load genres:" << e.what();
    }
}

void EnhancedAddMusicSingleDialog::loadCountries()
{
    // Load country list using modern Qt API
    QList<QLocale> allLocales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyTerritory);
    QStringList countries;
    
    for (const QLocale& locale : allLocales) {
        QString country = QLocale::territoryToString(locale.territory());
        if (!countries.contains(country) && !country.isEmpty()) {
            countries.append(country);
        }
    }
    
    countries.sort();
    m_countryCombo->addItems(countries);
}

bool EnhancedAddMusicSingleDialog::validateInputs()
{
    bool valid = true;
    m_fieldErrors.clear();
    
    // Validate file path
    if (!validateFilePath()) {
        valid = false;
    }
    
    // Validate required fields
    if (!validateRequiredFields()) {
        valid = false;
    }
    
    m_formValid = valid;
    updateValidationStatus();
    
    return valid;
}

bool EnhancedAddMusicSingleDialog::validateFilePath()
{
    QString filePath = m_filePathEdit->text().trimmed();
    
    if (filePath.isEmpty()) {
        showFieldError(m_filePathEdit, tr("File path is required"));
        return false;
    }
    
    if (!QFileInfo::exists(filePath)) {
        showFieldError(m_filePathEdit, tr("File does not exist"));
        return false;
    }
    
    if (!isSupportedAudioFile(filePath)) {
        showFieldError(m_filePathEdit, tr("Unsupported audio format"));
        return false;
    }
    
    clearFieldError(m_filePathEdit);
    return true;
}

bool EnhancedAddMusicSingleDialog::validateRequiredFields()
{
    bool valid = true;
    
    // Title is required
    if (m_titleEdit->text().trimmed().isEmpty()) {
        showFieldError(m_titleEdit, tr("Title is required"));
        valid = false;
    } else {
        clearFieldError(m_titleEdit);
    }
    
    // Artist is required
    if (m_artistEdit->text().trimmed().isEmpty()) {
        showFieldError(m_artistEdit, tr("Artist is required"));
        valid = false;
    } else {
        clearFieldError(m_artistEdit);
    }
    
    return valid;
}

void EnhancedAddMusicSingleDialog::updateValidationStatus()
{
    bool valid = validateInputs();
    
    m_addButton->setEnabled(valid && !m_isAdding && !m_isExtracting);
    
    if (valid) {
        m_validationStatusLabel->setText(tr("✓ All fields are valid. Ready to add music."));
        m_validationStatusLabel->setStyleSheet("color: #4CAF50; font-size: 11px; padding: 5px;");
    } else {
        QStringList errors;
        for (auto it = m_fieldErrors.begin(); it != m_fieldErrors.end(); ++it) {
            errors.append(it.value());
        }
        
        if (!errors.isEmpty()) {
            m_validationStatusLabel->setText(tr("⚠ Please fix the following issues:\n• %1").arg(errors.join("\n• ")));
            m_validationStatusLabel->setStyleSheet("color: #F44336; font-size: 11px; padding: 5px;");
        } else {
            m_validationStatusLabel->setText(tr("Please fill in the required fields."));
            m_validationStatusLabel->setStyleSheet("color: #666666; font-size: 11px; padding: 5px;");
        }
    }
}

void EnhancedAddMusicSingleDialog::showFieldError(QWidget* field, const QString& error)
{
    m_fieldErrors[field] = error;
    field->setStyleSheet(INVALID_FIELD_STYLE);
    field->setToolTip(error);
}

void EnhancedAddMusicSingleDialog::clearFieldError(QWidget* field)
{
    m_fieldErrors.remove(field);
    field->setStyleSheet(VALID_FIELD_STYLE);
    field->setToolTip("");
}

void EnhancedAddMusicSingleDialog::extractMetadata()
{
    QString filePath = m_filePathEdit->text().trimmed();
    if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
        return;
    }
    
    if (m_isExtracting) {
        return; // Already extracting
    }
    
    m_isExtracting = true;
    showProgress(tr("Extracting metadata..."));
    
    extractMetadataAsync(filePath);
}

void EnhancedAddMusicSingleDialog::extractMetadataAsync(const QString& filePath)
{
    QFuture<QVariantMap> future = QtConcurrent::run([filePath]() -> QVariantMap {
        MetadataExtractionResult result = MetadataExtractor::extractMetadata(filePath);
        return result.toVariantMap();
    });
    
    m_metadataWatcher->setFuture(future);
}

void EnhancedAddMusicSingleDialog::applyMetadata(const QVariantMap& metadata)
{
    MetadataExtractionResult result = MetadataExtractionResult::fromVariantMap(metadata);
    
    if (!result.success) {
        QMessageBox::warning(this, tr("Metadata Extraction"), 
                           tr("Failed to extract metadata: %1").arg(result.errorMessage));
        return;
    }
    
    // Apply metadata to form fields (only if fields are empty)
    if (m_titleEdit->text().isEmpty() && !result.title.isEmpty()) {
        m_titleEdit->setText(result.title);
    }
    
    if (m_artistEdit->text().isEmpty() && !result.artist.isEmpty()) {
        m_artistEdit->setText(result.artist);
    }
    
    if (m_albumEdit->text().isEmpty() && !result.album.isEmpty()) {
        m_albumEdit->setText(result.album);
    }
    
    if (m_genre1Combo->currentText().isEmpty() && !result.genre.isEmpty()) {
        int index = m_genre1Combo->findText(result.genre);
        if (index >= 0) {
            m_genre1Combo->setCurrentIndex(index);
        } else {
            m_genre1Combo->setCurrentText(result.genre);
        }
    }
    
    // Update file info with extracted metadata
    QString fileInfo = tr("Size: %1, Format: %2").arg(
        formatFileSize(result.fileSize),
        QFileInfo(m_filePathEdit->text()).suffix().toUpper()
    );
    
    if (result.duration > 0) {
        fileInfo += tr(", Duration: %1").arg(formatDuration(result.duration));
    }
    
    if (result.bitrate > 0) {
        fileInfo += tr(", Bitrate: %1 kbps").arg(result.bitrate);
    }
    
    m_fileInfoLabel->setText(fileInfo);
    
    // Trigger validation update
    onInputChanged();
}

void EnhancedAddMusicSingleDialog::showProgress(const QString& message)
{
    m_progressLabel->setText(message);
    m_progressLabel->setVisible(true);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate progress
    
    setFormEnabled(false);
}

void EnhancedAddMusicSingleDialog::hideProgress()
{
    m_progressLabel->setVisible(false);
    m_progressBar->setVisible(false);
    
    setFormEnabled(true);
}

void EnhancedAddMusicSingleDialog::updateProgress(int value, const QString& message)
{
    if (!message.isEmpty()) {
        m_progressLabel->setText(message);
    }
    
    if (m_progressBar->maximum() == 0) {
        m_progressBar->setRange(0, 100);
    }
    
    m_progressBar->setValue(value);
}

bool EnhancedAddMusicSingleDialog::addMusic()
{
    if (!validateInputs() || !m_repository) {
        return false;
    }
    
    m_isAdding = true;
    showProgress(tr("Adding music to library..."));
    
    try {
        MusicItem musicItem = createMusicItemFromForm();
        
        if (m_repository->addMusic(musicItem)) {
            m_addedMusicId = musicItem.id;
            
            hideProgress();
            m_isAdding = false;
            
            emit musicAdded(m_addedMusicId);
            
            QMessageBox::information(this, tr("Success"), tr("Music added successfully!"));
            
            return true;
        } else {
            hideProgress();
            m_isAdding = false;
            
            QString error = tr("Failed to add music to the database.");
            emit errorOccurred(error);
            QMessageBox::critical(this, tr("Error"), error);
            
            return false;
        }
        
    } catch (const std::exception& e) {
        hideProgress();
        m_isAdding = false;
        
        QString error = tr("Failed to add music: %1").arg(e.what());
        emit errorOccurred(error);
        QMessageBox::critical(this, tr("Error"), error);
        
        return false;
    }
}

MusicItem EnhancedAddMusicSingleDialog::createMusicItemFromForm() const
{
    MusicItem item;
    
    item.song = m_titleEdit->text().trimmed();
    item.artist = m_artistEdit->text().trimmed();
    // Album field not available in current MusicItem structure
    item.genre1 = m_genre1Combo->currentText().trimmed();
    item.genre2 = m_genre2Combo->currentText().trimmed();
    item.country = m_countryCombo->currentText().trimmed();
    item.path = m_filePathEdit->text().trimmed();
    // Use publishedDate field for the date
    item.publishedDate = m_dateEdit->date().toString(Qt::ISODate);
    // Notes field not available in current MusicItem structure
    
    // Time field can be used for duration if needed
    item.time = "0:00"; // Default, will be updated by metadata extraction
    
    return item;
}

void EnhancedAddMusicSingleDialog::resetForm()
{
    m_filePathEdit->clear();
    m_titleEdit->clear();
    m_artistEdit->clear();
    m_albumEdit->clear();
    m_genre1Combo->setCurrentIndex(0);
    m_genre2Combo->setCurrentIndex(0);
    m_countryCombo->setCurrentIndex(-1);
    m_dateEdit->setDate(QDate::currentDate());
    m_notesEdit->clear();
    
    m_fileInfoLabel->clear();
    m_validationStatusLabel->clear();
    
    m_fieldErrors.clear();
    m_formValid = false;
    m_addedMusicId = 0;
    
    // Reset field styles
    for (QWidget* widget : {m_filePathEdit, m_titleEdit, m_artistEdit, m_albumEdit}) {
        widget->setStyleSheet(NEUTRAL_FIELD_STYLE);
        widget->setToolTip("");
    }
    
    m_addButton->setEnabled(false);
    m_extractMetadataButton->setEnabled(false);
}

void EnhancedAddMusicSingleDialog::setFormEnabled(bool enabled)
{
    m_fileGroup->setEnabled(enabled);
    m_musicInfoGroup->setEnabled(enabled);
    m_addButton->setEnabled(enabled && m_formValid);
    m_clearButton->setEnabled(enabled);
    m_manageGenresButton->setEnabled(enabled);
    m_autoFillButton->setEnabled(enabled);
}

QStringList EnhancedAddMusicSingleDialog::getSupportedExtensions() const
{
    return {"*.mp3", "*.ogg", "*.wav", "*.flac", "*.m4a", "*.aac", "*.wma"};
}

QString EnhancedAddMusicSingleDialog::formatFileSize(qint64 bytes) const
{
    if (bytes < 1024) {
        return tr("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return tr("%1 KB").arg(bytes / 1024);
    } else if (bytes < 1024 * 1024 * 1024) {
        return tr("%.1f MB").arg(bytes / (1024.0 * 1024.0));
    } else {
        return tr("%.1f GB").arg(bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

QString EnhancedAddMusicSingleDialog::formatDuration(qint64 milliseconds) const
{
    int totalSeconds = static_cast<int>(milliseconds / 1000);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    
    return tr("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

bool EnhancedAddMusicSingleDialog::isSupportedAudioFile(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    QStringList supportedExtensions = {"mp3", "ogg", "wav", "flac", "m4a", "aac", "wma"};
    return supportedExtensions.contains(extension);
}

// MetadataExtractionResult implementation

QVariantMap MetadataExtractionResult::toVariantMap() const
{
    QVariantMap map;
    map["success"] = success;
    map["title"] = title;
    map["artist"] = artist;
    map["album"] = album;
    map["genre"] = genre;
    map["duration"] = duration;
    map["fileSize"] = fileSize;
    map["format"] = format;
    map["bitrate"] = bitrate;
    map["sampleRate"] = sampleRate;
    map["errorMessage"] = errorMessage;
    return map;
}

MetadataExtractionResult MetadataExtractionResult::fromVariantMap(const QVariantMap& map)
{
    MetadataExtractionResult result;
    result.success = map.value("success", false).toBool();
    result.title = map.value("title").toString();
    result.artist = map.value("artist").toString();
    result.album = map.value("album").toString();
    result.genre = map.value("genre").toString();
    result.duration = map.value("duration", 0).toLongLong();
    result.fileSize = map.value("fileSize", 0).toLongLong();
    result.format = map.value("format").toString();
    result.bitrate = map.value("bitrate", 0).toInt();
    result.sampleRate = map.value("sampleRate", 0).toInt();
    result.errorMessage = map.value("errorMessage").toString();
    return result;
}

// MetadataExtractor implementation

MetadataExtractor::MetadataExtractor(QObject* parent)
    : QObject(parent)
{
}

MetadataExtractionResult MetadataExtractor::extractMetadata(const QString& filePath)
{
    if (!QFileInfo::exists(filePath)) {
        MetadataExtractionResult result;
        result.success = false;
        result.errorMessage = "File does not exist";
        return result;
    }
    
    // Try different extraction methods in order of preference
    QStringList availableTools = getAvailableTools();
    
    if (availableTools.contains("mediainfo")) {
        return extractWithMediaInfo(filePath);
    } else if (availableTools.contains("ffprobe")) {
        return extractWithFFProbe(filePath);
    } else if (availableTools.contains("exiftool")) {
        return extractWithExifTool(filePath);
    } else {
        MetadataExtractionResult result;
        result.success = false;
        result.errorMessage = "No metadata extraction tools available";
        return result;
    }
}

bool MetadataExtractor::isMetadataExtractionAvailable()
{
    return !getAvailableTools().isEmpty();
}

QStringList MetadataExtractor::getAvailableTools()
{
    QStringList tools;
    QStringList candidates = {"mediainfo", "ffprobe", "exiftool"};
    
    for (const QString& tool : candidates) {
        QString toolPath = QStandardPaths::findExecutable(tool);
        if (!toolPath.isEmpty()) {
            tools.append(tool);
        }
    }
    
    return tools;
}

MetadataExtractionResult MetadataExtractor::extractWithExifTool(const QString& filePath)
{
    MetadataExtractionResult result;
    
    QProcess process;
    QStringList arguments;
    arguments << "-Duration" << "-Title" << "-Artist" << "-Album" << "-Genre" << filePath;
    
    process.start("exiftool", arguments);
    if (!process.waitForFinished(5000)) {
        result.success = false;
        result.errorMessage = "exiftool process timed out";
        return result;
    }
    
    if (process.exitCode() != 0) {
        result.success = false;
        result.errorMessage = QString("exiftool failed: %1").arg(process.readAllStandardError());
        return result;
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n');
    
    for (const QString& line : lines) {
        if (line.contains("Duration")) {
            QString durationStr = line.split(':').last().trimmed();
            result.duration = parseDuration(durationStr);
        } else if (line.contains("Title")) {
            result.title = line.split(':').last().trimmed();
        } else if (line.contains("Artist")) {
            result.artist = line.split(':').last().trimmed();
        } else if (line.contains("Album")) {
            result.album = line.split(':').last().trimmed();
        } else if (line.contains("Genre")) {
            result.genre = line.split(':').last().trimmed();
        }
    }
    
    QFileInfo fileInfo(filePath);
    result.fileSize = fileInfo.size();
    result.format = fileInfo.suffix().toUpper();
    result.success = true;
    
    return result;
}

MetadataExtractionResult MetadataExtractor::extractWithMediaInfo(const QString& filePath)
{
    MetadataExtractionResult result;
    
    QProcess process;
    QStringList arguments;
    arguments << "--Output=JSON" << filePath;
    
    process.start("mediainfo", arguments);
    if (!process.waitForFinished(5000)) {
        result.success = false;
        result.errorMessage = "mediainfo process timed out";
        return result;
    }
    
    if (process.exitCode() != 0) {
        result.success = false;
        result.errorMessage = QString("mediainfo failed: %1").arg(process.readAllStandardError());
        return result;
    }
    
    // Parse JSON output (simplified implementation)
    QString output = process.readAllStandardOutput();
    
    // For now, use a simple text-based parsing
    // In a real implementation, you would parse the JSON properly
    if (output.contains("Title")) {
        QRegularExpression titleRegex("\"Title\"\\s*:\\s*\"([^\"]+)\"");
        QRegularExpressionMatch match = titleRegex.match(output);
        if (match.hasMatch()) {
            result.title = match.captured(1);
        }
    }
    
    QFileInfo fileInfo(filePath);
    result.fileSize = fileInfo.size();
    result.format = fileInfo.suffix().toUpper();
    result.success = true;
    
    return result;
}

MetadataExtractionResult MetadataExtractor::extractWithFFProbe(const QString& filePath)
{
    MetadataExtractionResult result;
    
    QProcess process;
    QStringList arguments;
    arguments << "-v" << "quiet" << "-print_format" << "json" << "-show_format" << filePath;
    
    process.start("ffprobe", arguments);
    if (!process.waitForFinished(5000)) {
        result.success = false;
        result.errorMessage = "ffprobe process timed out";
        return result;
    }
    
    if (process.exitCode() != 0) {
        result.success = false;
        result.errorMessage = QString("ffprobe failed: %1").arg(process.readAllStandardError());
        return result;
    }
    
    // Parse JSON output (simplified implementation)
    QString output = process.readAllStandardOutput();
    
    // Simple text-based parsing for demonstration
    if (output.contains("duration")) {
        QRegularExpression durationRegex("\"duration\"\\s*:\\s*\"([^\"]+)\"");
        QRegularExpressionMatch match = durationRegex.match(output);
        if (match.hasMatch()) {
            result.duration = static_cast<qint64>(match.captured(1).toDouble() * 1000);
        }
    }
    
    QFileInfo fileInfo(filePath);
    result.fileSize = fileInfo.size();
    result.format = fileInfo.suffix().toUpper();
    result.success = true;
    
    return result;
}

qint64 MetadataExtractor::parseDuration(const QString& durationStr)
{
    // Parse various duration formats (HH:MM:SS, MM:SS, seconds, etc.)
    QRegularExpression timeRegex("(\\d+):(\\d+):(\\d+)");
    QRegularExpressionMatch match = timeRegex.match(durationStr);
    
    if (match.hasMatch()) {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        int seconds = match.captured(3).toInt();
        return (hours * 3600 + minutes * 60 + seconds) * 1000;
    }
    
    // Try MM:SS format
    QRegularExpression mmssRegex("(\\d+):(\\d+)");
    match = mmssRegex.match(durationStr);
    if (match.hasMatch()) {
        int minutes = match.captured(1).toInt();
        int seconds = match.captured(2).toInt();
        return (minutes * 60 + seconds) * 1000;
    }
    
    // Try seconds only
    bool ok;
    double seconds = durationStr.toDouble(&ok);
    if (ok) {
        return static_cast<qint64>(seconds * 1000);
    }
    
    return 0;
}

int MetadataExtractor::parseBitrate(const QString& bitrateStr)
{
    QRegularExpression bitrateRegex("(\\d+)");
    QRegularExpressionMatch match = bitrateRegex.match(bitrateStr);
    
    if (match.hasMatch()) {
        return match.captured(1).toInt();
    }
    
    return 0;
}