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
    return (std::abs(pix1.color.red - pix2.color.red) < tolerance)
        && (std::abs(pix1.color.green - pix2.color.green) < tolerance)
        && (std::abs(pix1.color.blue - pix2.color.blue) < tolerance);
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
    for (int i = 0; (i < width); i++) {
        png.setRed(row, col + i); 
        png.setRed(row + height - 1, col + i);
    }
    // Draw vertical lines
    for (int i = 0; (i < height); i++) { 
        png.setRed(row + i, col); 
        png.setRed(row + i, col + width - 1);
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
 * This method checks if the specified match location overlaps with any of the
 * specified match locations.
 * * \param[in] matchLocations The vector of match locations.
 * \param[in] match The match location to be checked.
 * \param[in] mask The mask image that is used to determine which pixels
 * in the main image are to be considered.
 * * \return true if the match location overlaps with any of the specified match
 * locations, false otherwise.
*/
bool hasOverlap(const vector<pair<int, int>> &matchLocations,
                const pair<int, int> &match, const PNG &mask) {
    int maskHeight = mask.getHeight();
    int maskWidth = mask.getWidth();

    int candidateRow = match.first;
    int candidateCol = match.second;
    int candidateEndRow = candidateRow + maskHeight;
    int candidateEndCol = candidateCol + maskWidth;

    for (const auto &loc : matchLocations) {
        int existingRowStart = loc.first;
        int existingColStart = loc.second;
        int existingRowEnd = existingRowStart + maskHeight;
        int existingColEnd = existingColStart + maskWidth;

        // Check for intersection
        if (candidateCol < existingColEnd &&
            candidateEndCol > existingColStart &&
            candidateRow < existingRowEnd &&
            candidateEndRow > existingRowStart) {
            return true;
        }
    }

    return false;
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
        std::cout << "sub-image matched at:" 
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
void processPotentialMatch(PNG& mainImage, const PNG& maskImage,
                           vector<pair<int, int>>& matchLocations,
                           int requiredMatch, int tolerance,
                           int row, int col) {
    int maskTotalPixels = maskImage.getWidth() * maskImage.getHeight();
    int matchPixCount = countMatchingPixels(mainImage, maskImage,
                                            row, col, tolerance);
    int mismatchPixCount = maskTotalPixels - matchPixCount;
    int netMatch = matchPixCount - mismatchPixCount;

    if (netMatch > requiredMatch) {
        const auto match = std::make_pair(row, col);
        if (!hasOverlap(matchLocations, match, maskImage)) {
            matchLocations.push_back(match);
            drawBox(mainImage, row, col,
                    maskImage.getWidth(), maskImage.getHeight());
        }
    }
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
void imageSearch(const std::string &mainImageFile,
                 const std::string &searchImageFile,
                 const std::string &outImageFile, const bool isMask = true,
                 const int matchPercent = 75, const int tolerance = 32) {
    PNG mainImage, maskImage;
    mainImage.load(mainImageFile);
    maskImage.load(searchImageFile);

    vector<pair<int, int>> matchLocations;
    int maskTotalPixels = maskImage.getWidth() * maskImage.getHeight();
    int requiredMatch = (maskTotalPixels * matchPercent) / 100;

    for (int row = 0; row <= mainImage.getHeight() - maskImage.getHeight();
         row++) {
        for (int col = 0; col <= mainImage.getWidth() - maskImage.getWidth();
             col++) {
            processPotentialMatch(mainImage, maskImage, matchLocations,
                                  requiredMatch, tolerance, row, col);
        }
    }

    printMatchingRegions(matchLocations, maskImage);
    mainImage.write(outImageFile);
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
