#!/bin/bash

BUILDDIR=$(dirname "$0")
pushd "$BUILDDIR" >/dev/null
BUILDDIR=$(pwd)
popd >/dev/null

NASMVER="2.13.03"

# For nasm in Xcode
export PATH="/opt/local/bin:$PATH"

cd "$BUILDDIR"

prompt() {
  echo "$1"
  read -p "Enter [Y]es to continue: " v
  if [ "$v" != "Y" ] && [ "$v" != "y" ]; then
    exit 1
  fi
}

updaterepo() {
  if [ ! -d "$2" ]; then
    git clone "$1" -b "$3" --depth=1 "$2" || exit 1
  fi
  pushd "$2" >/dev/null
  git pull
  popd >/dev/null
}

if [ "$BUILDDIR" != "$(printf "%s\n" $BUILDDIR)" ]; then
  echo "EDK2 build system may still fail to support directories with spaces!"
  exit 1
fi

if [ "$(which clang)" = "" ] || [ "$(which git)" = "" ] || [ "$(clang -v 2>&1 | grep "no developer")" != "" ] || [ "$(git -v 2>&1 | grep "no developer")" != "" ]; then
  echo "Missing Xcode tools, please install them!"
  exit 1
fi

if [ "$(nasm -v)" = "" ] || [ "$(nasm -v | grep Apple)" != "" ]; then
  echo "Missing or incompatible nasm!"
  echo "Download the latest nasm from http://www.nasm.us/pub/nasm/releasebuilds/"
  prompt "Last tested with nasm $NASMVER. Install it automatically?"
  pushd /tmp >/dev/null
  rm -rf "nasm-${NASMVER}-macosx.zip" "nasm-${NASMVER}"
  curl -OL "http://www.nasm.us/pub/nasm/releasebuilds/${NASMVER}/macosx/nasm-${NASMVER}-macosx.zip" || exit 1
  unzip -q "nasm-${NASMVER}-macosx.zip" "nasm-${NASMVER}/nasm" "nasm-${NASMVER}/ndisasm" || exit 1
  sudo mkdir -p /usr/local/bin || exit 1
  sudo mv "nasm-${NASMVER}/nasm" /usr/local/bin/ || exit 1
  sudo mv "nasm-${NASMVER}/ndisasm" /usr/local/bin/ || exit 1
  rm -rf "nasm-${NASMVER}-macosx.zip" "nasm-${NASMVER}"
  popd >/dev/null
fi

if [ "$(which mtoc.NEW)" == "" ] || [ "$(which mtoc)" == "" ]; then
  echo "Missing mtoc or mtoc.NEW!"
  echo "To build mtoc follow: https://github.com/tianocore/tianocore.github.io/wiki/Xcode#mac-os-x-xcode"
  prompt "Install prebuilt mtoc and mtoc.NEW automatically?"
  rm -f mtoc mtoc.NEW
  unzip -q external/mtoc-mac64.zip mtoc.NEW || exit 1
  sudo mkdir -p /usr/local/bin || exit 1
  sudo cp mtoc.NEW /usr/local/bin/mtoc || exit 1
  sudo mv mtoc.NEW /usr/local/bin/ || exit 1
fi

if [ ! -d "Binaries" ]; then
  mkdir Binaries || exit 1
  cd Binaries || exit 1
  ln -s ../UDK/Build/VirtualSmcPkg/RELEASE_XCODE5/X64 RELEASE || exit 1
  ln -s ../UDK/Build/VirtualSmcPkg/DEBUG_XCODE5/X64 DEBUG || exit 1
  cd .. || exit 1
fi

if [ "$CONFIGURATION" != "" ]; then
  MODE="$(echo $CONFIGURATION | tr /a-z/ /A-Z/)"
fi

if [ "$1" != "" ]; then
  echo "Got action $1, skipping!"
  exit 0
fi

if [ ! -f UDK/UDK.ready ]; then
  rm -rf UDK
fi

updaterepo "https://github.com/tianocore/edk2" UDK UDK2018 || exit 1
cd UDK

if [ ! -d VirtualSmcPkg ]; then
  ln -s .. VirtualSmcPkg || exit 1
fi

source edksetup.sh || exit 1
make -C BaseTools || exit 1
touch UDK.ready

if [ "$MODE" = "" ] || [ "$MODE" = "DEBUG" ]; then
  build -a X64 -b DEBUG -t XCODE5 -p VirtualSmcPkg/VirtualSmcPkg.dsc || exit 1
fi

if [ "$MODE" = "" ] || [ "$MODE" = "RELEASE" ]; then
  build -a X64 -b RELEASE -t XCODE5 -p VirtualSmcPkg/VirtualSmcPkg.dsc || exit 1
fi