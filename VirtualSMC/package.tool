#!/bin/bash

if [ "${TARGET_BUILD_DIR}" = "" ]; then
  echo "This must not be run outside of Xcode"
  exit 1
fi

cd "${TARGET_BUILD_DIR}"

rm -rf package *.zip || exit 1

if [ "$1" != "" ]; then
  echo "Got action $1, skipping!"
  exit 0
fi

mkdir -p package/Tools || exit 1
mkdir -p package/Kexts || exit 1
mkdir -p package/Drivers || exit 1

cd package || exit 1

cp ../smcread Tools/ || exit 1
cp ../rtcread Tools/ || exit 1
cp ../smc Tools/ || exit 1
cp ../libaistat.dylib Tools/ || exit 1

for kext in ../*.kext; do
  echo "$kext"
  cp -a "$kext" Kexts/ || exit 1
done

# Workaround Xcode 10 bug
if [ "$CONFIGURATION" = "" ]; then
  if [ "$(basename "$TARGET_BUILD_DIR")" = "Debug" ]; then
    CONFIGURATION="Debug"
  else
    CONFIGURATION="Release"
  fi
fi

if [ "$CONFIGURATION" = "Release" ]; then
  mkdir -p dSYM || exit 1
  for dsym in ../*.dSYM; do
    if [ "$dsym" = "../*.dSYM" ]; then
      continue
    fi
    echo "$dsym"
    cp -a "$dsym" dSYM/ || exit 1
  done
fi

cp "${SRCROOT}/VirtualSmcPkg/Binaries/$(echo $CONFIGURATION | tr /a-z/ /A-Z/)/VirtualSmc.efi" Drivers/ || exit 1

archive="${MODULE_VERSION} ($(echo $CONFIGURATION | tr /a-z/ /A-Z/)).zip"
zip -qry ../"${archive}" * || exit 1
