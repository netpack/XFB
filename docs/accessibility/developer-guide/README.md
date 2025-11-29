# XFB Accessibility System Developer Guide

## Overview

This guide provides comprehensive documentation for developers working with the XFB accessibility system. The system provides full ORCA screen reader integration, keyboard navigation, audio feedback, and comprehensive accessibility features for visually impaired users.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Core Components](#core-components)
3. [Integration Patterns](#integration-patterns)
4. [Best Practices](#best-practices)
5. [Testing Guidelines](#testing-guidelines)
6. [Performance Considerations](#performance-considerations)
7. [Troubleshooting](#troubleshooting)

## Architecture Overview

The XFB accessibility system is built on Qt6's accessibility framework and follows a layered architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                    XFB Application Layer                     │
├─────────────────────────────────────────────────────────────┤
│                 Accessibility Service Layer                 │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐ │
│  │Accessibility│ │  Keyboard   │ │    Audio Feedback       │ │
│  │  Manager    │ │ Navigation  │ │      Service            │ │
│  └─────────────┘ └─────────────┘ └─────────────────────────┘ │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐ │
│  │   Widget    │ │Live Region  │ │   Settings Service      │ │
│  │ Enhancer    │ │  Manager    │ │                         │ │
│  └─────────────┘ └─────────────┘ └─────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                    Qt6 Accessibility Layer                  │
│         QAccessible | QAccessibleInterface | AT-SPI         │
├─────────────────────────────────────────────────────────────┤
│                      System Layer                           │
│                   ORCA Screen Reader                        │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### AccessibilityManager

The central coordinator for all accessibility features. This singleton class manages:

- Initialization and configuration of accessibility services
- Coordination between different accessibility components
- Settings management and persistence
- Application lifecycle integration

**Key Methods:**
- `instance()` - Get singleton instance
- `initialize()` - Initialize accessibility system
- `enableAccessibility(bool)` - Enable/disable accessibility
- `registerAccessibleWidget()` - Register widgets for enhancement

### WidgetAccessibilityEnhancer

Automatically enhances Qt widgets with proper accessibility attributes:

- Automatic widget discovery through event filtering
- Role and state management
- Custom accessible interfaces for complex widgets
- Grid and table accessibility enhancements

**Integration Pattern:**
```cpp
// Automatic enhancement through event filter
WidgetAccessibilityEnhancer* enhancer = new WidgetAccessibilityEnhancer(this);
enhancer->initialize(); // Installs application-wide event filter

// Manual enhancement for specific widgets
enhancer->enhanceWidget(myWidget);
enhancer->setAccessibleName(myWidget, "Descriptive Name");
```

### KeyboardNavigationController

Provides comprehensive keyboard navigation throughout the application:

- Custom tab order management
- Arrow key navigation in complex widgets
- Keyboard shortcuts for all major functions
- Focus indication and management

**Navigation Patterns:**
- Tab/Shift+Tab: Logical flow through interactive elements
- Arrow keys: Navigation within grids, lists, and complex controls
- Shortcuts: Direct access to major functions
- Escape: Consistent cancel/close behavior

### AudioFeedbackService

Delivers immediate audible feedback for user actions:

- Action confirmation announcements
- Error message delivery
- Progress updates for long operations
- Customizable announcement timing

**Feedback Categories:**
- Immediate: Button clicks, menu selections
- Confirmational: Save operations, playlist additions
- Informational: Status changes, progress updates
- Critical: Error messages, important alerts

### LiveRegionManager

Handles real-time content updates and live regions:

- Playback status announcements
- Time remaining updates
- Queue and playlist changes
- System status notifications

## Integration Patterns

### Adding Accessibility to New Widgets

When creating new UI components, follow these patterns:

1. **Automatic Enhancement (Recommended):**
```cpp
// Widget will be automatically enhanced when shown
MyCustomWidget* widget = new MyCustomWidget(parent);
widget->show(); // Enhancement happens automatically
```

2. **Manual Enhancement:**
```cpp
MyCustomWidget* widget = new MyCustomWidget(parent);
AccessibilityManager::instance()->registerAccessibleWidget(
    widget, 
    AccessibilityMetadata{
        .name = "Custom Widget",
        .description = "Performs custom function",
        .role = QAccessible::Button,
        .helpText = "Press Enter to activate"
    }
);
```

3. **Custom Accessible Interface:**
```cpp
class MyAccessibleInterface : public QAccessibleWidget {
public:
    MyAccessibleInterface(MyCustomWidget* widget) 
        : QAccessibleWidget(widget) {}
    
    QString text(QAccessible::Text t) const override {
        switch (t) {
        case QAccessible::Name:
            return "Custom Widget Name";
        case QAccessible::Description:
            return "Detailed description of functionality";
        default:
            return QAccessibleWidget::text(t);
        }
    }
    
    QAccessible::State state() const override {
        QAccessible::State state = QAccessibleWidget::state();
        if (widget()->isEnabled()) {
            state.focusable = true;
        }
        return state;
    }
};

// Register the interface
QAccessible::installFactory([](const QString &classname, QObject *object) -> QAccessibleInterface* {
    if (classname == "MyCustomWidget") {
        return new MyAccessibleInterface(qobject_cast<MyCustomWidget*>(object));
    }
    return nullptr;
});
```

### Adding Audio Feedback

To add audio feedback for user actions:

```cpp
// Connect to existing signals
connect(myButton, &QPushButton::clicked, [this]() {
    AudioFeedbackService::instance()->announceAction("Button activated");
});

// For complex operations
connect(myService, &MyService::operationCompleted, [this](bool success) {
    if (success) {
        AudioFeedbackService::instance()->announceSuccess("Operation completed successfully");
    } else {
        AudioFeedbackService::instance()->announceError("Operation failed");
    }
});
```

### Adding Keyboard Navigation

For custom widgets requiring special keyboard handling:

```cpp
class MyCustomWidget : public QWidget {
protected:
    void keyPressEvent(QKeyEvent* event) override {
        KeyboardNavigationController* nav = KeyboardNavigationController::instance();
        
        switch (event->key()) {
        case Qt::Key_Tab:
            nav->handleTabNavigation(this, event);
            break;
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
            nav->handleArrowNavigation(this, event);
            break;
        default:
            QWidget::keyPressEvent(event);
        }
    }
};
```

### Adding Live Regions

For dynamic content that should be announced:

```cpp
// Register a live region
LiveRegionManager* liveManager = LiveRegionManager::instance();
liveManager->createLiveRegion(statusLabel, QAccessible::LiveRegion::Polite);

// Update content (will be announced automatically)
statusLabel->setText("New status information");
liveManager->updateLiveRegion(statusLabel, "New status information");
```

## Best Practices

### 1. Accessibility-First Design

- Consider accessibility requirements during the design phase
- Use semantic HTML-like structures in Qt widgets
- Provide meaningful names and descriptions for all interactive elements
- Ensure logical tab order and keyboard navigation

### 2. Performance Optimization

- Use lazy initialization for accessibility services
- Cache accessibility metadata efficiently
- Avoid blocking operations in accessibility code
- Throttle live region updates to prevent announcement spam

### 3. Error Handling

- Implement graceful degradation when accessibility services fail
- Provide fallback mechanisms for critical functionality
- Log accessibility errors for debugging
- Notify users of accessibility limitations when appropriate

### 4. Testing Integration

- Write unit tests for all accessibility components
- Include accessibility testing in CI/CD pipeline
- Test with actual screen readers (ORCA)
- Validate keyboard-only operation

### 5. Documentation Standards

- Document all accessibility-related code with Doxygen comments
- Include usage examples in documentation
- Explain accessibility implications of design decisions
- Maintain up-to-date integration guides

## Testing Guidelines

### Unit Testing

Test individual accessibility components:

```cpp
class TestAccessibilityManager : public QObject {
    Q_OBJECT
private slots:
    void testInitialization() {
        AccessibilityManager* manager = AccessibilityManager::instance();
        QVERIFY(manager != nullptr);
        
        manager->initialize();
        QVERIFY(manager->isEnabled());
    }
    
    void testWidgetRegistration() {
        QWidget* widget = new QWidget();
        AccessibilityMetadata metadata;
        metadata.name = "Test Widget";
        
        AccessibilityManager::instance()->registerAccessibleWidget(widget, metadata);
        
        QCOMPARE(widget->accessibleName(), QString("Test Widget"));
        delete widget;
    }
};
```

### Integration Testing

Test cross-component functionality:

```cpp
class TestAccessibilityIntegration : public QObject {
    Q_OBJECT
private slots:
    void testKeyboardNavigationWithAudioFeedback() {
        // Create test widgets
        QWidget* parent = new QWidget();
        QPushButton* button1 = new QPushButton("Button 1", parent);
        QPushButton* button2 = new QPushButton("Button 2", parent);
        
        // Initialize accessibility
        AccessibilityManager::instance()->initialize();
        
        // Test navigation
        QTest::keyPress(parent, Qt::Key_Tab);
        QCOMPARE(QApplication::focusWidget(), button1);
        
        QTest::keyPress(parent, Qt::Key_Tab);
        QCOMPARE(QApplication::focusWidget(), button2);
        
        delete parent;
    }
};
```

### ORCA Compatibility Testing

Test with actual screen reader:

```cpp
class TestORCACompatibility : public QObject {
    Q_OBJECT
private slots:
    void testScreenReaderAnnouncements() {
        // This test requires ORCA to be running
        if (!isORCARunning()) {
            QSKIP("ORCA not available for testing");
        }
        
        QWidget* widget = new QWidget();
        widget->setAccessibleName("Test Widget");
        widget->show();
        
        // Verify AT-SPI communication
        QVERIFY(hasATSPIInterface(widget));
        
        delete widget;
    }
};
```

## Performance Considerations

### Memory Management

- Use weak references where possible to avoid circular dependencies
- Clean up accessibility interfaces when widgets are destroyed
- Implement efficient caching for frequently accessed metadata

### CPU Performance

- Process accessibility events asynchronously when possible
- Batch accessibility updates to reduce overhead
- Use efficient data structures for accessibility metadata storage

### Responsiveness

- Ensure accessibility operations don't block the UI thread
- Implement timeout mechanisms for accessibility operations
- Prioritize critical accessibility announcements

## Troubleshooting

### Common Issues

1. **ORCA Not Announcing Widget Changes**
   - Verify AT-SPI bridge is properly configured
   - Check that QAccessibleEvent is being sent
   - Ensure widget has proper accessible name and role

2. **Keyboard Navigation Not Working**
   - Verify focus policy is set correctly
   - Check tab order configuration
   - Ensure event filters are properly installed

3. **Audio Feedback Not Playing**
   - Check audio system configuration
   - Verify AudioFeedbackService is initialized
   - Test with different audio backends

4. **Performance Issues**
   - Profile accessibility code with performance monitor
   - Check for memory leaks in accessibility interfaces
   - Optimize announcement frequency

### Debug Tools

Use the built-in accessibility debugging tools:

```cpp
// Enable accessibility debugging
AccessibilityManager::instance()->setDebugMode(true);

// Monitor performance
AccessibilityPerformanceMonitor* monitor = AccessibilityPerformanceMonitor::instance();
monitor->startMonitoring();

// Validate accessibility implementation
AccessibilityValidator* validator = AccessibilityValidator::instance();
validator->validateWidget(myWidget);
```

### Logging

Enable detailed accessibility logging:

```cpp
// Set log level for accessibility components
Logger::instance()->setLogLevel("Accessibility", Logger::Debug);

// Log accessibility events
ACCESSIBILITY_LOG_DEBUG("Widget enhanced: " << widget->objectName());
```

## Migration Guide

### From Previous Versions

When upgrading from previous XFB versions:

1. Update accessibility service initialization
2. Migrate custom accessibility implementations
3. Update keyboard shortcut configurations
4. Test with latest ORCA version

### Breaking Changes

- AccessibilityManager API changes in v1.0
- KeyboardNavigationController event handling updates
- AudioFeedbackService configuration format changes

## Contributing

### Code Style

Follow the established code style for accessibility components:

- Use descriptive variable and method names
- Include comprehensive Doxygen documentation
- Follow Qt coding conventions
- Add unit tests for new functionality

### Review Process

All accessibility-related changes must:

1. Pass automated accessibility tests
2. Be tested with ORCA screen reader
3. Include updated documentation
4. Have performance impact assessment

## Support

For accessibility-related development questions:

- Check the troubleshooting section
- Review existing test cases for examples
- Consult the API documentation
- Test with actual screen reader users when possible

## References

- [Qt6 Accessibility Documentation](https://doc.qt.io/qt-6/accessible.html)
- [AT-SPI Specification](https://www.freedesktop.org/wiki/Accessibility/AT-SPI2/)
- [ORCA Screen Reader Documentation](https://help.gnome.org/users/orca/stable/)
- [WCAG 2.1 Guidelines](https://www.w3.org/WAI/WCAG21/quickref/)