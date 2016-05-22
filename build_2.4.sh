#!/usr/bin/env bash

rm -rf build CMakeLists.txt
mkdir build
cp CMakeLists.txt_cv2 CMakeLists.txt
cd build
cmake -DCMAKE_PREFIX_PATH=/usr/local/opencv/2.4.9/osx ..
make -j8
