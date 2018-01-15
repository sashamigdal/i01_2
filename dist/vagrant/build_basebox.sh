#!/bin/bash

set -x
set -e
#set -u

if [[ -z "$1" ]]; then
    echo "Usage: $0 <output_path>"
    echo "Create a Vagrant basebox using packer and packer-arch."
    echo "Requires git, sha256sum, virtualbox, and wget."
    exit 1
fi

OUTPUT_PATH="$1"
SCRIPT_PATH="${BASH_SOURCE[0]}"
SCRIPT_RELDIR="$(dirname ${SCRIPT_PATH})"
SCRIPT_DIR="$(cd ${SCRIPT_RELDIR} && pwd)"
TEMP_DIR="$(mktemp -d basebox.XXXXX)"

OSKERNEL="`uname -s | tr '[:upper:]' '[:lower:]'`"
PACKERZIP="packer_0.7.2_${OSKERNEL}_amd64.zip"
case $OSKERNEL in
    linux)
        PACKERSHA256="2e0a7971d0df81996ae1db0fe04291fb39a706cc9e8a2a98e9fe735c7289379f"
        SHA256SUM="sha256sum"
        ;;
    darwin)
        PACKERSHA256="bed7ddc255b5b71b139de5e30d4221926cd314a87baf0b937ba7a97c196b1286"
        SHA256SUM="shasum -a 256"
        ;;
    *)
        echo "Unsupported kernel: $OSKERNEL"
        exit 1
        ;;
esac

pushd "$TEMP_DIR"
    echo "Downloading packer and packer-arch to ${TEMP_DIR}..."
    wget -O "${PACKERZIP}" "https://dl.bintray.com/mitchellh/packer/${PACKERZIP}"
    if [[ "x$(${SHA256SUM} ${PACKERZIP} | cut -d' ' -f1)" != "x${PACKERSHA256}" ]]; then
        echo "Checksum failed on Packer ZIP file..." >&2
        exit 1
    fi
    unzip "${PACKERZIP}"

    echo "Cloning packer-arch git repository..."
    git clone "https://github.com/elasticdog/packer-arch.git" || \
        exit 2
    pushd packer-arch
        echo "Patching base box install script..."
        patch -p1 < "${SCRIPT_DIR}/packer-arch-install-virtualbox-sh.patch"
    popd
        echo "Building base box..."
        ./packer build -only=virtualbox-iso packer-arch/arch-template.json || \
            exit 3
popd
mv "${TEMP_DIR}/packer_arch_virtualbox.box" "${OUTPUT_PATH}" || exit 4
echo "Removing ${TEMP_DIR} in 5 seconds..." && \
    sleep 5 && \
    rm -r "${TEMP_DIR}"
