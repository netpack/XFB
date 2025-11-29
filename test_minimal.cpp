#include <QCoreApplication>
#include <QDebug>
#include <QTemporaryDir>
#include "src/services/Logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "Starting minimal test...";
    
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        qDebug() << "Failed to create temp dir";
        return 1;
    }
    
    QString logDir = tempDir.path() + "/logs";
    qDebug() << "Log directory:" << logDir;
    
    Logger logger;
    qDebug() << "Logger created";
    
    bool result = logger.initialize(logDir, 5, 5);
    qDebug() << "Initialize result:" << result;
    
    if (result) {
        logger.writeLog(Logger::LogLevel::Info, "Test", "Test message", "Test");
        qDebug() << "Log written";
        
        logger.flush();
        qDebug() << "Log flushed";
    }
    
    qDebug() << "Test completed successfully";
    return 0;
}