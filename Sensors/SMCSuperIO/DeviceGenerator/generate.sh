#!/bin/bash

# Remove the original resources
rm -f "${PROJECT_DIR}/Sensors/SMCSuperIO/Devices.cpp"

# Reformat project plists (perl part is for Xcode styling)
#find "${PROJECT_DIR}/Resources/" -name "*.plist" \
#  -exec plutil -convert xml1 {} \; \
#  -exec perl -0777 -pi -e "s#<data>\s+([^\s]+)\s+</data>#<data>\1</data>#g" {} \; || exit 1

ret=0
"${TARGET_BUILD_DIR}/DeviceGenerator" \
	"${PROJECT_DIR}/Sensors/SMCSuperIO/Resources" \
	"${PROJECT_DIR}/Sensors/SMCSuperIO/Devices.cpp" || ret=1

if (( $ret )); then
	echo "Failed to build Devices.cpp"
	exit 1
fi
