#!/bin/bash

IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
echo "Host IP: $IP"

/usr/X11/bin/xhost +$IP

BIO_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/BiometricLib
BIO_DST=/home/developer/projects/Biometriclib
FACE_RECOG_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/face_recognition
FACE_RECOG_DST=/home/developer/projects/face_recognition
IRIS_UX_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/IrisAnalysis
IRIS_UX_DST=/home/developer/projects/IrisAnalysis

CONTAINER_IP=192.168.0.50

exec docker run -it \
    -v  $BIO_SRC:$BIO_DST \
    -v  $FACE_RECOG_SRC:$FACE_RECOG_DST \
    -v  $IRIS_UX_SRC:$IRIS_UX_DST \
    -e DISPLAY=$IP:0 \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v ${HOME}/.Xauthority:/home/user/.Xauthority \
    --name iris-ocv3-qt5_dev \
    --ip $CONTAINER_IP \
    iris-ocv3-qt5:latest \
    "$@"
