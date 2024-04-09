# XFB

XFB is an open-source radio automation software developed by [Frédéric Bogaerts](https://www.researchgate.net/profile/Frederic-Bogaerts) at [Netpack Online Solutions](https://www.netpack.pt).

![XFB](https://netpack.pt/img/xfb-home.webp)

## Overview

XFB is designed to automate radio station operations, providing comprehensive management of various assets such as music, jingles, advertisements, programs, genres, and more. With its intuitive interface and powerful features, XFB simplifies the management and broadcasting process for radio stations.

## Features

- **Database Management**: XFB includes a robust database system for cataloging and organizing music, jingles, advertisements, and other assets.
- **Automated Scheduling**: Easily create and manage schedules for music playback, advertisement slots, and program airing.
- **Customizable Workflow**: Tailor XFB to fit your station's unique needs with customizable settings and configurations.
- **Remote Access (beta)**: Access and control XFB remotely, allowing for convenient management from anywhere with an internet connection.


## Getting Started

To get started with XFB, follow these steps:

### Installing from AUR (Arch Linux or Arch-based (i.e.: Manjaro))


#### Option 1: Using GUI (e.g., pacman)

1. Open your package manager GUI (e.g., `pacman`).

2. Navigate to the AUR section and ensure you have AUR repositories enabled

3. Search for `xfb` and click Install.


#### Option 2: Using the terminal

1. Make sure you have access to the AUR repositories.

    To add the AUR repositories to your `pacman` configuration file do:

    ```sh
    echo '[aur]' | sudo tee -a /etc/pacman.conf
    echo 'SigLevel = Never' | sudo tee -a /etc/pacman.conf
    echo 'Server = https://repo.archlinux.org/$arch' | sudo tee -a /etc/pacman.conf
    ```

2. Update your package lists:

    ```sh
    sudo pacman -Sy
    ```

3. Install XFB using `yay`:

    ```sh
    yay -S xfb
    ```

### Manual Installation

1. Clone or download the latest release of XFB from the [GitHub repository](https://github.com/netpack/XFB).

2. Compile and install:

    ```sh
    cd XFB && mkpkg -si --force
    ```

### Optional Dependencies

1. Audacity
2. Youtube-dl

## Support and Contributions

For support or inquiries regarding XFB, feel free to reach out to [Frédéric Bogaerts](https://www.researchgate.net/profile/Frederic-Bogaerts) or the [Netpack Online Solutions](https://www.netpack.pt) team. Contributions to the project are welcome and can be submitted through GitHub pull requests.

## License

XFB - GNU General Public License version 3 (GPL-3.0)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License version 3 (GPL-3.0) as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.




Have fun,
Frédéric Bogaerts
