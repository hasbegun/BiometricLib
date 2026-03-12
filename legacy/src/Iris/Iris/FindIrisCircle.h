/*********************************************************//**
** @file FindIrisCircle.h
** Find the iris circle using Hough Transform.
**************************************************************/

#ifndef FIND_IRIS_CIRCLE_H
#define FIND_IRIS_CIRCLE_H

#include <opencv2/core/core.hpp>

/**
 * Iris boundaries detection class.
 */
class FindIrisCircle
{
public:

  /**
	* Detect the iris boundary using Hough Transform.
	*
	* @param img				Input image
	* @param rPupil			Radius of the pupil circle
	* @param uIrisRadius		Maximum radius
	* @param scaling			Scaling factor to speed up the Hough transform
	* @param lowThres		Threshold for connected edges
	* @param highThres		Threshold for creating edge map
	* @param destVal 		(OUT) Iris center points and radius (0:x, 1:y, 2:radius)
	*/
	static void doDetect(IplImage *img, int rPupil, int uIrisRadius, 
		double scaling, double lowThres, double highThres,
		int* destVal);
	
	/**
	* Find the iris center points. 
    * The distance between pupil and iris center points is constained.
	*
	* @param xyPupil		Pupil's center
	* @param xyIris		Iris' center
	* @param setPt		Points out of the ROI (Region Of Interest)
    * @param val 		Maximum distance between pupil and iris center position
	* @param dataType	Valid \c dataType values:
	*					- \c 1 = classic still image
	*					- \c 2 = video captured at a distance (distant-videos)
	* @return 			Original coordinates for pupil circle
	*/
	static CvPoint getOriginPoints(CvPoint xyPupil, CvPoint xyIris, CvPoint setPt, int val); 
};
#endif // !FIND_IRIS_CIRCLE_H
