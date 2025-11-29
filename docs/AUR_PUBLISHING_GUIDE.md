# XFB AUR Publishing Guide

This guide explains how to publish and maintain the XFB package on the Arch User Repository (AUR).

## Prerequisites

Before publishing to AUR, you need:

1. **An Arch Linux account**: Register at https://aur.archlinux.org/register
2. **SSH key configured**: Add your SSH public key to your AUR account
3. **Git installed**: `sudo pacman -S git`
4. **Base development tools**: `sudo pacman -S base-devel`

## Initial Setup

### 1. Configure Git for AUR

```bash
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"
```

### 2. Set up SSH for AUR

Generate an SSH key if you don't have one:

```bash
ssh-keygen -t ed25519 -C "your.email@example.com"
```

Add your public key (`~/.ssh/id_ed25519.pub`) to your AUR account at:
https://aur.archlinux.org/account/

Test your connection:

```bash
ssh -T aur@aur.archlinux.org
```

## Publishing to AUR

### Step 1: Clone the AUR Repository

For a new package:

```bash
git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
cd aur-xfb
```

### Step 2: Copy Package Files

Copy the PKGBUILD from the XFB repository:

```bash
cp /path/to/XFB/PKGBUILD .
```

### Step 3: Generate .SRCINFO

The `.SRCINFO` file is required by AUR and contains metadata about your package:

```bash
makepkg --printsrcinfo > .SRCINFO
```

### Step 4: Test the Package Locally

Before publishing, always test your package:

```bash
# Build the package
makepkg -si

# Or use the test script
cd /path/to/XFB
./test-arch-install.sh
```

### Step 5: Commit and Push to AUR

```bash
git add PKGBUILD .SRCINFO
git commit -m "Initial release of XFB 2.0.0"
git push origin master
```

## Updating the Package

When releasing a new version:

### 1. Update PKGBUILD

Edit the `PKGBUILD` file:

```bash
pkgver=2.0.1  # Update version
pkgrel=1      # Reset to 1 for new version
```

### 2. Update Checksums

If you're using checksums (not SKIP):

```bash
updpkgsums
```

### 3. Regenerate .SRCINFO

```bash
makepkg --printsrcinfo > .SRCINFO
```

### 4. Test the Update

```bash
makepkg -si
```

### 5. Commit and Push

```bash
git add PKGBUILD .SRCINFO
git commit -m "Update to version 2.0.1"
git push
```

## Package Maintenance

### Responding to Comments

- Monitor the AUR page for user comments: https://aur.archlinux.org/packages/xfb
- Respond to bug reports and feature requests
- Mark comments as resolved when fixed

### Handling Out-of-Date Flags

Users can flag packages as out-of-date. When this happens:

1. Update the PKGBUILD with the new version
2. Test the package
3. Push the update
4. The flag will be automatically cleared

### Orphaning or Disowning

If you can no longer maintain the package:

1. Go to https://aur.archlinux.org/packages/xfb
2. Click "Disown Package"
3. Optionally, find a new maintainer first

## Testing on Different Arch-Based Distributions

### Manjaro

```bash
# Update system
sudo pacman -Syu

# Install from AUR
pamac build xfb
# or
yay -S xfb
```

### EndeavourOS

```bash
# Update system
sudo pacman -Syu

# Install from AUR
yay -S xfb
```

### Garuda Linux

```bash
# Update system
sudo pacman -Syu

# Install from AUR
paru -S xfb
```

## Testing in a Clean Environment

Use a container or VM to test in a clean Arch environment:

### Using Docker

```bash
# Create a Dockerfile
cat > Dockerfile.arch << 'EOF'
FROM archlinux:latest

RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm base-devel git sudo

RUN useradd -m -G wheel builder && \
    echo "builder ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER builder
WORKDIR /home/builder

CMD ["/bin/bash"]
EOF

# Build and run
docker build -t arch-test -f Dockerfile.arch .
docker run -it --rm -v $(pwd):/xfb arch-test

# Inside container
cd /xfb
./test-arch-install.sh
```

### Using QEMU/KVM

1. Download Arch ISO: https://archlinux.org/download/
2. Create a VM with at least 2GB RAM and 20GB disk
3. Install Arch Linux
4. Test the package installation

## Common Issues and Solutions

### Issue: "Unknown public key" error

**Solution**: Import the required GPG keys:

```bash
gpg --recv-keys <KEY_ID>
```

### Issue: Dependency conflicts

**Solution**: Update the `depends` array in PKGBUILD to match current Arch package names.

### Issue: Build fails on different architectures

**Solution**: Test on both x86_64 and aarch64 if possible, or limit `arch` array:

```bash
arch=('x86_64')  # Only support x86_64
```

### Issue: Qt6 library not found

**Solution**: Ensure all Qt6 dependencies are listed:

```bash
depends=(
    'qt6-base'
    'qt6-multimedia'
    'qt6-webengine'
    ...
)
```

## Best Practices

1. **Always test before pushing**: Use `makepkg -si` to test locally
2. **Keep PKGBUILD clean**: Follow Arch packaging standards
3. **Update regularly**: Keep the package in sync with upstream releases
4. **Respond to users**: Address comments and issues promptly
5. **Document changes**: Use clear commit messages
6. **Version bumps**: Increment `pkgrel` for packaging changes, update `pkgver` for new releases

## Resources

- **AUR Submission Guidelines**: https://wiki.archlinux.org/title/AUR_submission_guidelines
- **PKGBUILD Reference**: https://wiki.archlinux.org/title/PKGBUILD
- **Arch Package Guidelines**: https://wiki.archlinux.org/title/Arch_package_guidelines
- **AUR Helpers**: https://wiki.archlinux.org/title/AUR_helpers

## Automation

Consider setting up GitHub Actions to automatically test the PKGBUILD:

```yaml
# .github/workflows/test-pkgbuild.yml
name: Test PKGBUILD

on:
  push:
    paths:
      - 'PKGBUILD'
  pull_request:
    paths:
      - 'PKGBUILD'

jobs:
  test:
    runs-on: ubuntu-latest
    container:
      image: archlinux:latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Update system
        run: pacman -Syu --noconfirm
      
      - name: Install dependencies
        run: pacman -S --noconfirm base-devel git
      
      - name: Test PKGBUILD
        run: |
          useradd -m builder
          chown -R builder:builder .
          sudo -u builder makepkg --syncdeps --noconfirm
```

## Contact

For questions about AUR publishing:
- **Maintainer**: Netpack <info@netpack.pt>
- **AUR Package**: https://aur.archlinux.org/packages/xfb
- **GitHub Issues**: https://github.com/netpack/XFB/issues
