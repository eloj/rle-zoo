#!/bin/bash
#
# Create a release package for a specific OS.
#
PROJECT=rle-zoo
VERSION=${VERSION:-$(<VERSION)}
OS=$1
ARCH=${2:-`uname -m`}
BP="packages/${OS}"
RP="${BP}/${PROJECT}"
FILES='rle-zoo rle-genops rle-parser rle_*.h LICENSE'

if [ -z "${VERSION}" ]; then
	echo "Could not determine VERSION. Missing file or wrong directory?"
	exit 1
fi

if [ -z "${OS}" ]; then
	echo "usage: $0 <operating-system>"
	exit 1
fi

if [[ "${OS}" == "linux" || "${OS}" == "windows" ]]; then
	echo "Packaging ${PROJECT} version ${VERSION} for ${OS}-${ARCH}"
	OPTIMIZED=1 make -B rle-zoo rle-genops rle-parser
	if [ $? -ne 0 ]; then
		echo "Build failed, packaging aborted."
		exit 1
	fi
	mkdir -p ${RP}
	cp ${FILES} ${RP}
	tar -C ${BP} -czvf packages/${PROJECT}-${VERSION}.${OS}-${ARCH}.tar.gz ${PROJECT}
	if [ "${OS}" == "windows" ]; then
		pushd .
		cd ${RP} && 7z -bd a ../../${PROJECT}-${VERSION}.${OS}-${ARCH}.7z *
		popd
	fi
	# rm -r ${BP}
else
	echo "Unknown target OS: ${OS}"
	exit 2
fi

echo "Done."
