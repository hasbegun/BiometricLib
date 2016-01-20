/*********************************************************//**
** @file Shifts.h
** Shifts the bitwise for correcting rotational inconsistency.
**************************************************************/

#ifndef SHIFTS_H
#define SHIFTS_H

#include "ImageUtility.h"

class Shifts
{
public:
	Shifts(void);
	~Shifts(void);

	/**
	* Shifts the iris patterns bitwise.
	* In order to provide the best match each shift is by two bits 
	* to the left and to the right.
	*
	* @param templates Input template
	* @param width Input width
	* @param height Input height
	* @param noshifts >0: Number of shifts to the right
	* 				<0: Number of shifts to the left
	* @param nscales  Number of filters used for encoding, needed to
    *                 determine how many bits should be moved when shifting
	* @param templatenew Used to return the shifted template
	*/
	static void X_ShiftBits(int *templates, int width, int height, 
					int noshifts, int nscales, int *templatenew); 
	
	/**
	* Shifts the iris patterns bitwise.
	* In order to provide the best match each shift is one bit up and down.
	*
	* @param templates Input template
	* @param width Input width
	* @param height Input height
	* @param noshifts >0: Number of shifts to the down (two-bit unit)
	* 				<0: Number of shifts to the up (one-bit unit)
	* @param nscales  Number of filters used for encoding, needed to
    *                 determine how many bits should be moved when shifting
	* @param templatenew Used to return the shifted template
	*/
	static void Y_ShiftBits(int *templates, int width, int height, 
					int noshifts,int nscales, int *templatenew);


};
#endif // !SHIFTS_H
