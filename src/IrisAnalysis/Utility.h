
#ifndef __UTILITY_H
#define __UTILITY_H

#include <iostream>
#include <vector>
#include <stdio.h>
#include <algorithm>
#include "string.h"

using namespace std;

class Utility
{
public:
    Utility(void);
    ~Utility(void);
    
    /**
     * Read single column from a file and save it into vector.
     *
     * @param fname		File name
     * @param location	Column number where needs to be extracted
     * @param skip		Number of rows to skip (3)
     * @param val		(OUT) extracted data from the column
     */
    static int readSingleColumn(const char* fname,
                                int location,
                                int skip,
                                std::vector<float> &val);
    
    /**
     * Read four columns from a file and save it into vectors.
     *
     * @param fname		File name
     * @param location	Starting column number where needs to be extracted
     * @param skip		Number of rows to skip (3)
     * @param val1		(OUT) extracted data from the column1
     * @param val2		(OUT) extracted data from the column2
     * @param val3		(OUT) extracted data from the column3
     * @param val4		(OUT) extracted data from the column4
     */
    static int readFourScores(const char* fname,
                              int location,
                              int skip,
                              std::vector<float> &val1,
                              std::vector<float> &val2,
                              std::vector<float> &val3,
                              std::vector<float> &val4);
    
    /**
     * Write data.
     */
    static void appendToFile(const char* outputFileName,
                             const char* fmt, ...);
    
    /**
     * Get partial file name.
     */
    static char* getFilenamePart(char* buffer);
    
    /**
     * \n to \0.
     */
    static void stripLineBreak(char* buffer);
    
protected:
    
    /**
     * give a number of tabs as a option.
     */
    static char* jumpToColumn(char* buf,
                              int colNo,
                              int tabNum);
    
    /**
     * Indicate how many tab or space you used--default: 1.
     */
    static char* getColumn(char** buf,
                           int tabNum);
    
    
};
#endif // !__Utility_H
