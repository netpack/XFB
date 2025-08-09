<div align="center">
  <img src="https://netpack.pt/img/xfb-home.webp" alt="XFB Screenshot" width="800"/>
  <h1>XFB - Radio Automation Software</h1>
  <p><strong>The Open-Source Solution for Professional Radio Broadcasting, now rebuilt with Qt6.</strong></p>

  <p>
    <a href="https://github.com/netpack/XFB/releases"><img src="https://img.shields.io/github/v/release/netpack/XFB?display_name=tag&sort=semver" alt="Latest Release"></a>
    <a href="https://aur.archlinux.org/packages/xfb"><img src="https://img.shields.io/aur/version/xfb" alt="AUR version"></a>
    <a href="https://github.com/netpack/XFB/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPL v3"></a>
  </p>
</div>

XFB is an open-source radio automation software developed by [Frédéric Bogaerts](https://www.researchgate.net/profile/Frederic-Bogaerts) at [Netpack Online Solutions](https://www.netpack.pt).

## ✨ Features

XFB is designed to automate radio station operations, providing comprehensive management of various assets. With its intuitive interface and powerful features, XFB simplifies the entire broadcasting process.

-   📊 **Database Management**: A robust system for cataloging and organizing music, jingles, advertisements, programs, and more.
-   🔄 **Automated Scheduling**: Easily create and manage schedules for music playback, advertisement slots, and program airing.
-   ⚙️ **Customizable Workflow**: Tailor XFB to fit your station's unique needs with customizable settings and configurations.
-   🌐 **Remote Access (beta)**: Access and control XFB remotely, allowing for convenient management from anywhere with an internet connection.

---

## 🚀 Getting Started: Installation

The easiest way to install XFB is by using a pre-built package for your operating system.

| Operating System            | Download / Source                                                                   | Installation Command                                    |
| --------------------------- | ----------------------------------------------------------------------------------- | ------------------------------------------------------- |
| 🍏 **macOS** (Intel/Apple Silicon) | [**GitHub Releases**](https://github.com/netpack/XFB/releases) (Download the `.dmg` file) | Drag `XFB.app` to your Applications folder.               |
| 🐧 **Debian / Ubuntu** (amd64)  | [**GitHub Releases**](https://github.com/netpack/XFB/releases) (Download the `.deb` file) | `sudo apt install ./xfb_*.deb`                            |
|  Arch Linux (Manjaro, etc.) | [**AUR (Arch User Repository)**](https://aur.archlinux.org/packages/xfb)            | `yay -S xfb`                                            |

### Installing on Arch Linux (Details)

If you're new to the AUR on Arch Linux, here are the detailed steps.

#### Option 1: Using a GUI Helper (e.g., `pamac`)

1.  Open your package manager GUI (e.g., `pamac-manager`).
2.  Navigate to the settings, find the "Third Party" or "AUR" tab, and ensure it's enabled.
3.  Search for `xfb` and click "Install".

#### Option 2: Using a CLI Helper (e.g., `yay`)

1.  If you don't have an AUR helper like `yay`, install it first:
    ```sh
    sudo pacman -S --needed git base-devel
    git clone https://aur.archlinux.org/yay.git
    cd yay
    makepkg -si
    ```
2.  Install XFB using `yay`:
    ```sh
    yay -S xfb
    ```

---

<details>
<summary>🛠️ Building from Source (for Developers)</summary>
<br>

If you prefer to compile the application yourself, follow these instructions.

#### Prerequisites
- A C++ compiler (g++, clang)
- Git
- Qt6 Development Libraries

#### On Debian / Ubuntu
1. **Install dependencies:**
   ```sh
   sudo apt update
   sudo apt install build-essential qt6-base-dev qt6-multimedia-dev git
   ```
2. **Clone and build:**
   ```sh
   git clone https://github.com/netpack/XFB.git
   cd XFB/src
   mkdir build && cd build
   qmake6 ../XFB.pro
   make -j$(nproc)
   ```
3. **Run:**
   ```sh
   ./XFB
   ```

#### On Arch Linux
1. **Install dependencies:**
   ```sh
   sudo pacman -S --needed git base-devel qt6-base qt6-multimedia
   ```
2. **Clone and build using `makepkg`:**
   ```sh
   git clone https://aur.archlinux.org/xfb.git # Or your own repo clone
   cd xfb
   makepkg -si
   ```

</details>

### Optional Dependencies

For full functionality, you may want to install these helpful tools:
-   **Audacity**: For advanced audio editing.
-   **yt-dlp**: For downloading media from various online sources.

---

## ❤️ Support and Contributions

For support or inquiries, please [open an issue](https://github.com/netpack/XFB/issues) on GitHub.

Contributions to the project are welcome! Please feel free to fork the repository and submit a pull request.

## 📜 License

XFB is licensed under the **GNU General Public License v3.0**.

This program is free software and is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY. See the `LICENSE` file for the full text.

<br>

🎶 Have fun,

👨‍💻 Frédéric Bogaerts
