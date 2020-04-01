# Dev env on docker.

1. In order to set up the dev environment
    * a. Check out docker repository and make a base docker image
    ``` git clone git@github.com:hasbegun/docker.git```
    run docker build.
    ``` cd opencv2```
    ``` dockerbuild.sh```
    This creates docker image "u16-ocv2"

    * b. Make dev image
    ``` cd docker/ocv2 ```
    ``` dockerbuild.sh ```
    This creates docker image "iris" based on "u16-ocv2"

2. Run
    * a. Run docker
    ``` cd docker/ocv2 ```
    ``` dockerrun.sh ```
    
    This runs a docker image "iris_dev" and mounts src dirs to
    /home/developer/projects
        BiometricLib <br>
        face_recognition <br>
        IrisAnalysis <br>
