FROM ubuntu:20.04

# Install dependencies for Qt installer and build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    libgl1-mesa-dev \
    libxcb-xinerama0 \
    libxkbcommon-x11-0 \
    libpulse-dev \
    libfontconfig1 \
    libfreetype6 \
    libx11-xcb1 \
    libxcb1 \
    libxrender1 \
    libxcb-glx0 \
    libxcb-keysyms1 \
    libxcb-image0 \
    libxcb-shm0 \
    libxcb-icccm4 \
    libxcb-sync1 \
    libxcb-xfixes0 \
    libxcb-shape0 \
    libxcb-randr0 \
    libxcb-render-util0 \
    libxcb-xinerama0 \
    libxcb-xkb1 \
    libxkbcommon0 \
    libxcb-util0 \
    libxcb-xinput0 \
    curl \
    ca-certificates \
    --fix-missing \
    && rm -rf /var/lib/apt/lists/*

# Download Qt online installer
RUN wget https://download.qt.io/official_releases/online_installers/qt-unified-linux-x64-online.run -O /tmp/qt-installer.run && \
    chmod +x /tmp/qt-installer.run

# Install Qt 6.5.1 silently
RUN /tmp/qt-installer.run --script /tmp/qt-installer-noninteractive.qs || true

# Copy the non-interactive installer script
COPY src/qt-installer-noninteractive.qs /tmp/qt-installer-noninteractive.qs

# Set environment variables for Qt6
ENV PATH="/opt/Qt/6.5.1/gcc_64/bin:$PATH"
ENV QTDIR="/opt/Qt/6.5.1/gcc_64"

# Set working directory
WORKDIR /xfb

# Copy source code into container
COPY . /xfb

# Build the project
RUN qmake src/XFB.pro && make -j$(nproc)

# Default command to keep container running for inspection
CMD ["/bin/bash"]
