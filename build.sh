#!/usr/bin/env bash

OCV=/usr/local/opencv/2.4.13.3
#OCV=/usr/local/opt/opencv
BUILD_DIR="build"

if [ -d ${BUILD_DIR} ]; then
	echo "==================="
	echo "remove build dir..."
	echo "==================="
	rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR}
cd build
cmake -D OpenCV_DIR=${OCV} -DCMAKE_INSTALL_PREFIX=/usr/local/eyedentiscan ..
make -j8 && make install
