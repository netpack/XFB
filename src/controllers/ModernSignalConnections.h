#ifndef MODERNSIGNALCONNECTIONS_H
#define MODERNSIGNALCONNECTIONS_H

#include <QObject>
#include <QMetaObject>
#include <QDebug>
#include <functional>

/**
 * @brief Helper class for modern Qt signal-slot connections
 * 
 * This class provides utilities for creating modern Qt connections using
 * lambda functions and function pointers, improving type safety and
 * enabling better compile-time error checking.
 * 
 * @since XFB 2.0
 */
class ModernSignalConnections
{
public:
    /**
     * @brief Create a connection with automatic cleanup
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot or lambda to connect to
     * @param type The connection type (default: Qt::AutoConnection)
     * @return The connection object for later disconnection if needed
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot>
    static QMetaObject::Connection connectSafely(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!sender || !receiver) {
            qWarning() << "ModernSignalConnections: Null sender or receiver";
            return QMetaObject::Connection();
        }

        auto connection = QObject::connect(sender, signal, receiver, slot, type);
        if (!connection) {
            qWarning() << "ModernSignalConnections: Failed to create connection";
        }
        
        return connection;
    }

    /**
     * @brief Create a connection with lambda function
     * @param sender The sender object
     * @param signal The signal to connect
     * @param lambda The lambda function to execute
     * @param type The connection type (default: Qt::AutoConnection)
     * @return The connection object for later disconnection if needed
     */
    template<typename Sender, typename Signal, typename Lambda>
    static QMetaObject::Connection connectLambda(
        const Sender* sender,
        Signal signal,
        Lambda lambda,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        if (!sender) {
            qWarning() << "ModernSignalConnections: Null sender";
            return QMetaObject::Connection();
        }

        auto connection = QObject::connect(sender, signal, lambda, type);
        if (!connection) {
            qWarning() << "ModernSignalConnections: Failed to create lambda connection";
        }
        
        return connection;
    }

    /**
     * @brief Create a queued connection for thread-safe communication
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot to connect to
     * @return The connection object for later disconnection if needed
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot>
    static QMetaObject::Connection connectQueued(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot)
    {
        return connectSafely(sender, signal, receiver, slot, Qt::QueuedConnection);
    }

    /**
     * @brief Create a direct connection for immediate execution
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot to connect to
     * @return The connection object for later disconnection if needed
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot>
    static QMetaObject::Connection connectDirect(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot)
    {
        return connectSafely(sender, signal, receiver, slot, Qt::DirectConnection);
    }

    /**
     * @brief Create a single-shot connection that disconnects after first emission
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot to connect to
     * @return The connection object (will be invalid after first emission)
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot>
    static QMetaObject::Connection connectSingleShot(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot)
    {
        return connectSafely(sender, signal, receiver, slot, 
                           static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
    }

    /**
     * @brief Create a single-shot lambda connection
     * @param sender The sender object
     * @param signal The signal to connect
     * @param lambda The lambda function to execute once
     * @return The connection object (will be invalid after first emission)
     */
    template<typename Sender, typename Signal, typename Lambda>
    static QMetaObject::Connection connectSingleShotLambda(
        const Sender* sender,
        Signal signal,
        Lambda lambda)
    {
        return connectLambda(sender, signal, lambda, 
                           static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::SingleShotConnection));
    }

    /**
     * @brief Safely disconnect a connection
     * @param connection The connection to disconnect
     * @return true if disconnection was successful
     */
    static bool disconnectSafely(const QMetaObject::Connection& connection)
    {
        if (connection) {
            return QObject::disconnect(connection);
        }
        return true; // Already disconnected
    }

    /**
     * @brief Disconnect all connections from a sender to a receiver
     * @param sender The sender object
     * @param receiver The receiver object
     * @return true if disconnection was successful
     */
    static bool disconnectAll(const QObject* sender, const QObject* receiver = nullptr)
    {
        if (!sender) {
            qWarning() << "ModernSignalConnections: Null sender for disconnectAll";
            return false;
        }
        
        return QObject::disconnect(sender, nullptr, receiver, nullptr);
    }

    /**
     * @brief Create a connection with error handling
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot to connect to
     * @param errorHandler Function to call if connection fails
     * @return The connection object for later disconnection if needed
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot, typename ErrorHandler>
    static QMetaObject::Connection connectWithErrorHandling(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot,
        ErrorHandler errorHandler)
    {
        auto connection = connectSafely(sender, signal, receiver, slot);
        if (!connection) {
            errorHandler();
        }
        return connection;
    }
};

/**
 * @brief RAII wrapper for managing Qt connections
 * 
 * This class automatically disconnects connections when destroyed,
 * ensuring proper cleanup and preventing dangling connections.
 */
class ConnectionManager
{
public:
    ConnectionManager() = default;
    
    ~ConnectionManager()
    {
        disconnectAll();
    }

    // Non-copyable but movable
    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;
    
    ConnectionManager(ConnectionManager&& other) noexcept
        : m_connections(std::move(other.m_connections))
    {
    }
    
    ConnectionManager& operator=(ConnectionManager&& other) noexcept
    {
        if (this != &other) {
            disconnectAll();
            m_connections = std::move(other.m_connections);
        }
        return *this;
    }

    /**
     * @brief Add a connection to be managed
     * @param connection The connection to manage
     */
    void addConnection(const QMetaObject::Connection& connection)
    {
        if (connection) {
            m_connections.append(connection);
        }
    }

    /**
     * @brief Create and manage a connection
     * @param sender The sender object
     * @param signal The signal to connect
     * @param receiver The receiver object
     * @param slot The slot to connect to
     * @param type The connection type
     * @return The created connection
     */
    template<typename Sender, typename Signal, typename Receiver, typename Slot>
    QMetaObject::Connection connect(
        const Sender* sender,
        Signal signal,
        const Receiver* receiver,
        Slot slot,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        auto connection = ModernSignalConnections::connectSafely(sender, signal, receiver, slot, type);
        addConnection(connection);
        return connection;
    }

    /**
     * @brief Create and manage a lambda connection
     * @param sender The sender object
     * @param signal The signal to connect
     * @param lambda The lambda function to execute
     * @param type The connection type
     * @return The created connection
     */
    template<typename Sender, typename Signal, typename Lambda>
    QMetaObject::Connection connectLambda(
        const Sender* sender,
        Signal signal,
        Lambda lambda,
        Qt::ConnectionType type = Qt::AutoConnection)
    {
        auto connection = ModernSignalConnections::connectLambda(sender, signal, lambda, type);
        addConnection(connection);
        return connection;
    }

    /**
     * @brief Disconnect all managed connections
     */
    void disconnectAll()
    {
        for (const auto& connection : m_connections) {
            ModernSignalConnections::disconnectSafely(connection);
        }
        m_connections.clear();
    }

    /**
     * @brief Get the number of managed connections
     * @return Number of active connections
     */
    int connectionCount() const
    {
        return m_connections.size();
    }

private:
    QList<QMetaObject::Connection> m_connections;
};

#endif // MODERNSIGNALCONNECTIONS_H