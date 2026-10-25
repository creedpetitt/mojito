#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

REPO="creedpetitt/moji"
VERSION="v1.0.0"

echo -e "moji $VERSION - C++20 Web Server Framework"

is_installed_apt() {
    dpkg -s "$1" &> /dev/null
}

install_dependencies() {
    echo -e "${YELLOW}[+] Checking system dependencies...${NC}"

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/debian_version ]; then
            CORE_LIBS="cmake g++ build-essential ccache pkg-config"
            MISSING_CORE=""
            for lib in $CORE_LIBS; do
                if ! is_installed_apt "$lib"; then MISSING_CORE="$MISSING_CORE $lib"; fi
            done

            if [ -n "$MISSING_CORE" ]; then
                echo -e "${YELLOW}[+] Installing dependencies:$MISSING_CORE${NC}"
                sudo apt-get update && sudo apt-get install -y $MISSING_CORE
            fi
        elif [ -f /etc/redhat-release ]; then
            echo -e "${YELLOW}[+] Detected RHEL/Fedora-based system${NC}"
            if grep -qE "Rocky|Alma|CentOS" /etc/redhat-release; then
                sudo dnf install -y epel-release
                sudo dnf config-manager --set-enabled crb || true
            fi
            sudo dnf install -y gcc-c++ make cmake ccache pkg-config
        elif [ -f /etc/arch-release ]; then
            echo -e "${YELLOW}[+] Detected Arch Linux${NC}"
            sudo pacman -S --noconfirm --needed base-devel cmake ccache pkgconf
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        if ! command -v brew &> /dev/null; then
            echo -e "${RED}[!] Homebrew not found. Please install Homebrew first.${NC}"
            exit 1
        fi
        echo -e "${YELLOW}[+] Detected macOS. Installing via Homebrew...${NC}"
        brew install cmake ccache pkg-config
    else
        echo -e "${RED}[!] Unsupported OS. Please install dependencies manually.${NC}"
    fi
}

prepare_source() {
    if [ -f "CMakeLists.txt" ] && [ -d "include/moji" ] && [ -d "src" ]; then
        return
    fi

    echo -e "${YELLOW}[+] Remote install detected. Cloning moji repository...${NC}"
    TEMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/${REPO}.git "$TEMP_DIR"
    cd "$TEMP_DIR"
}

install_dependencies
prepare_source

echo -e "${YELLOW}[+] Configuring moji...${NC}"
cmake -S . -B build/install -Dmoji_BUILD_TESTS=OFF -Dmoji_INSTALL=ON

echo -e "${YELLOW}[+] Installing moji CMake package...${NC}"
if [ -w /usr/local ]; then
    cmake --install build/install
else
    sudo cmake --install build/install
fi

echo -e "${GREEN}[+] moji $VERSION installed successfully.${NC}"
echo -e "${YELLOW}[!] Link your app with moji::core from CMake.${NC}"
