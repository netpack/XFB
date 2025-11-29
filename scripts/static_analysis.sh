#!/bin/bash

# Static Analysis Script for XFB Project
# This script runs various static analysis tools on the codebase

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
REPORTS_DIR="${PROJECT_ROOT}/reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

echo_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

echo_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

echo_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create reports directory
mkdir -p "${REPORTS_DIR}"

echo_info "Starting static analysis for XFB project..."

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo_warning "Build directory not found. Creating and configuring..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cd "${PROJECT_ROOT}"
fi

# Ensure compile_commands.json exists
if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
    echo_info "Generating compile_commands.json..."
    cd "${BUILD_DIR}"
    cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cd "${PROJECT_ROOT}"
fi

# Run clang-tidy
echo_info "Running clang-tidy analysis..."
if command -v clang-tidy >/dev/null 2>&1; then
    find src -name "*.cpp" -o -name "*.h" | grep -v moc_ | grep -v ui_ | grep -v qrc_ | \
    xargs clang-tidy -p="${BUILD_DIR}" --format-style=file > "${REPORTS_DIR}/clang-tidy-report.txt" 2>&1 || true
    
    if [ -s "${REPORTS_DIR}/clang-tidy-report.txt" ]; then
        echo_warning "clang-tidy found issues. Check ${REPORTS_DIR}/clang-tidy-report.txt"
    else
        echo_success "clang-tidy analysis completed with no issues"
    fi
else
    echo_warning "clang-tidy not found. Skipping clang-tidy analysis."
fi

# Run cppcheck
echo_info "Running cppcheck analysis..."
if command -v cppcheck >/dev/null 2>&1; then
    cppcheck --project=cppcheck.xml \
             --enable=all \
             --inconclusive \
             --xml \
             --xml-version=2 \
             --output-file="${REPORTS_DIR}/cppcheck-report.xml" \
             2>/dev/null || true
    
    # Generate text report from XML
    if command -v xsltproc >/dev/null 2>&1 && [ -f "${REPORTS_DIR}/cppcheck-report.xml" ]; then
        # Simple text extraction from XML (basic approach)
        grep -o 'msg="[^"]*"' "${REPORTS_DIR}/cppcheck-report.xml" | sed 's/msg="//g' | sed 's/"$//g' > "${REPORTS_DIR}/cppcheck-report.txt" 2>/dev/null || true
    fi
    
    if [ -s "${REPORTS_DIR}/cppcheck-report.xml" ]; then
        echo_warning "cppcheck found issues. Check ${REPORTS_DIR}/cppcheck-report.xml"
    else
        echo_success "cppcheck analysis completed with no issues"
    fi
else
    echo_warning "cppcheck not found. Skipping cppcheck analysis."
fi

# Run additional checks if available
echo_info "Running additional static analysis checks..."

# Check for TODO/FIXME comments
echo_info "Checking for TODO/FIXME comments..."
find src tests -type f \( -name "*.cpp" -o -name "*.h" \) -exec grep -Hn "TODO\|FIXME\|XXX\|HACK" {} \; > "${REPORTS_DIR}/todo-fixme-report.txt" 2>/dev/null || true

if [ -s "${REPORTS_DIR}/todo-fixme-report.txt" ]; then
    echo_warning "Found TODO/FIXME comments. Check ${REPORTS_DIR}/todo-fixme-report.txt"
else
    echo_success "No TODO/FIXME comments found"
fi

# Check for potential security issues (basic patterns)
echo_info "Checking for potential security issues..."
{
    echo "=== Potential Security Issues ==="
    echo ""
    echo "Checking for unsafe string functions..."
    find src -name "*.cpp" -exec grep -Hn "strcpy\|strcat\|sprintf\|gets" {} \; 2>/dev/null || true
    echo ""
    echo "Checking for SQL injection patterns..."
    find src -name "*.cpp" -exec grep -Hn "exec.*+\|query.*+\|SELECT.*+\|INSERT.*+\|UPDATE.*+\|DELETE.*+" {} \; 2>/dev/null || true
    echo ""
    echo "Checking for hardcoded passwords/keys..."
    find src -name "*.cpp" -o -name "*.h" | xargs grep -i "password\|secret\|key" | grep -v "//\|/\*" 2>/dev/null || true
} > "${REPORTS_DIR}/security-check-report.txt"

if [ -s "${REPORTS_DIR}/security-check-report.txt" ]; then
    echo_warning "Potential security issues found. Check ${REPORTS_DIR}/security-check-report.txt"
else
    echo_success "No obvious security issues found"
fi

# Generate summary report
echo_info "Generating summary report..."
{
    echo "XFB Static Analysis Summary Report"
    echo "=================================="
    echo "Generated on: $(date)"
    echo ""
    
    echo "Files analyzed:"
    find src -name "*.cpp" -o -name "*.h" | grep -v moc_ | grep -v ui_ | grep -v qrc_ | wc -l
    echo ""
    
    if [ -f "${REPORTS_DIR}/clang-tidy-report.txt" ]; then
        echo "Clang-tidy issues: $(wc -l < "${REPORTS_DIR}/clang-tidy-report.txt")"
    fi
    
    if [ -f "${REPORTS_DIR}/cppcheck-report.xml" ]; then
        echo "Cppcheck issues: $(grep -c '<error' "${REPORTS_DIR}/cppcheck-report.xml" 2>/dev/null || echo "0")"
    fi
    
    if [ -f "${REPORTS_DIR}/todo-fixme-report.txt" ]; then
        echo "TODO/FIXME comments: $(wc -l < "${REPORTS_DIR}/todo-fixme-report.txt")"
    fi
    
    echo ""
    echo "Detailed reports available in: ${REPORTS_DIR}/"
    echo "- clang-tidy-report.txt"
    echo "- cppcheck-report.xml"
    echo "- todo-fixme-report.txt"
    echo "- security-check-report.txt"
    
} > "${REPORTS_DIR}/summary-report.txt"

echo_success "Static analysis completed!"
echo_info "Summary report: ${REPORTS_DIR}/summary-report.txt"
echo_info "All reports saved in: ${REPORTS_DIR}/"

# Return appropriate exit code
if [ -s "${REPORTS_DIR}/clang-tidy-report.txt" ] || [ -s "${REPORTS_DIR}/cppcheck-report.xml" ]; then
    echo_warning "Static analysis found issues. Please review the reports."
    exit 1
else
    echo_success "Static analysis completed with no critical issues."
    exit 0
fi