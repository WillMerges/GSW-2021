/**
 * Parses csv files in a given directory and graphs data based
 * on user specifications.
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
