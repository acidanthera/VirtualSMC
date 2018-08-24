#!/bin/bash

WORKDIR=$(dirname "$0")
pushd "$WORKDIR" &>/dev/null
WORKDIR="$(pwd)"
popd &>/dev/null

error() {
	echo "$1"
	exit 1
}

smcver() {
	if [ -f "$1" ]; then
		REV=$(head -1 "$1" | grep "# Version: " | cut -d" " -f3)
		echo "$REV"
	fi
}

smcverbranch() {
	branch=$(echo "$1" | sed 's/[0-9.]//g')
	if [ "$branch" != "" ] && [ "$branch" != "f" ]; then
		echo "_$branch"
	fi
}

smcvercmp() {
	new=$(echo "$1" | sed 's/[a-z]/./')
	if [ -f "$2" ]; then
		old=$(cat "$2" | grep "\[REV \] is " | cut -d" " -f6 | sed 's/[a-z]/./')
	else
		old=""
	fi

	if [ "$new" != "" ] ; then
		if [ "$old" == "" ] || [[ $new > $old ]]; then
			echo "$new"
		fi
	fi
}

if [ ! -x "$SMCREAD" ]; then
	if [ -x "$WORKDIR/smcread" ]; then
		SMCREAD="$WORKDIR/smcread"
	elif [ "$(which smcread)" != "" ]; then
		SMCREAD=smcread
	else
		error "Unable to locate smcread"
	fi
fi

if (($# < 1)); then
	error "Usage: ./smckdb.sh FirmwareUpdate.pkg"
fi

SRC="$(cd "$(dirname "$1")"; pwd)/$(basename "$1")"

rm -rf "$WORKDIR/tmp/"
mkdir -p "$WORKDIR/db/" || error "Unable to save db"

if [ -d "$SRC/SMCPayloads" ]; then
	PAYLOADS="$SRC/SMCPayloads"
else
	PAYLOADS="$WORKDIR/tmp/Scripts/Tools/SMCPayloads"
	pkgutil --expand "$SRC" "$WORKDIR/tmp" && test -d "$PAYLOADS" || error "Unable to unpack $1"
fi

pushd "$PAYLOADS" &>/dev/null
	for i in * ; do
		if [ -d "$i" ]; then
			echo "Processing $i"
			
			mkdir -p "$WORKDIR/db/$i"
			if [ ! -d "$WORKDIR/db/$i" ]; then
				echo "Unable to save $i info"
				continue
			fi

			RVBF_V=$(smcver "$i/flasher_base.smc")
			RVUF_V=$(smcver "$i/flasher_update.smc")
			REV_V=$(smcver "$i/$i.smc")

			RVBF_B=$(smcverbranch "$RVBF_V")
			RVUF_B=$(smcverbranch "$RVUF_V")
			REV_B=$(smcverbranch "$REV_V")

			RVBF=$(smcvercmp "$RVBF_V" "$WORKDIR/db/$i/flasher_base$RVBF_B.txt")
			RVUF=$(smcvercmp "$RVUF_V" "$WORKDIR/db/$i/flasher_update$RVUF_B.txt")
			REV=$(smcvercmp "$REV_V" "$WORKDIR/db/$i/main$REV_B.txt")

			if [ "$RVBF" != "" ]; then
				echo "Updating flasher_base"
				"$SMCREAD" "$i/flasher_base.smc" "$WORKDIR/db/$i/flasher_base$RVBF_B.bin" > "$WORKDIR/db/$i/flasher_base$RVBF_B.txt"
			fi

			if [ "$RVUF" != "" ]; then
				echo "Updating flasher_update"
				"$SMCREAD" "$i/flasher_update.smc" "$WORKDIR/db/$i/flasher_update$RVUF_B.bin" > "$WORKDIR/db/$i/flasher_update$RVUF_B.txt"
			fi
			
			if [ "$REV" != "" ]; then
				echo "Updating main"
				"$SMCREAD" "$i/$i.smc" "$WORKDIR/db/$i/main$REV_B.bin" > "$WORKDIR/db/$i/main$REV_B.txt"
			fi
		fi
	done
popd &>/dev/null

rm -rf "$WORKDIR/tmp"
exit 0
