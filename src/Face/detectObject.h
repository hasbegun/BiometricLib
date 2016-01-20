/*****************************************************************************
*   Face Recognition using Eigenfaces or Fisherfaces
******************************************************************************/

//////////////////////////////////////////////////////////////////////////////////////
// detectObject.cpp, by Shervin Emami (www.shervinemami.info) on 30th May 2012.
// Easily detect objects such as faces or eyes (using LBP or Haar Cascades).
//////////////////////////////////////////////////////////////////////////////////////

#ifndef DETECT_OBJECT
#define DETECT_OBJECT

#pragma once


#include <stdio.h>
#include <iostream>
#include <vector>

// Include OpenCV's C++ Interface
#include "opencv2/opencv.hpp"


using namespace cv;
using namespace std;

class DetectObject {
public:
    DetectObject(void);

    ~DetectObject(void);

    static void detectLargestObject(const Mat &img, CascadeClassifier &cascade, Rect &largestObject,
                                    int scaledWidth = 320);
    static void detectManyObjects(const Mat &img, CascadeClassifier &cascade, vector<Rect> &objects, int scaledWidth = 320);
};

    // Search for just a single object in the image, such as the largest face, storing the result into 'largestObject'.
// Can use Haar cascades or LBP cascades for Face Detection, or even eye, mouth, or car detection.
// Input is temporarily shrunk to 'scaledWidth' for much faster detection, since 240 is enough to find faces.
// Note: detectLargestObject() should be faster than detectManyObjects().
//void detectLargestObject(const Mat &img, CascadeClassifier &cascade, Rect &largestObject, int scaledWidth = 320);


// Search for many objects in the image, such as all the faces, storing the results into 'objects'.
// Can use Haar cascades or LBP cascades for Face Detection, or even eye, mouth, or car detection.
// Input is temporarily shrunk to 'scaledWidth' for much faster detection, since 240 is enough to find faces.
// Note: detectLargestObject() should be faster than detectManyObjects().
//void detectManyObjects(const Mat &img, CascadeClassifier &cascade, vector<Rect> &objects, int scaledWidth = 320);

#endif