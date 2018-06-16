#!/usr/bin/env bash

echo "Build BiometricLib [FaceLib, IrisLib, and IrisAnalysisLib] and install to /use/local/alphablocks"

OCV=/usr/local/opencv/2.4.13.6
BUILD_DIR="build"

if [ -d ${BUILD_DIR} ]; then
	echo "==================="
	echo "remove build dir..."
	echo "==================="
	rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR}
cd build
cmake -D OpenCV_DIR=${OCV} -DCMAKE_INSTALL_PREFIX=/usr/local/alphablocks ..
make -j8 && make install
