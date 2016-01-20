#!/bin/bash

set -x

cd xcode
cmake .. -DCMAKE_BUILD_TYPE=Debug -G Xcode
open BiometricLib.xcodeproj