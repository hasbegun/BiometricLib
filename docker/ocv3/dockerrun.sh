BIO_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/BiometricLib
BIO_DST=/home/developer/projects/Biometriclib
FACE_RECOG_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/face_recognition
FACE_RECOG_DST=/home/developer/projects/face_recognition
IRIS_UX_SRC=/Users/inho.choi/devel/alphablocks/projects/iris/IrisAnalysis
IRIS_UX_DST=/home/developer/projects/IrisAnalysis
docker run -it
    -v  $BIO_SRC:$BIO_DST \
    -v  $FACE_RECOG_SRC:$FACE_RECOG_DST \
    -v  $IRIS_UX_SRC:$IRIS_UX_DST \
    --name iris-ocv3-qt5_dev \
    iris-ocv3-qt5:latest