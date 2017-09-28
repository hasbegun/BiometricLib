OpenCV_DIR=/usr/local/opencv/2.4.13.3
BUILD_DIR=build-xcode

if [ -d $BUILD_DIR ]; then
    rm -rf ${BUILD_DIR}
fi
mkdir ${BUILD_DIR}

cd ${BUILD_DIR}
cp ${OpenCV_DIR}/share/OpenCV/lbpcascades/lbpcascade_frontalface.xml .
cp ${OpenCV_DIR}/share/OpenCV/haarcascades/haarcascade_eye*.xml .
cp ${OpenCV_DIR}/share/OpenCV/haarcascades/haarcascade_front*.xml .

cmake -G Xcode -D OpenCV_DIR=$OpenCV_DIR -DCMAKE_INSTALL_PREFIX=/usr/local/eyedentiscan ..
open BiometricLib.xcodeproj
