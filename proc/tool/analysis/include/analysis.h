/**
 * Parses csv files in a given directory and graphs data based
 * on user specifications.
 *
 * Required Flag Usage:
 *  -x flag specifies the the column being read as the X value
 *  -y flag specifies the the column being read as the Y value
 *  -d flag specifies the directory to read csv files
 *     NOTE: Should be labelled log<NUMBER>.csv since GSW generates files that way
 *
 * Optional Flag Usage:
 *  -s flag specifies the time period to analyze specifically
 *
 *  Example Usages:
 *      ./analysis -x MILLISEC -y Y_DATA -d data -s 1000 10000
 *      ./analysis -y Y_DATA -x MILLISEC -d data -s 1000 10000
 *      ./analysis -x MILLISEC -y Y_DATA -d data
 *
 *
 * @author Aaron Chan
 */

#ifndef DATA_ANALYSIS_ANALYSIS_H
#define DATA_ANALYSIS_ANALYSIS_H


#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

#include "matplotlibcpp.h"

/**
 * Utility function that combines milli and micro seconds into a single value
 *
 * @param milli
 * @param micro
 * @return
 */
ulong timeCombine(long milli, long micro);

/**
 * Utility function for parsing arguments
 *
 * @param argc
 * @param argv
 */
bool parseArgs(int argc, char *argv[]);

/**
 * Parses a given csv file and returns an array of two vectors
 * storing x and y values
 *
 * @param filename
 * @param xCol
 * @param yCol
 * @return vector<vector<double>>
 */
std::array<std::vector<npy_double>, 2> parseCSV();

/**
 * Splits a line of comma separated values into a vector of strings
 * @param str
 * @return
 */
std::vector<std::string> splitCSVLine(const std::string &str);


/**
 * Takes parsed data and graphs it
 * @param startTime
 * @param endTime
 * @param directory
 * @param xCol
 * @param yCol
 */
void plotData();

#endif //DATA_ANALYSIS_ANALYSIS_H
