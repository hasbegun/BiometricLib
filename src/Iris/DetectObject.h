#ifndef DETECTOBJECT_H
#define DETECTOBJECT_H

#include <stdio.h>
#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// Search for a single object in the image, such as the largetst face, storing the result into 'largest object'
// Can use haar or lbp cascades for face Detection, or even eye, mouth, or car detection.
// Input is temporarily shrunk to 'scaledWidth' for faster detection.
// 240 is enough to find faces
void detectLargestObject(const Mat &img, CascadeClassifier &cascade, Rect &largestObject, int scaledWidth = 320);

// Search for many objects in the image, such as all faces, storing the results into 'objects'
void detectManyObject(const Mat &img, CascadeClassifier &cascade, Rect &largestObject, int scaledWidth = 320);

#endif // DETECTOBJECT_H
