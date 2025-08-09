#!/bin/bash
# This script runs the hidden install_dependencies_mac.sh script inline to avoid opening multiple terminal windows

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
bash .install_dependencies_mac.sh
