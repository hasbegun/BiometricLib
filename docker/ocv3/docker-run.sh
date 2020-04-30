#!/bin/bash

SRC_BASE=/Users/inho.choi/devel/alphablocks
DST_BASE=/home/developer/projects
BIO_SRC=$SRC_BASE/projects/iris/BiometricLib
BIO_DST=$DST_BASE/Biometriclib
FACE_RECOG_SRC=$SRC_BASE/projects/iris/face_recognition
FACE_RECOG_DST=$DST_BASE/face_recognition
IRIS_UX_SRC=$SRC_BASE/projects/iris/IrisAnalysis
IRIS_UX_DST=$DST_BASE/IrisAnalysis
DATA_SET_SRC=$SRC_BASE/datasets
DATA_SET_DST=$DST_BASE/datasets

CONTAINER_IP=192.168.0.50

IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
echo "Host IP: $IP"

# the following arg is optional. this is for vnc access.
#  -v ${HOME}/.Xauthority:/home/developer/.Xauthority \
#################################

/usr/X11/bin/xhost +$IP
exec docker run -it \
    -v  $BIO_SRC:$BIO_DST:rw \
    -v  $FACE_RECOG_SRC:$FACE_RECOG_DST:rw \
    -v  $IRIS_UX_SRC:$IRIS_UX_DST:rw \
    -v  $DATA_SET_SRC:$DATA_SET_DST:rw \
    -e DISPLAY=$IP:0 \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v ${HOME}/.Xauthority:/home/developer/.Xauthority \
    --name iris-ocv3-qt5_dev \
    --ip $CONTAINER_IP \
    iris-ocv3-qt5:latest \
    "$@"
