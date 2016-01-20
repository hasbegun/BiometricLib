/*********************************************************//**
** @file MatchAlg.h
** Functions to match one template to another template.
** 
**************************************************************/
#ifndef MATCH_ALG_H
#define MATCH_ALG_H

#include "ImageUtility.h"

/*
* Matching of two templates.
*/
class MatchAlg 
{
public:
	/**
	* Match gallery template to probe template.
	*
	* @param galleryName	Input gallery image file name
	* @param probeName		Input probe image file name
	* @param gDataType		Valid \c dataType values:
	*						- \c 1 = classic still image
	*						- \c 2 = video captured at a distance (distant-videos)
	* @param pDataType		Valid \c dataType values:
	*						- \c 1 = classic still image
	*						- \c 2 = video captured at a distance (distant-videos)
	* @return Hamming distance value.
	*/
    static double mainMatchAlg(char *galleryName, char *probeName, int gDataType, int pDataType);

    /**
    * Generate templete for a given eye image.
    *
    * @param eyeFileName	Input image file name
    * @param dataType		Valid \c dataType values:
    *						- \c 1 = classic still image
    *						- \c 2 = video captured at a distance (distant-videos)
    * @return 0/1           - 0 = successful run
    *                       - 1 = Error.
    */
    static int oneEyeAnalysis(char *eyeFileName, int dataType);
};
#endif // !MATCH_ALG_H
