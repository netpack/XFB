# XFB Development Guide

This document provides information for developers working on the XFB radio automation software.

## Development Environment Setup

### Prerequisites

#### Required Software
- **Qt 6.5.0 or later** with the following modules:
  - Qt Core, GUI, Widgets
  - Qt Multimedia
  - Qt SQL
  - Qt Network
  - Qt WebEngine
  - Qt Test (for testing)
- **CMake 3.16 or later**
- **C++17 compatible compiler**:
  - GCC 8+ (Linux)
  - Clang 10+ (macOS/Linux)
  - MSVC 2019+ (Windows)

#### Optional Tools (Recommended)
- **clang-tidy** - Static analysis
- **cppcheck** - Additional static analysis
- **clang-format** - Code formatting
- **Doxygen** - Documentation generation
- **Git** - Version control

### Platform-Specific Setup

#### Ubuntu/Debian
```bash
# Install Qt6 and development tools
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    qt6-base-dev \
    qt6-multimedia-dev \
    qt6-webengine-dev \
    libqt6sql6-sqlite \
    clang-tidy \
    cppcheck \
    clang-format \
    doxygen \
    graphviz

# For audio support
sudo apt install -y \
    libasound2-dev \
    libpulse-dev
```

#### macOS
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt6 and development tools
brew install qt6 cmake ninja llvm cppcheck doxygen graphviz

# Add LLVM to PATH for clang-tidy
echo 'export PATH="/usr/local/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

#### Windows
1. Install Qt6 from [Qt official website](https://www.qt.io/download)
2. Install Visual Studio 2019 or later with C++ support
3. Install CMake from [cmake.org](https://cmake.org/download/)
4. Optional: Install Git for Windows

### Building the Project

#### Quick Start
```bash
# Clone the repository
git clone <repository-url>
cd XFB

# Build using the build script (Linux/macOS)
./scripts/build.sh

# Or build manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

#### Build Options
```bash
# Debug build with tests
./scripts/build.sh -d -t

# Clean build with static analysis
./scripts/build.sh -c -a

# Custom CMake arguments
./scripts/build.sh --cmake-args="-DCMAKE_INSTALL_PREFIX=/usr/local"
```

#### Windows Build
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Project Structure

```
XFB/
├── src/                    # Source code
│   ├── *.cpp              # Implementation files
│   ├── *.h                # Header files
│   ├── *.ui               # Qt Designer files
│   └── CMakeLists.txt     # Source build configuration
├── tests/                 # Test suite
│   ├── unit/              # Unit tests
│   ├── integration/       # Integration tests
│   └── test_utils.*       # Test utilities
├── docs/                  # Documentation
├── scripts/               # Build and utility scripts
├── cmake/                 # CMake modules
├── .github/workflows/     # CI/CD configuration
├── CMakeLists.txt         # Main build configuration
└── README.md             # Project overview
```

## Development Workflow

### Code Style
The project uses clang-format for consistent code formatting. Configuration is in `.clang-format`.

```bash
# Format all source files
find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying files
find src tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### Static Analysis
Run static analysis tools to catch potential issues:

```bash
# Run all static analysis tools
./scripts/static_analysis.sh

# Individual tools
clang-tidy src/*.cpp -p build/
cppcheck --project=cppcheck.xml
```

### Testing

#### Running Tests
```bash
# Build and run all tests
./scripts/build.sh -t

# Run tests manually
cd build
ctest --output-on-failure --verbose

# Run specific test
./tests/unit/test_config
```

#### Writing Tests
- Unit tests go in `tests/unit/`
- Integration tests go in `tests/integration/`
- Use Qt Test framework
- Inherit from `XFBTestBase` for common setup

Example test:
```cpp
#include <QtTest/QtTest>
#include "test_utils.h"

class TestMyClass : public XFBTestBase
{
    Q_OBJECT

private slots:
    void testBasicFunctionality();
    void testErrorHandling();
};

void TestMyClass::testBasicFunctionality()
{
    // Test implementation
    QVERIFY(true);
}

QTEST_MAIN(TestMyClass)
#include "test_myclass.moc"
```

### Documentation

#### Code Documentation
Use Doxygen-style comments for all public APIs:

```cpp
/**
 * @brief Brief description of the function
 * 
 * Detailed description of what the function does,
 * its parameters, return value, and any side effects.
 * 
 * @param parameter Description of parameter
 * @return Description of return value
 * @throws ExceptionType When this exception is thrown
 * 
 * @example
 * @code
 * MyClass obj;
 * obj.myFunction(42);
 * @endcode
 * 
 * @see RelatedFunction, RelatedClass
 * @since Version 2.0
 */
void myFunction(int parameter);
```

#### Generating Documentation
```bash
# Generate API documentation (when Doxygen is configured)
doxygen Doxyfile
```

## Continuous Integration

The project uses GitHub Actions for CI/CD:

- **Build and Test**: Builds on Linux, macOS, and Windows
- **Static Analysis**: Runs clang-tidy and cppcheck
- **Code Quality**: Checks formatting and complexity
- **Security Scan**: Basic security checks
- **Documentation**: Generates and deploys documentation
- **Packaging**: Creates distribution packages

## Debugging

### Debug Builds
```bash
# Build in debug mode
./scripts/build.sh -d

# With debugging symbols and sanitizers (Linux/macOS)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address -g"
```

### Qt Creator Integration
1. Open `CMakeLists.txt` in Qt Creator
2. Configure with your Qt6 installation
3. Build and run from the IDE

### Common Issues

#### Qt6 Not Found
```bash
# Set Qt6 path explicitly
export Qt6_DIR=/path/to/qt6/lib/cmake/Qt6
cmake .. -DQt6_DIR=$Qt6_DIR
```

#### Missing Dependencies
```bash
# Check what libraries are missing (Linux)
ldd build/bin/XFB

# macOS
otool -L build/src/XFB.app/Contents/MacOS/XFB
```

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Make your changes following the coding standards
4. Add tests for new functionality
5. Run static analysis and tests
6. Commit with descriptive messages
7. Push to your fork and create a pull request

### Commit Message Format
```
type(scope): brief description

Detailed description of the changes made.

- List specific changes
- Reference issues if applicable

Fixes #123
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

## Getting Help

- Check the [main README](../README.md) for general information
- Review existing issues and discussions
- Create an issue for bugs or feature requests
- Contact the maintainers for questions

## Architecture Overview

The XFB application follows a layered architecture:

1. **Presentation Layer**: Qt Widgets UI components
2. **Business Logic Layer**: Service classes for core functionality
3. **Data Access Layer**: Repository pattern for database operations
4. **Infrastructure Layer**: Database, file system, and hardware interfaces

For detailed architecture information, see the [Design Document](../.kiro/specs/xfb-optimization-and-documentation/design.md).