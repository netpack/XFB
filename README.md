# XFB Debian Repository

APT repository for XFB Radio Automation Software, version 3.1417.

## Install

The repository is not GPG-signed, so apt needs to be told to trust it:

```bash
echo "deb [trusted=yes] https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list
sudo apt update
sudo apt install xfb
```

## Update

```bash
sudo apt update && sudo apt upgrade xfb
```

## Remove

```bash
sudo apt remove xfb
sudo rm /etc/apt/sources.list.d/xfb.list
```

## Contents

- Architectures: amd64 arm64
- Suite: stable, component: main

Packages can also be downloaded directly from the
[GitHub releases page](https://github.com/netpack/XFB/releases).

- Website: https://netpack.pt
- Issues: https://github.com/netpack/XFB/issues
