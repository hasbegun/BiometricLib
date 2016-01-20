/*********************************************************//**
** @file FindHighLights.h
** Static methods for thresholding eyelash and reflection detection.
**
**************************************************************/

#ifndef FIND_HIGHLIGHTS_H
#define FIND_HIGHLIGHTS_H

#include <opencv2/core/core.hpp>
#include "Masek/Masek.h"

/**
 * Eyelash and reflection detection class.
 */
class FindHighLights
{
public:

	/**
	* Remove eyelashes and reflections within the iris region.
	*
	* @param noiseImage		Input Image
    * @param eyelashThres	Threshold value for eyelashes
	* @param reflectThres	Threshold value for reflections
	* @return Image 			Resulting image after removing both eyelashes and reflections
	*/
	static void removeHighLights2(Masek::IMAGE* noiseImage, int eyelashThres, int reflectThres);

};

#endif // !FIND_HIGHLIGHTS_H 

