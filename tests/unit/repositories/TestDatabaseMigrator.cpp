#include "TestDatabaseMigrator.h"
#include "../../../src/repositories/DatabaseMigrator.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSignalSpy>
#include <QFile>

void TestDatabaseMigrator::initTestCase()
{
    // Ensure we have a valid temporary directory
    QVERIFY(m_tempDir.isValid());
    
    // Set up database path
    m_databasePath = m_tempDir.path() + "/test_migrator.db";
}

void TestDatabaseMigrator::cleanupTestCase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
    QSqlDatabase::removeDatabase("test_migrator_connection");
}

void TestDatabaseMigrator::init()
{
    // Set up fresh database for each test
    setupTestDatabase();
    
    // Create migrator instance
    m_migrator = std::make_unique<DatabaseMigrator>(m_database, this);
}

void TestDatabaseMigrator::cleanup()
{
    // Clean up migrator
    m_migrator.reset();
    
    // Close database connection
    if (m_database.isOpen()) {
        m_database.close();
    }
}

void TestDatabaseMigrator::setupTestDatabase()
{
    // Remove existing connection if any
    if (QSqlDatabase::contains("test_migrator_connection")) {
        QSqlDatabase::removeDatabase("test_migrator_connection");
    }
    
    // Create new database connection
    m_database = QSqlDatabase::addDatabase("QSQLITE", "test_migrator_connection");
    m_database.setDatabaseName(m_databasePath);
    
    QVERIFY(m_database.open());
}

Migration TestDatabaseMigrator::createValidMigration(const QString& version, const QString& name, const QString& upSql, const QString& downSql)
{
    Migration migration;
    migration.version = version;
    migration.name = name;
    migration.description = QString("Test migration %1").arg(version);
    migration.upSql = upSql;
    migration.downSql = downSql;
    return migration;
}

void TestDatabaseMigrator::insertTestMigrations()
{
    // Add some test migrations
    m_migrator->addMigration(createValidMigration("001", "Create users table", 
        "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);",
        "DROP TABLE users;"));
    
    m_migrator->addMigration(createValidMigration("002", "Add email to users", 
        "ALTER TABLE users ADD COLUMN email TEXT;",
        "ALTER TABLE users DROP COLUMN email;"));
    
    m_migrator->addMigration(createValidMigration("003", "Create posts table", 
        "CREATE TABLE posts (id INTEGER PRIMARY KEY, title TEXT, user_id INTEGER);",
        "DROP TABLE posts;"));
}

void TestDatabaseMigrator::testInitialize()
{
    bool result = m_migrator->initialize();
    
    QVERIFY(result);
    
    // Check that migrations table was created
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='schema_migrations'");
    QVERIFY(query.exec());
    QVERIFY(query.next());
}

void TestDatabaseMigrator::testCreateMigrationsTable()
{
    // Initialize should create the table
    QVERIFY(m_migrator->initialize());
    
    // Verify table structure
    QSqlQuery query(m_database);
    query.prepare("PRAGMA table_info(schema_migrations)");
    QVERIFY(query.exec());
    
    QStringList columns;
    while (query.next()) {
        columns.append(query.value("name").toString());
    }
    
    QVERIFY(columns.contains("id"));
    QVERIFY(columns.contains("version"));
    QVERIFY(columns.contains("name"));
    QVERIFY(columns.contains("description"));
    QVERIFY(columns.contains("up_sql"));
    QVERIFY(columns.contains("down_sql"));
    QVERIFY(columns.contains("applied_at"));
    QVERIFY(columns.contains("is_applied"));
}

void TestDatabaseMigrator::testAddMigration()
{
    QVERIFY(m_migrator->initialize());
    
    Migration migration = createValidMigration();
    
    bool result = m_migrator->addMigration(migration);
    
    QVERIFY(result);
    
    // Verify migration was added
    QVERIFY(m_migrator->migrationExists("001"));
    
    Migration retrieved = m_migrator->getMigrationByVersion("001");
    QVERIFY(retrieved.isValid());
    QCOMPARE(retrieved.version, migration.version);
    QCOMPARE(retrieved.name, migration.name);
}

void TestDatabaseMigrator::testAddMigrationWithInvalidData()
{
    QVERIFY(m_migrator->initialize());
    
    Migration invalidMigration; // Empty migration
    
    QSignalSpy errorSpy(m_migrator.get(), &DatabaseMigrator::operationError);
    
    bool result = m_migrator->addMigration(invalidMigration);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestDatabaseMigrator::testAddMigrationDuplicateVersion()
{
    QVERIFY(m_migrator->initialize());
    
    Migration migration1 = createValidMigration("001", "First Migration");
    Migration migration2 = createValidMigration("001", "Duplicate Migration"); // Same version
    
    // Add first migration
    QVERIFY(m_migrator->addMigration(migration1));
    
    QSignalSpy errorSpy(m_migrator.get(), &DatabaseMigrator::operationError);
    
    // Try to add duplicate
    bool result = m_migrator->addMigration(migration2);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestDatabaseMigrator::testAddMigrationInvalidSql()
{
    QVERIFY(m_migrator->initialize());
    
    Migration migration = createValidMigration("001", "Invalid SQL", 
        "DROP DATABASE test;", // Dangerous operation
        "");
    
    QSignalSpy errorSpy(m_migrator.get(), &DatabaseMigrator::operationError);
    
    bool result = m_migrator->addMigration(migration);
    
    QVERIFY(!result);
    QCOMPARE(errorSpy.count(), 1);
}

void TestDatabaseMigrator::testMigrate()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    QSignalSpy batchStartedSpy(m_migrator.get(), &DatabaseMigrator::migrationBatchStarted);
    QSignalSpy batchCompletedSpy(m_migrator.get(), &DatabaseMigrator::migrationBatchCompleted);
    QSignalSpy migrationCompletedSpy(m_migrator.get(), &DatabaseMigrator::migrationCompleted);
    
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    
    QVERIFY(result.success);
    QCOMPARE(result.totalMigrations, 3);
    QCOMPARE(result.appliedCount, 3);
    QCOMPARE(result.appliedMigrations.size(), 3);
    QCOMPARE(result.failedMigrations.size(), 0);
    
    QCOMPARE(batchStartedSpy.count(), 1);
    QCOMPARE(batchCompletedSpy.count(), 1);
    QCOMPARE(migrationCompletedSpy.count(), 3);
    
    // Verify tables were created
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='users'");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='posts'");
    QVERIFY(query.exec());
    QVERIFY(query.next());
    
    // Verify current version
    QCOMPARE(m_migrator->getCurrentVersion(), QString("003"));
}

void TestDatabaseMigrator::testMigrateEmpty()
{
    QVERIFY(m_migrator->initialize());
    
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    
    QVERIFY(result.success);
    QCOMPARE(result.totalMigrations, 0);
    QCOMPARE(result.appliedCount, 0);
}

void TestDatabaseMigrator::testMigrateTo()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("002");
    
    QVERIFY(result.success);
    QCOMPARE(result.totalMigrations, 2);
    QCOMPARE(result.appliedCount, 2);
    
    // Verify only first two migrations were applied
    QVERIFY(m_migrator->isMigrationApplied("001"));
    QVERIFY(m_migrator->isMigrationApplied("002"));
    QVERIFY(!m_migrator->isMigrationApplied("003"));
    
    QCOMPARE(m_migrator->getCurrentVersion(), QString("002"));
}

void TestDatabaseMigrator::testMigrateToInvalidVersion()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("999");
    
    QVERIFY(result.success);
    QCOMPARE(result.totalMigrations, 3); // All migrations should be applied
    QCOMPARE(result.appliedCount, 3);
}

void TestDatabaseMigrator::testRollback()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    // First apply all migrations
    DatabaseMigrator::MigrationResult migrateResult = m_migrator->migrate();
    QVERIFY(migrateResult.success);
    
    QSignalSpy rollbackCompletedSpy(m_migrator.get(), &DatabaseMigrator::rollbackCompleted);
    
    // Rollback last migration
    DatabaseMigrator::MigrationResult rollbackResult = m_migrator->rollback(1);
    
    QVERIFY(rollbackResult.success);
    QCOMPARE(rollbackResult.totalMigrations, 1);
    QCOMPARE(rollbackResult.appliedCount, 1);
    
    QCOMPARE(rollbackCompletedSpy.count(), 1);
    
    // Verify migration 003 is no longer applied
    QVERIFY(m_migrator->isMigrationApplied("001"));
    QVERIFY(m_migrator->isMigrationApplied("002"));
    QVERIFY(!m_migrator->isMigrationApplied("003"));
    
    QCOMPARE(m_migrator->getCurrentVersion(), QString("002"));
    
    // Verify posts table was dropped
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name='posts'");
    QVERIFY(query.exec());
    QVERIFY(!query.next()); // Should not exist
}

void TestDatabaseMigrator::testRollbackTo()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    // First apply all migrations
    DatabaseMigrator::MigrationResult migrateResult = m_migrator->migrate();
    QVERIFY(migrateResult.success);
    
    // Rollback to version 001
    DatabaseMigrator::MigrationResult rollbackResult = m_migrator->rollbackTo("001");
    
    QVERIFY(rollbackResult.success);
    QCOMPARE(rollbackResult.totalMigrations, 2); // Should rollback 003 and 002
    QCOMPARE(rollbackResult.appliedCount, 2);
    
    // Verify only migration 001 is applied
    QVERIFY(m_migrator->isMigrationApplied("001"));
    QVERIFY(!m_migrator->isMigrationApplied("002"));
    QVERIFY(!m_migrator->isMigrationApplied("003"));
    
    QCOMPARE(m_migrator->getCurrentVersion(), QString("001"));
}

void TestDatabaseMigrator::testRollbackWithoutDownSql()
{
    QVERIFY(m_migrator->initialize());
    
    // Add migration without down SQL
    Migration migration = createValidMigration("001", "No Rollback", 
        "CREATE TABLE test (id INTEGER);", "");
    
    QVERIFY(m_migrator->addMigration(migration));
    
    // Apply migration
    DatabaseMigrator::MigrationResult migrateResult = m_migrator->migrate();
    QVERIFY(migrateResult.success);
    
    QSignalSpy rollbackFailedSpy(m_migrator.get(), &DatabaseMigrator::rollbackFailed);
    
    // Try to rollback
    DatabaseMigrator::MigrationResult rollbackResult = m_migrator->rollback(1);
    
    QVERIFY(!rollbackResult.success);
    QCOMPARE(rollbackFailedSpy.count(), 1);
}

void TestDatabaseMigrator::testGetCurrentVersion()
{
    QVERIFY(m_migrator->initialize());
    
    // No migrations applied
    QVERIFY(m_migrator->getCurrentVersion().isEmpty());
    
    insertTestMigrations();
    
    // Apply first two migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("002");
    QVERIFY(result.success);
    
    QCOMPARE(m_migrator->getCurrentVersion(), QString("002"));
}

void TestDatabaseMigrator::testGetLatestVersion()
{
    QVERIFY(m_migrator->initialize());
    
    // No migrations added
    QVERIFY(m_migrator->getLatestVersion().isEmpty());
    
    insertTestMigrations();
    
    QCOMPARE(m_migrator->getLatestVersion(), QString("003"));
}

void TestDatabaseMigrator::testCompareVersions()
{
    // This tests the internal compareVersions method indirectly
    QVERIFY(m_migrator->initialize());
    
    m_migrator->addMigration(createValidMigration("001", "First"));
    m_migrator->addMigration(createValidMigration("010", "Second"));
    m_migrator->addMigration(createValidMigration("002", "Third"));
    
    QList<Migration> migrations = m_migrator->getMigrations();
    QCOMPARE(migrations.size(), 3);
    
    // The latest version should be "010" (highest numeric value)
    QCOMPARE(m_migrator->getLatestVersion(), QString("010"));
}

void TestDatabaseMigrator::testGetMigrations()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    QList<Migration> allMigrations = m_migrator->getMigrations();
    QCOMPARE(allMigrations.size(), 3);
    
    QList<Migration> appliedMigrations = m_migrator->getMigrations(true);
    QCOMPARE(appliedMigrations.size(), 0); // None applied yet
    
    // Apply some migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("002");
    QVERIFY(result.success);
    
    appliedMigrations = m_migrator->getMigrations(true);
    QCOMPARE(appliedMigrations.size(), 2);
}

void TestDatabaseMigrator::testGetPendingMigrations()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    QList<Migration> pending = m_migrator->getPendingMigrations();
    QCOMPARE(pending.size(), 3);
    
    // Apply first migration
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("001");
    QVERIFY(result.success);
    
    pending = m_migrator->getPendingMigrations();
    QCOMPARE(pending.size(), 2);
}

void TestDatabaseMigrator::testGetAppliedMigrations()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    QList<Migration> applied = m_migrator->getAppliedMigrations();
    QCOMPARE(applied.size(), 0);
    
    // Apply migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    QVERIFY(result.success);
    
    applied = m_migrator->getAppliedMigrations();
    QCOMPARE(applied.size(), 3);
    
    // Check that applied migrations have appliedAt timestamp
    for (const Migration& migration : applied) {
        QVERIFY(migration.isApplied);
        QVERIFY(migration.appliedAt.isValid());
    }
}

void TestDatabaseMigrator::testGetMigrationByVersion()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    Migration migration = m_migrator->getMigrationByVersion("002");
    QVERIFY(migration.isValid());
    QCOMPARE(migration.version, QString("002"));
    QCOMPARE(migration.name, QString("Add email to users"));
    
    Migration notFound = m_migrator->getMigrationByVersion("999");
    QVERIFY(!notFound.isValid());
}

void TestDatabaseMigrator::testMigrationExists()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    QVERIFY(m_migrator->migrationExists("001"));
    QVERIFY(m_migrator->migrationExists("002"));
    QVERIFY(m_migrator->migrationExists("003"));
    QVERIFY(!m_migrator->migrationExists("999"));
}

void TestDatabaseMigrator::testIsMigrationApplied()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    // No migrations applied initially
    QVERIFY(!m_migrator->isMigrationApplied("001"));
    QVERIFY(!m_migrator->isMigrationApplied("002"));
    QVERIFY(!m_migrator->isMigrationApplied("003"));
    
    // Apply first migration
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("001");
    QVERIFY(result.success);
    
    QVERIFY(m_migrator->isMigrationApplied("001"));
    QVERIFY(!m_migrator->isMigrationApplied("002"));
    QVERIFY(!m_migrator->isMigrationApplied("003"));
}

void TestDatabaseMigrator::testGetStatistics()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    DatabaseMigrator::MigrationStats stats = m_migrator->getStatistics();
    
    QCOMPARE(stats.totalMigrations, 3);
    QCOMPARE(stats.appliedMigrations, 0);
    QCOMPARE(stats.pendingMigrations, 3);
    QVERIFY(stats.currentVersion.isEmpty());
    QCOMPARE(stats.latestVersion, QString("003"));
    QVERIFY(!stats.lastMigration.isValid());
    
    // Apply some migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrateTo("002");
    QVERIFY(result.success);
    
    stats = m_migrator->getStatistics();
    QCOMPARE(stats.totalMigrations, 3);
    QCOMPARE(stats.appliedMigrations, 2);
    QCOMPARE(stats.pendingMigrations, 1);
    QCOMPARE(stats.currentVersion, QString("002"));
    QCOMPARE(stats.latestVersion, QString("003"));
    QVERIFY(stats.lastMigration.isValid());
}

void TestDatabaseMigrator::testValidateMigrationSql()
{
    QVERIFY(m_migrator->initialize());
    
    // Valid SQL
    QString error = m_migrator->validateMigrationSql("CREATE TABLE test (id INTEGER);");
    QVERIFY(error.isEmpty());
    
    // Empty SQL
    error = m_migrator->validateMigrationSql("");
    QVERIFY(!error.isEmpty());
    
    // Dangerous SQL
    error = m_migrator->validateMigrationSql("DROP DATABASE test;");
    QVERIFY(!error.isEmpty());
    
    // Multiple statements
    error = m_migrator->validateMigrationSql("CREATE TABLE test1 (id INTEGER); CREATE TABLE test2 (id INTEGER);");
    QVERIFY(!error.isEmpty());
}

void TestDatabaseMigrator::testCreateBackup()
{
    QVERIFY(m_migrator->initialize());
    
    QString backupPath = m_tempDir.path() + "/backup.db";
    
    bool result = m_migrator->createBackup(backupPath);
    QVERIFY(result);
    
    // Verify backup file exists
    QFile backupFile(backupPath);
    QVERIFY(backupFile.exists());
    QVERIFY(backupFile.size() > 0);
}

void TestDatabaseMigrator::testRestoreFromBackup()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    // Apply migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    QVERIFY(result.success);
    
    // Create backup
    QString backupPath = m_tempDir.path() + "/backup.db";
    QVERIFY(m_migrator->createBackup(backupPath));
    
    // Reset migrations
    QVERIFY(m_migrator->reset());
    
    // Verify migrations are reset
    DatabaseMigrator::MigrationStats stats = m_migrator->getStatistics();
    QCOMPARE(stats.appliedMigrations, 0);
    
    // Restore from backup
    bool restoreResult = m_migrator->restoreFromBackup(backupPath);
    QVERIFY(restoreResult);
    
    // Verify migrations are restored
    stats = m_migrator->getStatistics();
    QCOMPARE(stats.appliedMigrations, 3);
}

void TestDatabaseMigrator::testReset()
{
    QVERIFY(m_migrator->initialize());
    insertTestMigrations();
    
    // Apply migrations
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    QVERIFY(result.success);
    
    // Verify migrations are applied
    DatabaseMigrator::MigrationStats stats = m_migrator->getStatistics();
    QCOMPARE(stats.appliedMigrations, 3);
    
    // Reset
    bool resetResult = m_migrator->reset();
    QVERIFY(resetResult);
    
    // Verify all migrations are marked as not applied
    stats = m_migrator->getStatistics();
    QCOMPARE(stats.appliedMigrations, 0);
    QCOMPARE(stats.pendingMigrations, 3);
}

void TestDatabaseMigrator::testDatabaseConnectionError()
{
    // Close the database to simulate connection error
    m_database.close();
    
    QSignalSpy errorSpy(m_migrator.get(), &DatabaseMigrator::operationError);
    
    bool result = m_migrator->initialize();
    
    // Should fail due to closed database
    QVERIFY(!result);
}

void TestDatabaseMigrator::testTransactionRollback()
{
    QVERIFY(m_migrator->initialize());
    
    // Add migration with invalid SQL that will fail during execution
    Migration invalidMigration = createValidMigration("001", "Invalid Migration", 
        "CREATE TABLE invalid_table (invalid_column INVALID_TYPE);", // This might fail on some databases
        "DROP TABLE invalid_table;");
    
    // For SQLite, let's use a more reliable way to cause failure
    invalidMigration.upSql = "INSERT INTO non_existent_table VALUES (1);"; // This will definitely fail
    
    QVERIFY(m_migrator->addMigration(invalidMigration));
    
    QSignalSpy migrationFailedSpy(m_migrator.get(), &DatabaseMigrator::migrationFailed);
    
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    
    QVERIFY(!result.success);
    QCOMPARE(migrationFailedSpy.count(), 1);
    QCOMPARE(result.appliedCount, 0);
    QCOMPARE(result.failedMigrations.size(), 1);
}

void TestDatabaseMigrator::testSignalEmission()
{
    QVERIFY(m_migrator->initialize());
    
    QSignalSpy migrationStartedSpy(m_migrator.get(), &DatabaseMigrator::migrationStarted);
    QSignalSpy migrationCompletedSpy(m_migrator.get(), &DatabaseMigrator::migrationCompleted);
    QSignalSpy migrationFailedSpy(m_migrator.get(), &DatabaseMigrator::migrationFailed);
    QSignalSpy rollbackStartedSpy(m_migrator.get(), &DatabaseMigrator::rollbackStarted);
    QSignalSpy rollbackCompletedSpy(m_migrator.get(), &DatabaseMigrator::rollbackCompleted);
    QSignalSpy batchStartedSpy(m_migrator.get(), &DatabaseMigrator::migrationBatchStarted);
    QSignalSpy batchCompletedSpy(m_migrator.get(), &DatabaseMigrator::migrationBatchCompleted);
    QSignalSpy errorSpy(m_migrator.get(), &DatabaseMigrator::operationError);
    
    insertTestMigrations();
    
    // Test migration signals
    DatabaseMigrator::MigrationResult result = m_migrator->migrate();
    QVERIFY(result.success);
    
    QCOMPARE(migrationStartedSpy.count(), 3);
    QCOMPARE(migrationCompletedSpy.count(), 3);
    QCOMPARE(batchStartedSpy.count(), 1);
    QCOMPARE(batchCompletedSpy.count(), 1);
    
    // Test rollback signals
    DatabaseMigrator::MigrationResult rollbackResult = m_migrator->rollback(1);
    QVERIFY(rollbackResult.success);
    
    QCOMPARE(rollbackStartedSpy.count(), 1);
    QCOMPARE(rollbackCompletedSpy.count(), 1);
    
    // Test error signal
    Migration invalid;
    m_migrator->addMigration(invalid);
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(TestDatabaseMigrator)
#include "TestDatabaseMigrator.moc"