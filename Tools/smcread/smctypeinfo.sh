#!/bin/bash

genbranch() {
	rm -f ${1}v${2}.*.tmp types/${1}v${2}.*.txt
	mkdir -p types
	touch ${1}v${2}.pubnum.tmp ${1}v${2}.pub.tmp ${1}v${2}.privnum.tmp ${1}v${2}.priv.tmp

	for i in $(find db -name "${1}.txt"); do
		vers=$(cat $i | grep '\[REV \] is ' | cut -d' ' -f6 | sed 's/[a-z]/./g')

		if [ "$vers" == "" ]; then
			echo "Invalid file at $i"
			continue
		elif [ "$(echo $vers | grep -e ^$2)" == "" ]; then
			continue
		fi

		k=pubnum
		while read p; do
			if [ "$p" != "" ]; then
				if [ "$k" == "pub" ] || [ "$k" == "priv" ]; then
					# Look the key up
					key=$(echo $p | cut -d' ' -f1 | sed -e s/[\]\[]//g)
					mvers=$(cat ${1}v${2}.$k.tmp | grep -e ^\\\[$key)
					if [ "$mvers" != "" ]; then
						if [[ $mvers < $vers ]]; then
							# Remove older
							cat ${1}v${2}.$k.tmp | grep -ve ^\\\[$key > "${1}v${2}.$k.ntmp"
							mv "${1}v${2}.$k.ntmp" "${1}v${2}.$k.tmp"
							echo "$p $vers" >> "${1}v${2}.$k.tmp"
						fi
					else
						# Add if missing
						echo "$p $vers" >> "${1}v${2}.$k.tmp"
					fi
				else
					echo "$p" >> "${1}v${2}.$k.tmp"
				fi
			fi

			if [ "$k" == "pubnum" ] && [[ "$p" =~ ^"Public keys" ]]; then
				k=pub
			elif [ "$k" == "pub" ] && [ "$p" == "" ]; then
				k=privnum
			elif [ "$k" == "privnum" ] && [[ "$p" =~ ^"Hidden keys" ]]; then
				k=priv
			elif [ "$k" == "priv" ] && [ "$p" == "" ]; then
				break
			fi
		done < $i
	done

	if [ "$(cat ${1}v${2}.pub.tmp | sed 's/handler \[.*\] //' | wc -l)" != "$(cat ${1}v${2}.pub.tmp | wc -l)" ]; then 
		echo "WTF"
	fi

	sort -r ${1}v${2}.pubnum.tmp | uniq > types/${1}v${2}.pubnum.txt
	cat ${1}v${2}.pub.tmp | sed 's/handler \[.*\] //' | sed -e 's# [0-9]\.[0-9]*\.[0-9]*##' | sort | uniq > types/${1}v${2}.pub.txt
	sort -r ${1}v${2}.privnum.tmp | uniq > types/${1}v${2}.privnum.txt
	cat ${1}v${2}.priv.tmp | sed 's/handler \[.*\] //' | sed -e 's# [0-9]\.[0-9]*\.[0-9]*##' | sort | uniq > types/${1}v${2}.priv.txt

	rm -f ${1}v${2}.*.tmp
}

genbranch main 1
genbranch main 2