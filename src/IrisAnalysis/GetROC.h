
#ifndef __GET_ROC_H__
#define __GET_ROC_H__

#include <iostream>
#include <vector>

using namespace std;

class GetROC
{
public:
    
    /**
     * Get ROC scores
     *
     * @param colNum				Start from -1
     * @param skip				Number of rows to skip (3 for our case)
     * @param matchFName			Input matching score file name
     * @param nonmatchFName		Input nonmatching score file name
     * @param outputFName		Output ROC score file name
     * @param pos				Valid \c pos values:
     *							- \c 1: Left
     *							- \c 2: Right		
     * @param index   			Experiment tag number
     */
    static void rocResult(int colNum, 
                          int skip, 
                          const char* matchFName, 
                          const char* nonMatchFName, 
                          const char* outputFName,
                          int pos, 
                          int index);
    
    
    /**
     * Sort scores and rescale the length of nonmatching scores relavant to the matching length
     *
     * @param matchScores		match score vector
     * @param nonmatchScores		nonmatch score vector
     * @param newMatchScores		(OUT) sorted match score vector
     * @param newNonmatchScores	(OUT) sorted/rescaled nonmatch score vector
     */
    static void doNewOrderedScores(vector<float> matchScores,
                                   vector<float> nonMatchScores,
                                   vector<float>& newMatchScores,
                                   vector<float>& newNonMatchScores);
    
    /**
     * Create ROC scores
     *
     * @param outputFName		Output ROC score file name
     * @param matchScores		Input matching score vector
     * @param nonmatchScores		Input nonmatching score vector	
     * @param pos				Valid \c pos values:
     *							- \c 1: Left
     *							- \c 2: Right		
     * @param index   			Experiment tag number
     */	
    static void createROC(const char* outputFName, 
                          vector<float> matchScores,
                          vector<float> nonMatchScores,
                          int pos, 
                          int index);
    
    /**
     * Get reduced ROC scores
     *
     * @param colNum				Start from -1
     * @param skip				Number of rows to skip (3 for our case)
     * @param inputFName			Input file name with the full ROC score
     * @param outputFName		(OUT) output file name with the reduced ROC score
     * @param pos				Valid \c pos values:
     *							- \c 1: Left
     *							- \c 2: Right		
     * @param index   			Experiment tag number
     */
    static void reducedScores1(int colNum, 
                               int skip,
                               const char* inputFName,
                               const char* outputFName,
                               int pos, 
                               int index);	
    
};

#endif // !__GET_ROC_H__
