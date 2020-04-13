#!/bin/bash

if [ -z "$1" ]; then
    echo "Container name or ID is not provided. \"iris-ocv3-qt5_dev\" is used."
    CONTAINER="iris-ocv3-qt5_dev"
else
    CONTAINER=$1
fi
echo "Container \"$CONTAINER\" ip"
docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $CONTAINER
