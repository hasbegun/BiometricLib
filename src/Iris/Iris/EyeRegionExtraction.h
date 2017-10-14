/*********************************************************//**
** @file EyeRegionExtraction.h
** Extract the iris images from the face-visible video frames.
**
**************************************************************/

#ifndef EYE_REGION_EXTRACTION_H
#define EYE_REGION_EXTRACTION_H

#include "ImageUtility.h"

class EyeRegionExtraction
{
public:

	/**
	* Detect the pupil's position for both left and right, align the degree difference 
    * and then extract the eye region.
	*
	* @param currentImg	Input image
	* @param rPupilMax	Pupil maximum radius
	* @param rIrisMax	Iris maximum radius
	* @param ratio4Circle	Select the greater length bewteen width and height under the size condition
	* @param closeItr		Number of iteration for closing
	* @param openItr		Number of iteration for opening
	* @param speed_m	Boost speed (Default=1: larger number is less precise)
	* @param alpha		Additional value for determining threshold
    * @param format		Image format (default: BMP)
	* @param lImg 		(OUT) Aligned left eye image
	* @param rImg 		(OUT) Aligned left eye image
	*/
    static void doExtract(IplImage* currentImg, int rPupilMax, int rIrisMax, double ratio4Circle, int closeItr, int openItr,
						int speed_m, int alpha, double norm, float nScale, const char* format, IplImage*& lImg, IplImage*& rImg);
};
#endif // !EYE_REGION_EXTRACTION_H

