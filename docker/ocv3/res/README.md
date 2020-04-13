1. Need to generate key if not exist.
To generate a key
>> ssh-keygen -t rsa -b 4096 -C "your_email@example.com"
place it and copy it.

2. For mac to run X apps, install XQuartz.
    Either download https://www.xquartz.org/releases/ or
    brew update && brew cask install xquartz

3. How to run X apps on docker on Mac.
   Ref: https://medium.com/@pigiuz/hw-accelerated-gui-apps-on-docker-7fd424fe813e

   General Idea.
  --------------------------------------
 |GUI App| <--> |Grphics Lib| <--> |Xlib|
  --------------------------------------
             Application             ||
                                     ||
     ==================================
     ||          Writes
     \/
  ----------------------------------------
 |X11 socat| <--> |X Server| <--> |Display|
  ----------------------------------------

  How to set it on mac.
  Set xhost access permistion by

  >> IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')
  >> /usr/X11/bin/xhost +$IP

  Then pass at the docker run time.
      -e DISPLAY=$IP:0
      -v /tmp/.X11-unix:/tmp/.X11-unix:rw
