#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>

// Function to unwrap phase values and make sure of 
// the continuity along the phase discontinuities
double unwrapPhase(double phase, double oldPhase) {
    double differene = phase - oldPhase;

    if (differene > CV_PI) {
        return phase - 2 * CV_PI;
    }
    else if (differene < (-CV_PI)) {
        return phase + 2 * CV_PI;
    }
    else {
        return phase;
    }
}

int main() {
    int width = 1280, height = 720, // width and height for the sinusoidal pattersn
        numPatterns = 6,
        period = 10; // yeilding period
    double phaseShift = 60.0; // degree of shift

    std::vector<cv::Mat> patterns;
    
    // creating sinusoidal patterns with the specifed specs
    for (int x = 0; x < numPatterns; x++) {
        cv::Mat pattern(height, width, CV_8UC1);

        // the phase off set for each pattern
        double phaseOffSet = (x * phaseShift) * (CV_PI / 180.0);

        for (int y = 0; y < height; ++y) {
            for (int z = 0; z < width; ++z) {
                double value = 127.5 * (1 + std::sin(2 * CV_PI * z / period + phaseOffSet));
                pattern.at<uchar>(y, z) = static_cast<uchar>(value);
            }
        }

        patterns.push_back(pattern);
        std::string filename = "./sinusoidal_patterns/pattern_" + std::to_string(x) + ".png";
        cv::imwrite(filename, pattern);
    }
    std::cout << "Patterns generated." << std::endl;

    // Calculate and unwrap the phase map
    cv::Mat unwrappedPhaseMap(height, width, CV_64F, cv::Scalar(0));

    for (int i = 0; i < numPatterns - 2; i++) {
        cv::Mat phaseMap(height, width, CV_64F); 

        for (int y = 0; y < height; ++y) {
            for (int z = 0; z < width; ++z) {
                if (i + 2 < patterns.size()) {
                    double I1 = static_cast<double>(patterns[i + 2].at<uchar>(y, z));
                    double I2 = static_cast<double>(patterns[i + 1].at<uchar>(y, z));
                    double I3 = static_cast<double>(patterns[i].at<uchar>(y, z));

                    double phaseValue = std::atan(std::sqrt(3) * (I1 - I3) / (2 * I2 - I1 - I3));
                    phaseMap.at<double>(y, z) = phaseValue;
                }
            }
        }

        for (int y = 0; y < height; ++y) {
            for (int z = 0; z < width; ++z) {
                if (i + 2 < patterns.size()) {
                    double phaseValue = phaseMap.at<double>(y, z);
                    double prevPhase = unwrappedPhaseMap.at<double>(y, z);
                    double unwrappedPhase = unwrapPhase(phaseValue, prevPhase);
                    unwrappedPhaseMap.at<double>(y, z) = unwrappedPhase;
                }
            }
        }
    }

    // Load fringe order patterns
    std::vector<cv::Mat> fringeOrderPatterns;

    for (int i = 0; i < 7; i++) {
        std::string patternFilename = "./sinusoidal_patterns/gray_pattern_" + std::to_string(i) + ".png";
        cv::Mat pattern = cv::imread(patternFilename, cv::IMREAD_GRAYSCALE);
        fringeOrderPatterns.push_back(pattern);
    }

    // Decode gray-coded images to obtain decimal matrix
    cv::Mat decimalMatrix(height, width, CV_32S, cv::Scalar(0));

    for (int y = 0; y < height; y++) {
        int binaryValue = 0;
        for (int i = 6; i >= 0; i--) {
            binaryValue = (binaryValue << 1) | fringeOrderPatterns[i].at<uchar>(y, 0);
        }
        for (int z = 0; z < width; z += 10) {
            decimalMatrix.at<int>(y, z) = binaryValue;
        }
    }

    // Unwrap phase map using fringe orders and plot a single row
    int rowToPlot = height / 2; 

    cv::Mat unwrappedPhaseLine(width, 1, CV_64F, cv::Scalar(0)); 

    for (int z = 0; z < width; z++) {
        double wrappedPhase = unwrappedPhaseMap.at<double>(rowToPlot, z);
        int fringeOrder = decimalMatrix.at<int>(rowToPlot, z);
        double unwrappedPhase = wrappedPhase + 2 * CV_PI * fringeOrder;
        unwrappedPhaseLine.at<double>(z, 0) = unwrappedPhase;
    }

    // Plot the unwrapped phase line
    cv::Mat unwrappedPhasePlot(height, width, CV_8U, cv::Scalar(0));
    for (int z = 0; z < width; z++) {
        int value = static_cast<int>((unwrappedPhaseLine.at<double>(z, 0) + CV_PI) / (2 * CV_PI) * 255);
        unwrappedPhasePlot.at<uchar>(rowToPlot, z) = static_cast<uchar>(value);
    }

    cv::imshow("Unwrapped Phase Line", unwrappedPhasePlot);
    cv::waitKey(0);

    std::cout << "Unwrapped phase map calculated and displayed." << std::endl;

    return 0;
}
