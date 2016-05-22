make clean
rm -rf CMakeCache.txt CMakeFiles Makefile cmake_install.cmake ../CMakeLists.txt
cp ../CMakeLists.txt_cv3 ../CMakeLists.txt
cmake -DCMAKE_PREFIX_PATH=/usr/local/opencv/3.1.0/osx ..

make -j8
