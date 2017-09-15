#!/usr/bin/env bash

export OCV=/usr/local/opencv/2.4.13.3

if [ -d "build" ]; then
	echo "==================="
	echo "remove build dir..."
	echo "==================="
	rm -rf build
fi
mkdir build
cd build
cmake -D OpenCV_DIR=${OCV} -DCMAKE_INSTALL_PREFIX=/usr/local/eyedentiscan ..
make -j8 && make install
