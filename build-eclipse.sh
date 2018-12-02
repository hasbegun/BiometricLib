export CC=/usr/bin/gcc
export CXX=/usr/bin/g++

OpenCV_DIR=/usr/local/opencv/2.4.13.6
BUILD_DIR=build-eclipse

if [ -d $BUILD_DIR ]; then
    rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR}

cd ${BUILD_DIR}
cp ${OpenCV_DIR}/share/OpenCV/lbpcascades/lbpcascade_frontalface.xml .
cp ${OpenCV_DIR}/share/OpenCV/haarcascades/haarcascade_eye*.xml .
cp ${OpenCV_DIR}/share/OpenCV/haarcascades/haarcascade_front*.xml .

cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DCMAKE_ECLIPSE_MAKE_ARGUMENTS=-j8 -D OpenCV_DIR=$OpenCV_DIR -DCMAKE_INSTALL_PREFIX=/usr/local/alphablocks ..

