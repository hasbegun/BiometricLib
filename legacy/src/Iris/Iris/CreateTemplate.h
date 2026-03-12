/*********************************************************//**
** @file CreateTemplate.h
** Create iris template.
**************************************************************/
#ifndef CREATE_TEMPLATE_H
#define CREATE_TEMPLATE_H

/**
 * Utility methods to create iris templates.
 */
class CreateTemplate
{
public:
	/**
	* Create an iris template for matching and non-matching.
	*
	* @param fileName	Input image file name
	* @param template1	(OUT) Returns the iris template
	* @param mask1		(OUT) Returns the iris mask
	* @param width		(OUT) Returns the template width
	* @param height		(OUT) Returns the template height
	* @param dataType	Valid \c dataType values:
	*					- \c 1 = classic still image
	*					- \c 2 = video captured at a distance (distant-videos)
	*/
	static void newCreateIrisTemplate(const char *fileName,
									  int **template1, int **mask1,
									  int *width, int *height,
									  int dataType);

};

#endif // !CREATE_TEMPLATE_H
