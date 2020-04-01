SRC=/Users/inho.choi/devel/alphablocks/projects/iris/BiometricLib
DST=/home/developer/projects/Biometriclib
FACE_RECOG_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/face_recognition
FACE_RECOG_DST=/home/developer/projects/face_recognition
IRIS_UX_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/IrisAnalysis
IRIS_UX_DST=/home/developer/projects/IrisAnalysis
docker run -it --name iris_ocv3_dev \
    -v  $SRC:$DST \
    -v  $FACE_RECOG_SRC:$FACE_RECOG_DST \
    -v  $IRIS_UX_SRC:$IRIS_UX_DST \
    iris-ocv3:latest