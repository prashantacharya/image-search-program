// Copyright [2025] <Prashant Acharya>

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <omp.h>
#include "PNG.h"

// It is ok to use the following namespace delarations in C++ source
// files only. They must never be used in header files.
using namespace std;
using namespace std::string_literals;


/**
 * This method computes the average background color of a region in an image.
 * * \param[in] img1 The main image in which the region is to be searched.
 * \param[in] mask The mask image that is used to determine which pixels
 * in the main image are to be considered.
 * \param[in] startRow The starting row of the region in the main image.
 * \param[in] startCol The starting column of the region in the main image.
 * \param[in] maxRow The ending row of the region in the main image.
 * \param[in] maxCol The ending column of the region in the main image.
 * * \return The average color of the region in the main image.
 */
Pixel computeBackgroundPixel(const PNG &img, const PNG &mask,
                             const int startRow, const int startCol,
                             const int maxRow, const int maxCol) {
    const Pixel Black{.rgba = 0xff'00'00'00U};
    int red = 0, blue = 0, green = 0, count = 0;

    #pragma omp parallel for reduction(+:red, green, blue, count) collapse(2)
    for (int row = 0; (row < maxRow); row++) {
        for (int col = 0; (col < maxCol); col++) {
            if (mask.getPixel(row, col).rgba == Black.rgba) {
                const auto pix = img.getPixel(row + startRow, col + startCol);
                red += pix.color.red;
                green += pix.color.green;
                blue += pix.color.blue;
                count++;
            }
        }
    }
    const unsigned char avgRed = (red / count), avgGreen = (green / count),
                        avgBlue = (blue / count);
    return {.color = {avgRed, avgGreen, avgBlue, 0}};
}


/**
 * This method checks if two pixels are the same shade of color.
 * * \param[in] pix1 The first pixel to be compared.
 * \param[in] pix2 The second pixel to be compared.
 * \param[in] tolerance The absolute acceptable difference between each color
 * * \return true if the two pixels are the same shade of color, false otherwise.
*/
bool isSameShade(const Pixel &pix1, const Pixel &pix2, const int tolerance) {
    return (std::abs(pix1.color.red - pix2.color.red) <= tolerance)
        && (std::abs(pix1.color.green - pix2.color.green) <= tolerance)
        && (std::abs(pix1.color.blue - pix2.color.blue) <= tolerance);
}

/**
 * This method draws a box around a region in an image.
 * * \param[in] png The image in which the box is to be drawn.
 * \param[in] row The starting row of the region in the image.
 * \param[in] col The starting column of the region in the image.
 * \param[in] width The width of the region in the image.
 * \param[in] height The height of the region in the image.
 */
void drawBox(PNG& png, int row, int col, int width, int height) {
    // Draw horizontal lines
    #pragma omp parallel for
    for (int i = 0; (i < width); i++) {
        png.setRed(row, col + i); 
        png.setRed(row + height - 1, col + i);
    }
    // Draw vertical lines
    #pragma omp parallel for
    for (int i = 0; (i < height); i++) { 
        png.setRed(row + i, col); 
        png.setRed(row + i, col + width - 1);
    }
}


/**
 * This method draws all the boxes around the matching regions in the main image.
 * * \param[in] mainImage The main image in which the boxes are to be drawn.
 * \param[in] maskImage The mask image that is used to determine which pixels
 * in the main image are to be considered.
 * \param[in] matchLocations The vector of match locations.
 */
void drawAllBoxes(PNG& mainImage, const PNG& maskImage, 
    const vector<pair<int, int>>& matchLocations) {
    #pragma omp parallel for
    for (size_t i = 0; i < matchLocations.size(); i++) {
        drawBox(mainImage, matchLocations[i].first, matchLocations[i].second,
                maskImage.getWidth(), maskImage.getHeight());
    }
}


/**
 * This method prints the RGBA color values of a pixel.
 * * \param[in] pix The pixel whose RGBA color values are to be printed.
 */
void printRGBAColor(const Pixel &pix) {
    std::cout << "Red: " << static_cast<int>(pix.color.red) << std::endl;
    std::cout << "Green: " << static_cast<int>(pix.color.green) << std::endl;
    std::cout << "Blue: " << static_cast<int>(pix.color.blue) << std::endl;
    std::cout << "Alpha: " << static_cast<int>(pix.color.alpha) << std::endl;
}


/*
 * This method checks if the pattern in the mask image matches the pattern
 * in the main image at the specified row and column.
 *
 * \param[in] img The main image in which the pattern is to be searched.
 * \param[in] mask The mask image that is used to determine which pixels
 * in the main image are to be considered.
 * \param[in] row The row of the main image in which the pattern is to be searched.
 * \param[in] col The column of the main image in which the pattern is to be searched.
 *
 * \return The number of matching pixels.
*/
int countMatchingPixels(const PNG &img, const PNG &mask,
                        int row, int col, int tolerance) {
    int matchPixCount = 0;
    int maskHeight = mask.getHeight();
    int maskWidth = mask.getWidth();
    const auto bgPixel = computeBackgroundPixel(img, mask,
                                                row, col,
                                                maskHeight, maskWidth);
    const Pixel Black{.rgba = 0xff000000U};

    #pragma omp parallel for reduction(+:matchPixCount) collapse(2)
    for (int r = 0; r < maskHeight; r++) {
        for (int c = 0; c < maskWidth; c++) {
            const Pixel &maskPix = mask.getPixel(r, c);
            const Pixel &imgPix = img.getPixel(row + r, col + c);
           
            if ((maskPix.rgba == Black.rgba &&
                 isSameShade(imgPix, bgPixel, tolerance)) ||
                (maskPix.rgba != Black.rgba &&
                 !isSameShade(imgPix, bgPixel, tolerance))) {
                matchPixCount++;
            }
        }
    }
    return matchPixCount;
}



/*
 * This method prints the matching regions in the mask image.
 * * \param[in] matchLocations The vector of match locations.
 * \param[in] mask The mask image that is used to determine which pixels
 * in the main image are to be considered.
 */
void printMatchingRegions(const vector<pair<int, int>> &matchLocations,
                          const PNG &mask) {
    vector<pair<int, int>> sortedLocations = matchLocations;
    std::sort(sortedLocations.begin(), sortedLocations.end());

    int maskHeight = mask.getHeight();
    int maskWidth = mask.getWidth();

    for (const auto &loc : sortedLocations) {
        int rowStart = loc.first;
        int colStart = loc.second;
        int rowEnd = rowStart + maskHeight;
        int colEnd = colStart + maskWidth;
        std::cout << "sub-image matched at: " 
                    << rowStart << ", " << colStart << ", " 
                    << rowEnd << ", " << colEnd << std::endl;
    }
    
    std::cout << "Number of matches: " << matchLocations.size() << std::endl;
}


/**
 * Helper function to process a single potential match.
 *
 * \param[in] mainImage The main image.
 * \param[in] maskImage The mask image.
 * \param[in,out] matchLocations The vector of found matches.
 * \param[in] requiredMatch The required number of matches.
 * \param[in] tolerance The color tolerance.
 * \param[in] row The current row.
 * \param[in] col The current column.
 */
void processPotentialMatchParallel(
    const PNG &mainImage, const PNG &maskImage, 
    int requiredMatch, int tolerance, int row, int col, 
    vector<pair<int,int>> &matchLocations) {

    const int total = maskImage.getWidth() * maskImage.getHeight();
    int count = countMatchingPixels(mainImage, 
        maskImage, row, col, tolerance);
    int netMatches = count - (total - count);

    if (netMatches > requiredMatch) {
        matchLocations.emplace_back(row, col);
    }
}


/**
 * This method removes overlapping matches from a vector of raw matches.
 *
 * \param[in] raw   Vector of all raw matches.
 * \param[in] mask  Mask image for width/height.
 * \return          Vector of non-overlapping matches.
 */
vector<pair<int,int>> filterOverlaps(const vector<pair<int,int>>& rawMatches, 
                                            const PNG &mask) {
    if (rawMatches.empty()) return {};
    vector<pair<int,int>> sorted = rawMatches, nonOverlappingMatches;
    sort(sorted.begin(), sorted.end());

    int mh = mask.getHeight(), mw = mask.getWidth();
    for (auto &match : sorted) {
        int r1 = match.first, c1 = match.second;
        int r1e = r1 + mh, c1e = c1 + mw;
        bool overlap = false;

        for (auto &validMatch : nonOverlappingMatches) {
            int r2 = validMatch.first, c2 = validMatch.second;
            if (c1 < c2 + mw && c1e > c2 &&
                r1 < r2 + mh && r1e > r2) {
                overlap = true;
                break;
            }
        }
        if (!overlap) nonOverlappingMatches.push_back(match);
    }
    return nonOverlappingMatches;
}


/**
 * This method finds the raw matches in the main image.
 *
 * \param[in] mainImg The main image.
 * \param[in] maskImg The mask image.
 * \param[in] requiredMatch The required number of matches.
 * \param[in] tolerance The color tolerance.
 * \return The vector of raw matches.
 */
vector<pair<int, int>> findRawMatches(
    const PNG &mainImg, const PNG &maskImg,
    int requiredMatch, int tolerance) {
    vector<pair<int, int>> raw;

    #pragma omp parallel
    {
        vector<pair<int, int>> local;

        #pragma omp for collapse(2)
        for (int r = 0; r <= mainImg.getHeight() - maskImg.getHeight(); r++)
            for (int c = 0; c <= mainImg.getWidth() - maskImg.getWidth(); c++)
                processPotentialMatchParallel(
                    mainImg, maskImg, requiredMatch,
                    tolerance, r, c, local);

        #pragma omp critical
        raw.insert(raw.end(), local.begin(), local.end());
    }
    return raw;
}



/**
 * This is the top-level method that is called from the main method to 
 * perform the necessary image search operation. 
 * * \param[in] mainImageFile The PNG image in which the specified searchImage 
 * is to be found and marked (for example, this will be "Flag_of_the_US.png")
 * * \param[in] srchImageFile The PNG sub-image for which we will be searching
 * in the main image (for example, this will be "star.png" or "start_mask.png") 
 * * \param[in] outImageFile The output file to which the mainImageFile file is 
 * written with search image file highlighted.
 * * \param[in] isMask If this flag is true then the searchImageFile should 
 * be deemed as a "mask". The default value is false.
 * * \param[in] matchPercent The percentage of pixels in the mainImage and
 * searchImage that must match in order for a region in the mainImage to be
 * deemed a match.
 * * \param[in] tolerance The absolute acceptable difference between each color
 * channel when comparing  
 */
void imageSearch(const string &mainFile, const string &searchFile,
                 const string &outFile, bool isMask,
                 int matchPercent, int tolerance) {
    PNG mainImg, maskImg;
    mainImg.load(mainFile);
    maskImg.load(searchFile);

    int total = maskImg.getWidth() * maskImg.getHeight();
    int requiredMatch = (total * matchPercent) / 100;

    auto raw = findRawMatches(mainImg, maskImg,
                              requiredMatch, tolerance);
    auto matches = filterOverlaps(raw, maskImg);

    drawAllBoxes(mainImg, maskImg, matches);
    printMatchingRegions(matches, maskImg);
    mainImg.write(outFile);
}


/**
 * The main method simply checks for command-line arguments and then calls
 * the image search method in this file.
 * * \param[in] argc The number of command-line arguments. This program
 * needs at least 3 command-line arguments.
 * * \param[in] argv The actual command-line arguments in the following order:
 * 1. The main PNG file in which we will be searching for sub-images
 * 2. The sub-image or mask PNG file to be searched-for
 * 3. The file to which the resulting PNG image is to be written.
 * 4. Optional: Flag (True/False) to indicate if the sub-image is a mask 
 * (deault: false)
 * 5. Optional: Number indicating required percentage of pixels to match
 * (default is 75)
 * 6. Optiona: A tolerance value to be specified (default: 32)
 */
int main(int argc, char *argv[]) {
    if (argc < 4) {
        // Insufficient number of required parameters.
        std::cout << "Usage: " << argv[0] << " <MainPNGfile> <SearchPNGfile> "
                  << "<OutputPNGfile> [isMaskFlag] [match-percentage] "
                  << "[tolerance]\n";
        return 1;
    }
    const std::string True("true");
    // Call the method that starts off the image search with the necessary
    // parameters.

    imageSearch(argv[1], argv[2], argv[3],       // The 3 required PNG files
            (argc > 4 ? (True == argv[4]) : true),  // Optional mask flag
            (argc > 5 ? std::stoi(argv[5]) : 75),   // Optional percentMatch
            (argc > 6 ? std::stoi(argv[6]) : 32));  // Optional tolerance

    return 0;
}

// End of source code
