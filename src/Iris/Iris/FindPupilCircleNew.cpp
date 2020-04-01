#include "FindPupilCircleNew.h"
#include "ImageUtility.h"
#include "ImageQuality.h"
#include <vector>
#include <algorithm>

CvBox2D cvFitEllipse2(const CvArr* array) {
    CvBox2D box;
    cv::AutoBuffer<double> Ad, bd;
    memset(&box, 0, sizeof(box));

    CvContour contour_header;
    CvSeq* ptseq = 0;
    CvSeqBlock block;
    int n;

    if(CV_IS_SEQ(array)) {
        ptseq = (CvSeq*)array;
        if(!CV_IS_SEQ_POINT_SET(ptseq))
            CV_Error(CV_StsBadArg, "Unsupported sequence type");
    } else {
        ptseq = cvPointSeqFromMat(CV_SEQ_KIND_GENERIC, array, &contour_header, &block);
	}

    n = ptseq->total;
    if(n < 5)
        CV_Error(CV_StsBadSize, "Number of points should be >= 5");

    /*
     *  New fitellipse algorithm, contributed by Dr. Daniel Weiss
     */
    CvPoint2D32f c = {0,0};
    double gfp[5], rp[5], t;
    CvMat A, b, x;
    const double min_eps = 1e-8;
    int i, is_float;
    CvSeqReader reader;

    Ad.allocate(n*5);
    bd.allocate(n);

    // first fit for parameters A - E
    A = cvMat(n, 5, CV_64F, Ad);
    b = cvMat(n, 1, CV_64F, bd);
    x = cvMat(5, 1, CV_64F, gfp);

    cvStartReadSeq(ptseq, &reader);
    is_float = CV_SEQ_ELTYPE(ptseq) == CV_32FC2;

    for(i = 0; i < n; i++) {
        CvPoint2D32f p;
        if(is_float)
            p = *(CvPoint2D32f*)(reader.ptr);
        else {
            p.x = (float)((int*)reader.ptr)[0];
            p.y = (float)((int*)reader.ptr)[1];
        }
        CV_NEXT_SEQ_ELEM(sizeof(p), reader);
        c.x += p.x;
        c.y += p.y;
    }
    c.x /= n;
    c.y /= n;

    for(i = 0; i < n; i++) {
        CvPoint2D32f p;
        if(is_float)
            p = *(CvPoint2D32f*)(reader.ptr);
        else {
            p.x = (float)((int*)reader.ptr)[0];
            p.y = (float)((int*)reader.ptr)[1];
        }
        CV_NEXT_SEQ_ELEM(sizeof(p), reader);
        p.x -= c.x;
        p.y -= c.y;

        bd[i] = 10000.0; // 1.0?
        Ad[i*5] = -(double)p.x * p.x; // A - C signs inverted as proposed by APP
        Ad[i*5 + 1] = -(double)p.y * p.y;
        Ad[i*5 + 2] = -(double)p.x * p.y;
        Ad[i*5 + 3] = p.x;
        Ad[i*5 + 4] = p.y;
    }

    cvSolve(&A, &b, &x, CV_SVD);

    // now use general-form parameters A - E to find the ellipse center:
    // differentiate general form wrt x/y to get two equations for cx and cy
    A = cvMat(2, 2, CV_64F, Ad);
    b = cvMat(2, 1, CV_64F, bd);
    x = cvMat(2, 1, CV_64F, rp);
    Ad[0] = 2 * gfp[0];
    Ad[1] = Ad[2] = gfp[2];
    Ad[3] = 2 * gfp[1];
    bd[0] = gfp[3];
    bd[1] = gfp[4];
    cvSolve(&A, &b, &x, CV_SVD);

    // re-fit for parameters A - C with those center coordinates
    A = cvMat(n, 3, CV_64F, Ad);
    b = cvMat(n, 1, CV_64F, bd);
    x = cvMat(3, 1, CV_64F, gfp);
    for(i = 0; i < n; i++) {
        CvPoint2D32f p;
        if(is_float)
            p = *(CvPoint2D32f*)(reader.ptr);
        else {
            p.x = (float)((int*)reader.ptr)[0];
            p.y = (float)((int*)reader.ptr)[1];
        }
        CV_NEXT_SEQ_ELEM(sizeof(p), reader);
        p.x -= c.x;
        p.y -= c.y;
        bd[i] = 1.0;
        Ad[i * 3] = (p.x - rp[0]) * (p.x - rp[0]);
        Ad[i * 3 + 1] = (p.y - rp[1]) * (p.y - rp[1]);
        Ad[i * 3 + 2] = (p.x - rp[0]) * (p.y - rp[1]);
    }
    cvSolve(&A, &b, &x, CV_SVD);

    // store angle and radii
    rp[4] = -0.5 * atan2(gfp[2], gfp[1] - gfp[0]); // convert from APP angle usage
    t = sin(-2.0 * rp[4]);
    if(fabs(t) > fabs(gfp[2])*min_eps)
        t = gfp[2]/t;
    else
        t = gfp[1] - gfp[0];
    rp[2] = fabs(gfp[0] + gfp[1] - t);
    if(rp[2] > min_eps)
        rp[2] = sqrt(2.0 / rp[2]);
    rp[3] = fabs(gfp[0] + gfp[1] + t);
    if(rp[3] > min_eps)
        rp[3] = sqrt(2.0 / rp[3]);

    box.center.x = (float)rp[0] + c.x;
    box.center.y = (float)rp[1] + c.y;
    box.size.width = (float)(rp[2]*2);
    box.size.height = (float)(rp[3]*2);
    if(box.size.width > box.size.height) {
        float tmp;
        CV_SWAP(box.size.width, box.size.height, tmp);
        box.angle = (float)(90 + rp[4]*180/CV_PI);
    }
    if(box.angle < -180)
		box.angle += 360;
    if(box.angle > 360)
		box.angle -= 360;

    return box;
}

void cvFitEllipse(const CvPoint2D32f* points, int count, CvBox2D* box) {
    CvMat mat = cvMat(1, count, CV_32FC2, (void*)points);
    *box = cvFitEllipse2(&mat);
}

void FindPupilCircleNew::doDetect(IplImage* img, int limitRadius, double ratio4Circle, int closeItr, int openItr,
								int m, int alpha, double norm, float nScale, int* destVal) {
	// Make a copy of the given image
	IplImage* grayImg = NULL;
	grayImg = cvCloneImage(img);

	// Find the min intensity within an image--used to determine a threshold
	double minValue, maxValue;
	CvPoint minLoc, maxLoc; //location
	cvMinMaxLoc(grayImg, &minValue, &maxValue, &minLoc, &maxLoc, 0);

	// Get threshold for detecting the pupil
	int threshold = getThreshold(grayImg, (int)minValue, alpha, norm);

	double circleRatio = ratio4Circle;
	unsigned count = 0;
	// First try
	while(circleRatio > 0.4 && destVal[2] < 1) {
		//cout << count++ <<" => the first trial to find the pupil" << endl;
		for(int i=threshold; i>=0; i=i-m)	//origin i--
		{
			getCoordinates(grayImg, closeItr, openItr, i, limitRadius, circleRatio, nScale, destVal);
			if(destVal[0] > 0 && destVal[1] > 0 && destVal[2] > 0) {
				// Draw the circle
                //cvCircle(grayImg, cvPoint(destVal[0], destVal[1]), destVal[2], CV_RGB(255,255,255), 1, 8);
                //ImageUtility::showImage("Pupil Circle", grayImg);
				break;
			}
		}
		circleRatio = circleRatio - 0.05;
   	}

#if 1   // if value is 0, the section of code will not run.
	// Second try
	closeItr = 0;
	circleRatio = ratio4Circle;
	count = 0;
	while(destVal[2] < 1 && circleRatio > 0.2) {
		cout << count ++ <<" => the second trial to find the pupil" << endl;
		for(int i=threshold+1; i < (threshold+40); i=i+m)	//origin i++
		{
			getCoordinates(grayImg, closeItr, openItr,i, limitRadius, circleRatio, nScale, destVal);
			if(destVal[0]>0 && destVal[1]>0 && destVal[2]>0) {
				// Draw the circle
                //cvCircle(grayImg, cvPoint(destVal[0], destVal[1]), destVal[2], CV_RGB(255,255,255), 1, 8);
                //ImageUtility::showImage("Pupil Circle", grayImg);
				break;
			}
		}
		circleRatio = circleRatio - 0.05;
	}
#endif

	// If both tries failed
	if(destVal[2] < 1) {
		cout << "Failed to load the pupil circle." << endl;
		destVal[0] = destVal[3];
		destVal[1] = destVal[4];
		destVal[2] = destVal[5];

		if(destVal[2] < 1 || destVal[2] > limitRadius) {
		  destVal[2] = limitRadius/2;
		}
		if(destVal[0] < 1 || destVal[0] > grayImg->width-1) {
			cout << "Failed to load the pupil's X center position." << endl;
			destVal[0] = grayImg->width/2;
		}
		if(destVal[1] < 1 || destVal[1] > grayImg->height-1) {
			cout << "Failed to load the pupil's Y center position." << endl;
			destVal[1] = grayImg->height/2;
		}
	}
	cvReleaseImage(&grayImg);
}

int FindPupilCircleNew::getThreshold(IplImage* img, int minVal, int alpha, double norm) {

	int threshold = 0;
	//double norm = 200.0;

	double n = img->width*img->height;

	ImageQuality *q = NULL;
	// Measure contrast of an image and determine the optimal threshold
	double** avgGLCM = q->mGLCM(img);
	double contrastScore = ImageQuality::_contrast(img, n, avgGLCM);
	q->_deleteGLCM(avgGLCM);
	delete q;

	if(contrastScore >= norm)
  		 contrastScore = norm;
 	threshold = (int)((contrastScore/norm+1.0)*(minVal+alpha));
	return threshold;
}

/// \todo Code needs to be re-arranged
void FindPupilCircleNew::getCoordinates(IplImage* grayImg, int closeItr, int openItr, int threshold,
									 int limitRadius, double ratio4Circle, float nScale, int* circles) {
	IplImage* destImg = NULL;
	CvMemStorage* storage = NULL;

	if(storage == NULL) {
		destImg = cvCreateImage(cvGetSize(grayImg), grayImg->depth, grayImg->nChannels);
		storage = cvCreateMemStorage(0);
	} else {
		cvClearMemStorage(storage);
	}

	for(int i=0; i<4; i++) {
		circles[i] = 0;
	}
	CvSeq* contour = NULL;
	cvThreshold(grayImg, destImg, threshold, 255, CV_THRESH_BINARY);

	// It is more efficient to blur the image after thresholding the image
	cvSmooth(destImg,destImg, CV_GAUSSIAN, 5, 5);
	// Start the morphological operators
	IplConvKernel* m_pSE = cvCreateStructuringElementEx(3, 3, 1, 1,CV_SHAPE_ELLIPSE, NULL);

	cvMorphologyEx(destImg, destImg, NULL,m_pSE, CV_MOP_CLOSE, closeItr);

	cvMorphologyEx(destImg, destImg, NULL,m_pSE, CV_MOP_OPEN, openItr);

	cvReleaseStructuringElement(&m_pSE);

	cvFindContours(destImg, storage, &contour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_TC89_L1);

	// Set up the minimum contour to ignore noise within the binary image
	const int minCount = (int) (16*nScale);

    // Find the first and second maximum contour
	int maxCount[2]={0};
	getMaxCount(contour, maxCount);
	int index = 0;
	// Get the pupil center and radius
	while(circles[2] == 0 && index < 2) {
 		getPupilPosition(contour, minCount, maxCount[index],limitRadius, ratio4Circle, circles);
		index++;
	}
	cvReleaseImage(&destImg);
	cvReleaseMemStorage(&storage);
}

void FindPupilCircleNew::getPupilPosition(CvSeq* contour, int minCount,
										  int maxCount, int limitRadius, double ratio4Circle, int *circles) {
	CvPoint* arrPoint = NULL;
	CvPoint2D32f* arr32Point = NULL;
	CvBox2D* box = NULL;
	CvPoint center;
	int radius;

	while(contour != NULL) {
		// Initialize
		center.x = 0;
		center.y = 0;
		radius = 0;

		if(CV_IS_SEQ_CURVE(contour)) {
			int count = contour->total;
			// Search the maximum contour
			if(count > minCount && count == maxCount) {
				arrPoint = (CvPoint*) malloc(count*sizeof(CvPoint));
				arr32Point = (CvPoint2D32f*) malloc((count+1)*sizeof(CvPoint2D32f));
				box = (CvBox2D*) malloc(sizeof(CvBox2D));

				// Get contour points
				cvCvtSeqToArray(contour, arrPoint, CV_WHOLE_SEQ);

				for(int i=0; i<count; i++) {
					arr32Point[i].x = (float)arrPoint[i].x;
					arr32Point[i].y = (float)arrPoint[i].y;
				}

				// Fit ellipse to the points
				cvFitEllipse(arr32Point, count, box);
				center.x = (int)box->center.x;
				center.y = (int)box->center.y;
				int height = (int)box->size.height;
				int width = (int)box->size.width;

				// \todo We assume that the pupil is a perfect circle
				radius = getRadius(width, height, limitRadius, ratio4Circle);
				/// \todo Try the fitting method in the future
				//radius = fit_circle_radius(arrPoint,center.x, center.y, radius,limitRadius);

				if(arrPoint != NULL)
					free(arrPoint);
				if(arr32Point != NULL)
					free(arr32Point);
				if(box != NULL)
					free(box);

				// Use below if the radius is bigger than the limitRadius
				circles[3] = center.x;
				circles[4]= center.y;
				circles[5] = radius;

				// Stop and draw the biggest circle
				if(0 < radius && radius < limitRadius) {
					circles[0] = center.x;
					circles[1] = center.y;
					circles[2] = radius;

					//cvCircle(grayImg, center, radius, CV_RGB(255,255,255), 1, 8);
					//ImageUtility::showImage("Seg", grayImg);
					break;
				}
			}
			contour = contour->h_next;
		}
	}
}

int FindPupilCircleNew::getRadius(int width, int height, int limitRadius, double ratio4Circle) {
	int radius = 0;
	int longRadius=0, shortRadius=0;
	if(height > width) {
		longRadius = height;
		shortRadius = width;
	} else {
		longRadius = width;
		shortRadius = height;
	}

	if(!(shortRadius < (int) (longRadius*ratio4Circle)))
		radius = cvRound(longRadius/2)+1;
	else
		radius = limitRadius + 1;

	return radius;
}

// Get the maximum contour count.
void FindPupilCircleNew::getMaxCount(CvSeq* contour, int* count)
{
	std::vector<int> ptCount;
	while(contour != NULL) {
		if(CV_IS_SEQ_CURVE(contour)) {
			ptCount.push_back(contour->total);
		}
		contour = contour->h_next;
	}
	sort(ptCount.begin(), ptCount.end());

	for(int i=0; (i< (int)ptCount.size()) && (i < 2); i++) {
		count[i] = ptCount[ptCount.size()-(i+1)];
	}
	ptCount.clear();
}
