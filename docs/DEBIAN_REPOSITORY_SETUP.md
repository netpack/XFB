# Setting Up a Debian Repository for XFB

This guide explains how to set up a Debian repository so users can install XFB with `apt install xfb`.

## Overview

There are several options for hosting a Debian repository:

1. **GitHub Releases** (Easiest) - Users download .deb manually
2. **GitHub Pages** (Simple) - Host repository on GitHub Pages
3. **PPA (Launchpad)** (Ubuntu-specific) - Official Ubuntu PPA
4. **Self-hosted** (Advanced) - Host on your own server
5. **Third-party services** (Cloudsmith, Packagecloud) - Managed hosting

## Option 1: GitHub Releases (Current - Simplest)

This is what you're currently doing. Users download the .deb file.

### Advantages
- No infrastructure needed
- Simple to maintain
- Works immediately

### Disadvantages
- Users can't use `apt install xfb`
- No automatic updates
- Manual download required

### Current Usage
```bash
# Download from releases
wget https://github.com/netpack/XFB/releases/download/v2.0.0/xfb_2.0.0-1_amd64.deb

# Install
sudo apt install ./xfb_2.0.0-1_amd64.deb
```

## Option 2: GitHub Pages Repository (Recommended)

Host a Debian repository on GitHub Pages for free.

### Setup Steps

#### 1. Create Repository Structure

```bash
# Create debian repository structure
mkdir -p debian-repo/pool/main
mkdir -p debian-repo/dists/stable/main/binary-amd64

# Copy .deb file
cp xfb_2.0.0-1_amd64.deb debian-repo/pool/main/
```

#### 2. Generate Repository Metadata

```bash
cd debian-repo

# Generate Packages file
dpkg-scanpackages pool/main /dev/null | gzip -9c > dists/stable/main/binary-amd64/Packages.gz
dpkg-scanpackages pool/main /dev/null > dists/stable/main/binary-amd64/Packages

# Generate Release file
cd dists/stable
cat > Release << EOF
Origin: XFB
Label: XFB Radio Automation
Suite: stable
Codename: stable
Version: 2.0
Architectures: amd64 arm64
Components: main
Description: XFB Radio Automation Software Repository
Date: $(date -R)
EOF

# Add checksums
apt-ftparchive release . >> Release
```

#### 3. Sign the Repository (Optional but Recommended)

```bash
# Generate GPG key if you don't have one
gpg --full-generate-key

# Export public key
gpg --armor --export YOUR_EMAIL > ../../../xfb-archive-keyring.gpg

# Sign Release file
gpg --default-key YOUR_EMAIL -abs -o Release.gpg Release
gpg --default-key YOUR_EMAIL --clearsign -o InRelease Release
```

#### 4. Push to GitHub Pages

```bash
# In your XFB repository
git checkout --orphan gh-pages
git rm -rf .
cp -r debian-repo/* .
git add .
git commit -m "Initial Debian repository"
git push origin gh-pages
```

#### 5. Enable GitHub Pages

1. Go to repository Settings → Pages
2. Select `gh-pages` branch
3. Save

### User Installation

Users add your repository:

```bash
# Add repository
echo "deb [trusted=yes] https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update and install
sudo apt update
sudo apt install xfb
```

With GPG signing:

```bash
# Add GPG key
wget -qO - https://netpack.github.io/XFB/xfb-archive-keyring.gpg | sudo apt-key add -

# Add repository
echo "deb https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update and install
sudo apt update
sudo apt install xfb
```

## Option 3: Ubuntu PPA (Launchpad)

Official Ubuntu PPA for Ubuntu users.

### Setup Steps

#### 1. Create Launchpad Account

1. Go to https://launchpad.net
2. Create account
3. Create PPA: https://launchpad.net/~your-username/+activate-ppa

#### 2. Prepare Source Package

```bash
# Build source package
debuild -S -sa

# Sign with GPG
debsign xfb_2.0.0-1_source.changes
```

#### 3. Upload to PPA

```bash
# Upload
dput ppa:your-username/xfb xfb_2.0.0-1_source.changes
```

### User Installation

```bash
sudo add-apt-repository ppa:your-username/xfb
sudo apt update
sudo apt install xfb
```

### Advantages
- Official Ubuntu integration
- Automatic building for multiple Ubuntu versions
- Trusted by users

### Disadvantages
- Ubuntu only (not Debian)
- Requires source package
- More complex setup

## Option 4: Self-Hosted Repository

Host on your own server (netpack.pt).

### Setup on Server

```bash
# Install required tools
sudo apt install dpkg-dev apt-utils

# Create repository structure
mkdir -p /var/www/html/debian/pool/main
mkdir -p /var/www/html/debian/dists/stable/main/binary-amd64

# Copy .deb files
cp xfb_*.deb /var/www/html/debian/pool/main/

# Generate Packages file
cd /var/www/html/debian
dpkg-scanpackages pool/main /dev/null | gzip -9c > dists/stable/main/binary-amd64/Packages.gz

# Generate Release file
cd dists/stable
apt-ftparchive release . > Release

# Sign (optional)
gpg --clearsign -o InRelease Release
```

### User Installation

```bash
# Add repository
echo "deb [trusted=yes] https://netpack.pt/debian stable main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update and install
sudo apt update
sudo apt install xfb
```

## Option 5: Managed Services

### Cloudsmith

Free for open source projects.

1. Sign up at https://cloudsmith.io
2. Create repository
3. Upload .deb file via web interface or CLI
4. Get installation instructions

### Packagecloud

Similar to Cloudsmith.

1. Sign up at https://packagecloud.io
2. Create repository
3. Upload package
4. Users add repository

## Recommended Approach for XFB

I recommend **Option 2 (GitHub Pages)** because:

1. ✅ Free hosting
2. ✅ Easy to maintain
3. ✅ Works with `apt install`
4. ✅ Automatic updates possible
5. ✅ No additional infrastructure
6. ✅ Version control integrated

## Automation Script

I'll create a script to automate the GitHub Pages repository setup.

### Usage

```bash
# Build .deb package
./build-deb-no-tests.sh

# Update repository
./update-debian-repo.sh

# Push to GitHub Pages
git push origin gh-pages
```

## Comparison Table

| Method | Ease | Cost | apt install | Auto-updates | Trust |
|--------|------|------|-------------|--------------|-------|
| GitHub Releases | ⭐⭐⭐⭐⭐ | Free | ❌ | ❌ | ⭐⭐⭐ |
| GitHub Pages | ⭐⭐⭐⭐ | Free | ✅ | ✅ | ⭐⭐⭐ |
| Ubuntu PPA | ⭐⭐⭐ | Free | ✅ | ✅ | ⭐⭐⭐⭐⭐ |
| Self-hosted | ⭐⭐ | Paid | ✅ | ✅ | ⭐⭐⭐⭐ |
| Cloudsmith | ⭐⭐⭐⭐⭐ | Free/Paid | ✅ | ✅ | ⭐⭐⭐⭐ |

## Next Steps

1. Choose your preferred method
2. Follow the setup guide
3. Test installation on clean system
4. Update documentation with installation instructions
5. Announce to users

## Security Considerations

### GPG Signing

Always sign your repository for security:

```bash
# Generate key
gpg --full-generate-key

# Export public key
gpg --armor --export YOUR_EMAIL > xfb-archive-keyring.gpg

# Sign Release
gpg --clearsign -o InRelease Release
```

### HTTPS

Always use HTTPS for repository hosting:
- GitHub Pages: Automatic HTTPS
- Self-hosted: Use Let's Encrypt

### Package Verification

Users should verify packages:

```bash
# Check package signature
dpkg-sig --verify xfb_*.deb

# Check package contents
dpkg-deb --contents xfb_*.deb
```

## Troubleshooting

### Repository Not Found

```bash
# Check URL
curl -I https://your-repo-url/dists/stable/Release

# Verify repository structure
ls -R debian-repo/
```

### GPG Key Issues

```bash
# Import key manually
wget -qO - https://your-repo-url/key.gpg | sudo apt-key add -

# Or use trusted=yes (less secure)
deb [trusted=yes] https://your-repo-url stable main
```

### Package Not Found

```bash
# Update package list
sudo apt update

# Search for package
apt-cache search xfb

# Check available versions
apt-cache policy xfb
```

## Resources

- **Debian Repository Format**: https://wiki.debian.org/DebianRepository/Format
- **GitHub Pages**: https://pages.github.com/
- **Launchpad PPA**: https://help.launchpad.net/Packaging/PPA
- **apt-ftparchive**: https://manpages.debian.org/apt-ftparchive

## Support

For questions about repository setup:
- **GitHub Issues**: https://github.com/netpack/XFB/issues
- **Email**: info@netpack.pt
