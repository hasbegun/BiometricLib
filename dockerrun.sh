SRC=/Users/inho.choi/devel/alphablocks/projects/iris/BiometricLib
DST=/home/developer/projects/biometriclib
docker run -it --name irislib_dev \
    -v  $SRC:$DST \
    irislib:latest