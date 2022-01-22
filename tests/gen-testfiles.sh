#!/bin/bash

# Output a string (ARG2, defaults to 'A'), repeated ARG1 times.
rep() {
	C=${1:-0}
	S=${2:-A}
	if [ "$C" -ne "0" ]; then
		printf "%.0s$S" $(seq 1 $C)
	fi
}

#
# Only generate files if we are not being sourced.
#
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	echo "Regenerating test files..."
	rep 126 A >R126A
	rep 127 A >R127A
	rep 128 A >R128A
	rep 129 A >R129A

	rep 64 Aa | cut -c -126 | tr -d '\n' > C126
	rep 64 Aa | cut -c -127 | tr -d '\n' > C127
	rep 64 Aa > C128
	rep 65 Aa | cut -c -129 | tr -d '\n' > C129

	echo "Done."
fi
