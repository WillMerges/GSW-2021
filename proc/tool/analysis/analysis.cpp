#include "include/analysis.h"
#include <sstream>
#include <fstream>
#include <experimental/filesystem>


using namespace std;
namespace plt = matplotlibcpp;
namespace fs = std::experimental::filesystem;


static int block = 1;
static uint64_t startX = 0;
static uint64_t endX = 18446744073709551; // Literally the end of time
static bool finishedReading = false;
static string xAxis;
static string yAxis;
static string directory;

uint64_t timeCombine(uint64_t milli, uint64_t micro) {
    return (milli * 1000) + micro;
}

bool parseArgs(int argc, char *argVector[]) {
    bool directorySet = false;
    bool xSet = false;
    bool ySet = false;
    bool helpSet = false;

    /**
     * -t <start_time> <end_time>
     * -b <time_block>
     * -d <directory>
     * -h <help>
     * -x <x_axis>
     * -y <y_axis>
     */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argVector[i], "-s") == 0) {
            startX = atoi(argVector[i + 1]);
            endX = atoi(argVector[i + 2]);

            i += 2;
        } else if (strcmp(argVector[i], "-b") == 0) {
            block = atoi(argVector[i + 1]);

            i++;
        } else if (strcmp(argVector[i], "-d") == 0) {
            if (i + 1 >= argc) {
                cout << "Error: -d requires a directory path" << endl;
                return false;
            } else {
                directory = argVector[i + 1];
                directorySet = true;
            }

            i++;
        } else if (strcmp(argVector[i], "-x") == 0) {
            if (i + 1 >= argc) {
                cout << "Error: -x requires an x variable" << endl;
            } else {
                xSet = true;
                xAxis = argVector[i + 1];
            }

            i++;
        } else if (strcmp(argVector[i], "-y") == 0) {
            if (i + 1 >= argc) {
                cout << "Error: -y requires a y variable" << endl;
            } else {
                ySet = true;
                yAxis = argVector[i + 1];
            }

            i++;
        } else if (strcmp(argVector[i], "-h") == 0) {
            cout << "Usage: ./analysis -s <start_x> <end_x> -b <time_block> -d <directory>" << endl;
            helpSet = true;
            break;
        }
    }

    if (!(!helpSet && directorySet && xSet && ySet)) {
        cout << "Error: Missing required arguments" << endl;
        return false;
    }

    return true;

}


vector<string> splitCSVLine(const string &str) {
    vector<string> result;

    stringstream ss(str);
    string token;

    while (getline(ss, token, ',')) {
        result.push_back(token);
    }
    return result;
}

array<vector<npy_double>, 2> parseCSV(const string &filename) {
    int millisecColNum = -1;
    int microsecColNum = -1;
    int xColNum = -1;
    int yColNum = -1;

    if (!fs::exists(filename)) {
        cout << "File does not exist" << endl;
        return {};
    }

    ifstream file(filename);
    vector<npy_double> xVals;
    vector<npy_double> yVals;

    string line;

    getline(file, line);
    vector<string> headers = splitCSVLine(line);

    for (int i = 0; i < headers.size(); i++) {
        if (headers[i] == "MILLISEC") {
            millisecColNum = i;
        }

        if (headers[i] == "MICROSEC") {
            microsecColNum = i;
        }

        if (headers[i] == xAxis) {
            xColNum = i;
        }

        if (headers[i] == yAxis) {
            yColNum = i;
        }
    }

    if (millisecColNum == -1 || microsecColNum == -1 || xColNum == -1 || yColNum == -1) {
        cout << "Columns not found" << endl;
        return {};
    }

    if (file.is_open()) {
        double blockXSum = 0;
        double blockYSum = 0;
        int lineCount = 0;

        while (!finishedReading && getline(file, line)) {
            vector<string> lineVals = splitCSVLine(line);

            if (lineVals[millisecColNum].empty() && lineVals[microsecColNum].empty()) {
                continue;
            }

            double xVal = stod(lineVals[xColNum]);
            double yVal = stod(lineVals[yColNum]);


            if (xVal < startX) {
                continue;
            }

            cout << xVal << " " << endX << endl;
            if (xVal > endX) {
                cout << "End" << endl;
                finishedReading = true;
                break;
            }


            blockXSum += xVal;
            blockYSum += yVal;
            lineCount++;

            if (xVal - startX > block) {
                xVals.push_back(blockXSum / lineCount);
                yVals.push_back(blockYSum / lineCount);
                blockXSum = 0;
                blockYSum = 0;
                lineCount = 0;

                startX = xVal;
            }
        }

    }

    file.close();

    return {xVals, yVals};
}

void plotData() {
    array<vector<npy_double>, 2> csvData;
    array<vector<npy_double>, 2> newData;
    int fileCount = 1;
//    npy_double t = milliseconds;
//    t += microseconds * 1000

    plt::xlabel(xAxis);
    plt::ylabel(yAxis);

    // While file exists
    while (fs::exists(directory + "/" + "log" + to_string(fileCount) + ".csv")) {
        newData = parseCSV(directory + "/" + "log" + to_string(fileCount) + ".csv");

        csvData[0].insert(csvData[0].end(), newData[0].begin(), newData[0].end());
        csvData[1].insert(csvData[1].end(), newData[1].begin(), newData[1].end());

        if (finishedReading) {
            break;
        }

        fileCount++;
    }

    plt::plot(csvData[0], csvData[1]);

    plt::save("plot.pdf");
}


int main(int argc, char *argv[]) {
    bool argSuccess = parseArgs(argc, argv);
    if (!argSuccess) {
        return 1;
    }


    plotData();

    return 0;
}