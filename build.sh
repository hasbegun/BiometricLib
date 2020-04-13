#!/usr/bin/env bash

echo "Build BiometricLib [FaceLib, IrisLib, and IrisAnalysisLib] and install to /use/local/alphablocks"
BUILD_DIR="build"

if [ -d ${BUILD_DIR} ]; then
	echo "==================="
	echo "remove build dir..."
	echo "==================="
	rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR}
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/alphablocks ..
make -j4 && sudo make install
