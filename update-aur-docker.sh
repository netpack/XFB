#!/bin/bash
# Update AUR using Docker (works on macOS)
# This script creates an Arch Linux container and updates AUR from there

set -e

echo "╔══════════════════════════════════════════════════════════╗"
echo "║          XFB AUR Update via Docker                       ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "❌ Error: Docker not found"
    echo ""
    echo "Install Docker:"
    echo "  macOS: brew install --cask docker"
    echo "  Or download from: https://www.docker.com/products/docker-desktop"
    exit 1
fi

echo "✓ Docker found"
echo ""

# Check if SSH key exists
if [ ! -f "$HOME/.ssh/id_ed25519" ] && [ ! -f "$HOME/.ssh/id_rsa" ]; then
    echo "⚠️  Warning: No SSH key found"
    echo ""
    echo "Generate one with:"
    echo "  ssh-keygen -t ed25519 -C 'your.email@example.com'"
    echo ""
    echo "Then add it to AUR:"
    echo "  https://aur.archlinux.org/account/"
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Create Dockerfile for AUR update
cat > Dockerfile.aur-update << 'EOF'
FROM archlinux:latest

# Update system and install required packages
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
        base-devel \
        git \
        openssh \
        vim

# Create user for AUR operations
RUN useradd -m -G wheel aur && \
    echo "aur ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

USER aur
WORKDIR /home/aur

CMD ["/bin/bash"]
EOF

echo "Building Docker image..."
docker build -t xfb-aur-update -f Dockerfile.aur-update .

echo ""
echo "Starting Arch Linux container..."
echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  You are now in an Arch Linux container                  ║"
echo "║                                                          ║"
echo "║  Steps to update AUR:                                    ║"
echo "║                                                          ║"
echo "║  1. Configure git:                                       ║"
echo "║     git config --global user.name 'Your Name'            ║"
echo "║     git config --global user.email 'your@email.com'      ║"
echo "║                                                          ║"
echo "║  2. Set up SSH key:                                      ║"
echo "║     mkdir -p ~/.ssh                                      ║"
echo "║     # Copy your SSH key to ~/.ssh/                       ║"
echo "║     chmod 600 ~/.ssh/id_*                                ║"
echo "║                                                          ║"
echo "║  3. Clone AUR repo:                                      ║"
echo "║     git clone ssh://aur@aur.archlinux.org/xfb.git        ║"
echo "║     cd xfb                                               ║"
echo "║                                                          ║"
echo "║  4. Copy PKGBUILD:                                       ║"
echo "║     # Copy from /xfb/PKGBUILD                            ║"
echo "║                                                          ║"
echo "║  5. Generate .SRCINFO:                                   ║"
echo "║     makepkg --printsrcinfo > .SRCINFO                    ║"
echo "║                                                          ║"
echo "║  6. Commit and push:                                     ║"
echo "║     git add PKGBUILD .SRCINFO                            ║"
echo "║     git commit -m 'Update to version 2.0.0'              ║"
echo "║     git push                                             ║"
echo "║                                                          ║"
echo "║  Type 'exit' when done                                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Run container with current directory mounted
docker run -it --rm \
    -v "$(pwd):/xfb:ro" \
    -v "$HOME/.ssh:/home/aur/.ssh-host:ro" \
    xfb-aur-update

echo ""
echo "Container exited"
echo ""
echo "If you successfully updated AUR, verify at:"
echo "  https://aur.archlinux.org/packages/xfb"
