/*********************************************************//**
** @file ImageQuality.h
** Image Quality Measurement.
**
**************************************************************/

#ifndef IMAGE_QUALITY_H
#define IMAGE_QUALITY_H

#include "ImageUtility.h"

// \todo Valid edge Detector type =>characterization?
//#define SOBEL_EDGE	1 /// Detector type: Sobel edge
//#define QUICK_EDGE	2 /// Detector type: Quick edge
//#define CANNY_EDGE	3 /// Detector type: Canny edge

/**
 * Image Qt GUI class forward declarations.
 */
class ImageWidget;
class QImage;

/**
 * Class for measuring the image quality.
 */
class ImageQuality
{
public:    
  
	/** 
	 * Main function - computes all statistics and edge operators.
	 *
	 * @param img		Input image
	 * @param factors	Edge Detector type
	 * @return			Score
	 * @see calcSobelEdge
	 */
	static double doProcess(IplImage* img, int factors); 

	/** 
	 * Calculates Sobel score using the edge-based spacial domain approach.
	 *
	 * @param img		Input image	 
	 * @param numPix	(OUT) Total number of pixels
	 * @return			Gradient magnitude or score
	 */
	static double calcSobelEdge(IplImage* img, int& numPix);


	/** 
	 * Calculate Contrast using second-order statistics 
	 * based on GLCM (Gray Level Co-occurence Matrics).
	 *
	 * @param img	Input image
	 * @param n		Total number of pixels
	 * @param _glcm	Input gray level co-occurence matrics
	 * @return		Contrast value
	 */
	static float _contrast(IplImage* img, double n, double** _glcm);


	/** 
	 * Creates a direction independent GLCM.
	 *
	 * @note Need to deallocate manually using _deleteGLCM
	 *
	 * @param img	Input image
	 * @return		GLCM ([256][256])
	 */
	static double** mGLCM(IplImage* img);
	
	/** 
	 * Creates a direction dependent GLCM.
	 *
	 * @note Need to deallocate manually using _deleteGLCM
	 *
	 * @param img	Input image
	 * @param phi	Angle - valid \c directions:
	 *				- \c 0 (default)
	 *				- \c 45
	 *				- \c 90
	 *				- \c 135
	 * @return		GLCM ([256][256])
	 */
	static double** GLCM(IplImage* img, int phi);

	/** 
	 * Frees a GLCM.
	 *
	 * @param _glcm GLCM that should be deallocated
	 * @see mGLCM
	 * @see GLCM
	 */
	static void _deleteGLCM(double** _glcm);	

	/**
	* Creates an empty (all values = 0.0) image quality matrix.
	* @param _tmp	Return matrix template
	* @param width	Matrix width
	* @param height Matrix height
	*/
	static void _create(double**& _tmp, int width, int height);

	/**
	* Deallocates a passed image quality matrix.
	* @param _tmp	Matrix template
	* @param size	Matrix size
	*/
	static void _delete(double** _tmp, int size);

};
#endif // !IMAGE_QUALITY_H

