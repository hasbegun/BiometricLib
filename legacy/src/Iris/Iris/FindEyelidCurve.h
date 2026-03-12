/*********************************************************//**
** @file FindEyelidCurve.h
** Find the eyelid curves using Hough Transform and Lagrange interpolation.
**************************************************************/

#ifndef FIND_EYELID_CURVE_H
#define FIND_EYELID_CURVE_H

#include "ImageUtility.h"
#include "Masek/Masek.h"

/**
 * Eyelids curve detection class.
 */
class FindEyelidCurve
{
public:
	FindEyelidCurve(void);
	~FindEyelidCurve(void);

	/**
	* Detect eyelid curves.
	*
	* @param iplImg		Input image
	* @param xPupil		X coordinate of the pupil center
	* @param yPupil		Y coordinate of the pupil center
	* @param rPupil		Pupil's maxinum radius
	* @param xIris		X coordinate of the iris center
	* @param yIris		Y coordinate of the iris center
	* @param rIris		Iris' maxinum radius
	* @param x	(OUT)	X array
	* @param ty (OUT)	Y array of the upper eyelid 
	* @param by (OUT)	Y array of the lower eyelid
	*/
	static void findCurves(IplImage* iplImg, 
						 int xPupil, int yPupil,int rPupil, 
                         int xIris, int yIris, int rIris,
						 double* x, double* ty, double* by);

	/**
	* Calculate the new Y array for eyelid curves.
	*
	* @param img		Input image
	* @param x		X input array
	* @param y		Y input array
	* @param destY 	(OUT) Y array of the new eyelid
	*/
	static void calcCurvePoints(IplImage* img, double* x, double* y, int* destY);
	
	
	/**
	* Detect eyelid lines using Hough Transform.
	*
	* @param iplImg		Input image
	* @param xPupil		X coordinate of the pupil center
	* @param yPupil		Y coordinate of the pupil center
	* @param rPupil		Pupil's maxinum radius
	* @param xIris		X coordinate of the iris center
	* @param yIris		Y coordinate of the iris center
	* @param rIris		Iris' maximum radius
	* @param x	(OUT)	X array
	* @param ty (OUT)	Y array of the upper eyelid 
	* @param by (OUT)	Y array of the lower eyelid
	*/
	static void findLines(IplImage* iplImg, 
						int xPupil, int yPupil, int rPupil, 
						int xIris, int yIris, int rIris,
						double* x, double* ty, double* by);

	/**
	* Calculate the new Y array for eyelid curves.
	*
	* @param img		Input image
	* @param x		X input array
	* @param y		Y input array
	* @param destY 	(OUT) Y array of the new eyelid
	*/
	static void calcLinePoints(IplImage* img, double* x, double* y, int* destY);
	
	/**
	* Add NaN values for values outside the iris region.
	*
	* @param image	Input image
	* @param x		X input array
	* @param ty		Y input array for the upper eyelid
	* @param by		Y input array for the lower eyelid
	*/
	static void maskOutNoise(Masek::IMAGE* image, double *x, int* ty, int *by);

private:
	/**
	* Add NaN values outside the iris region.
	*
	* @param image		Input image
	* @param eyeImage	Input eyeImage
	* @param min 		(OUT) Minimum value
	* @param min 		(OUT) Maximum value
	* @param originX	Original X coordinate
	* @param originY	Original Y coordinate
	*/
	static void getEyelidPoint(Masek::IMAGE* image, 
							Masek::IMAGE* eyeImage,
							int& min, int& max, 
							int originX, int originY);
		
	/**
	* Name: Lagrange Interpolation Program
	* Description:To perform Lagrange Interpolation. 
	*             This is a numerical analysis method by
    * @param x      x-coor
    * @param f      y-coor
    * @param n      order of polynomial
    * @param xbar   value want to estimate
    *
	*/
    static double LagrangeInterpolation(double *x, double *f, int n, double xbar);

	/*static void polyFit(double *x, double *y, int n, double *a, int m, double *dt);
	static int findPara(Masek::IMAGE *image, double **lines);
	static int coordinates(Masek::filter *m_edgeim, double* x, double* y);
	*/
};
#endif // !FIND_EYELID_CURVE_H

