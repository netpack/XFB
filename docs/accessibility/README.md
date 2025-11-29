# XFB Accessibility Documentation

## Overview

This directory contains comprehensive documentation for XFB Radio Broadcasting Software's accessibility features, designed to support visually impaired users through ORCA screen reader integration and other assistive technologies.

## Documentation Structure

### For Users

**[User Guide](user-guide/README.md)** - Complete guide for end users
- [Quick Setup Guide](user-guide/quick-setup.md) - Get started in 5 minutes
- [Keyboard Reference](user-guide/keyboard-reference.md) - Complete shortcut reference
- [Troubleshooting Guide](user-guide/troubleshooting.md) - Common issues and solutions

### For Developers

**[Developer Guide](developer-guide/README.md)** - Technical documentation for developers
- [Integration Patterns](developer-guide/integration-patterns.md) - Code patterns and examples
- [Best Practices](developer-guide/best-practices.md) - Development guidelines
- [API Documentation](developer-guide/api/) - Generated from Doxygen (run `doxygen Doxyfile`)

### For System Administrators

**[Deployment Guide](deployment/README.md)** - Installation and deployment
- [System Requirements](deployment/system-requirements.md) - Hardware and software requirements
- [Installation Guide](deployment/installation-guide.md) - Step-by-step installation

## Quick Links

### Getting Started
- **New Users**: Start with [Quick Setup Guide](user-guide/quick-setup.md)
- **Developers**: Begin with [Developer Guide](developer-guide/README.md)
- **Administrators**: See [Installation Guide](deployment/installation-guide.md)

### Common Tasks
- **Keyboard Shortcuts**: [Keyboard Reference](user-guide/keyboard-reference.md)
- **Troubleshooting**: [Troubleshooting Guide](user-guide/troubleshooting.md)
- **Integration**: [Integration Patterns](developer-guide/integration-patterns.md)

### Technical Reference
- **API Documentation**: Generate with `cd docs/accessibility && doxygen Doxyfile`
- **System Requirements**: [System Requirements](deployment/system-requirements.md)
- **Best Practices**: [Best Practices](developer-guide/best-practices.md)

## Accessibility Features

XFB provides comprehensive accessibility support including:

### Screen Reader Integration
- Full ORCA screen reader compatibility
- Comprehensive keyboard navigation
- Audio feedback for all user actions
- Live region updates for dynamic content

### Keyboard Accessibility
- Complete keyboard-only operation
- Logical tab order throughout interface
- Customizable keyboard shortcuts
- Arrow key navigation in complex widgets

### Audio Feedback
- Action confirmations and status updates
- Progress announcements for long operations
- Error notifications and critical alerts
- Customizable verbosity levels

### Braille Display Support
- BrlTTY integration for braille displays
- Formatted braille output for track information
- Navigation support through braille routing

### Customization Options
- Adjustable verbosity levels (Terse/Normal/Verbose)
- Customizable keyboard shortcuts
- Audio feedback preferences
- Braille display configuration

## System Requirements

### Minimum Requirements
- **OS**: Ubuntu 20.04 LTS or compatible Linux distribution
- **Screen Reader**: ORCA 3.36 or newer
- **Memory**: 4 GB RAM
- **Storage**: 2 GB available space

### Recommended Setup
- **OS**: Ubuntu 22.04 LTS with GNOME desktop
- **Screen Reader**: ORCA 40.0 or newer
- **Memory**: 16 GB RAM
- **Storage**: SSD with 10 GB available space

For complete requirements, see [System Requirements](deployment/system-requirements.md).

## Installation

### Quick Installation (Ubuntu/Debian)
```bash
# Add XFB repository
wget -qO - https://packages.xfb.com/gpg-key.asc | sudo gpg --dearmor -o /usr/share/keyrings/xfb-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/xfb-archive-keyring.gpg] https://packages.xfb.com/ubuntu $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Install XFB with accessibility support
sudo apt update
sudo apt install xfb

# Launch XFB
xfb --accessibility-mode
```

For detailed installation instructions, see [Installation Guide](deployment/installation-guide.md).

## Getting Help

### Documentation
- **User Questions**: Check [User Guide](user-guide/README.md) and [Troubleshooting](user-guide/troubleshooting.md)
- **Development Help**: See [Developer Guide](developer-guide/README.md)
- **Installation Issues**: Consult [Installation Guide](deployment/installation-guide.md)

### Community Support
- **Forum**: XFB Accessibility Community Forum
- **Mailing List**: accessibility@xfb.com
- **Bug Reports**: Use built-in bug reporter (`Ctrl+Shift+B` in XFB)

### Professional Support
- **Email**: support@xfb.com
- **Emergency**: Critical broadcasting issues hotline
- **Training**: Professional accessibility training available

## Contributing

### Documentation Contributions
- Report documentation issues or gaps
- Submit improvements and corrections
- Translate documentation to other languages
- Share usage examples and tips

### Code Contributions
- Follow accessibility coding standards in [Best Practices](developer-guide/best-practices.md)
- Include accessibility tests with new features
- Update documentation for accessibility changes
- Test with actual screen reader users when possible

### Testing and Feedback
- Test with different assistive technologies
- Report accessibility bugs and issues
- Provide feedback on user experience
- Participate in accessibility user testing

## Accessibility Standards Compliance

XFB accessibility features are designed to comply with:

- **WCAG 2.1 AA**: Web Content Accessibility Guidelines
- **Section 508**: US Federal accessibility standards
- **EN 301 549**: European accessibility standard
- **AT-SPI**: Assistive Technology Service Provider Interface

## Version Information

- **Documentation Version**: 1.0
- **XFB Version Compatibility**: 2.0+
- **Last Updated**: 2024
- **Maintained By**: XFB Accessibility Team

## License

This documentation is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License. The XFB software is licensed under the GNU General Public License v3.0.

---

**Note**: This documentation is actively maintained. For the most current information, check the online documentation at https://docs.xfb.com/accessibility/