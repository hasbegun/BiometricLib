# Dev env on docker.

1. In order to set up the dev environment
    * a. Check out docker repository and make a base docker image<br>
    ``` git clone git@github.com:hasbegun/docker.git```<br>
    run docker build.<br>
    ``` cd opencv2```<br>
    ``` dockerbuild.sh```<br>
    This creates docker image "u16-ocv2"

    * b. Make dev image
    ``` cd docker/ocv2 ```<br>
    ``` dockerbuild.sh ```<br>
    This creates docker image "iris" based on "u16-ocv2"

2. Run
    * a. Run docker
    ``` cd docker/ocv2 ```<br>
    ``` dockerrun.sh ```<br>
    
    This runs a docker image "iris_dev" and mounts src dirs to<br>
    /home/developer/projects <br>
        BiometricLib <br>
        face_recognition <br>
        IrisAnalysis <br>
