#include <iostream>
#include <cassert>

#include "a9.h"
#include "basicImageManipulation.h"

using namespace std;

void testComputeEnergyFunc() {
    /*
     * Test computerEnergyFunc() by applying it to sample
     * image. Ensure that the resulting image contains bright 
     * regions that correspond to edges in the original image 
     * (i.e., areas where the gradient magnitudes are larger).
     */
    Image im("Input/boston.png");
    im = scaleLin(im, 0.5);
    Image e = computeEnergyFunc(im);
    e = normalize(e);
    e.write("Output/energyFunc.png");
}

void testTranspose() {
    /*
     * Test that an image is successfully transposed,
     * meaning it's width and height are interchanged. 
     */
    const Image im("Input/boston.png");
    const Image transposed = transpose(im);
    transposed.write("Output/transposed.png");
}

void testComputeCumulMinEnergy() {
    /*
     * Test computeCumulMinEnergy() by applying the function 
     * to a sample image. For the vertical image, ensure that 
     * the resulting image generally becomes bright as we move 
     * from the top of the image to the bottom
     * (which implies that the energies are being propogated) and 
     * that the brightness increases suddenly when we get to the buildings
     * (the most edge-heavy parts of the image).
     * For the horizontal image, test the same pattern except
     * ensure that the brightness increases left to right.
     */
    Image im("Input/boston.png");
    im = scaleLin(im, 0.5);
    const Image e = computeEnergyFunc(im);
    Image MHor = computeCumulMinEnergy(e, false);
    Image MVert = computeCumulMinEnergy(e, true);
    // normalize first so pixels lie between 0 and 1
    MHor = normalize(MHor);
    MVert = normalize(MVert); 
    MHor.write("Output/MHor.png");
    MVert.write("Output/MVert.png");
}

void testFindOptimalSeam() {
    /*
     * Test findOptimalSeam() on a sample image. 
     * Overlay the resulting seam on the image to ensure
     * that it corresponds to low-energy parts of the image.
     * Ensure that the optimal vertical seam has length
     * equal to the height of the image, and that the optimal
     * horizontal seam has length equal to the width. Also 
     * ensure that adjacent seam elements are no more than one 
     * pixel away from each other.
     */
    Image im("Input/boston.png");
    im = scaleLin(im, 0.5);
    const Image e = computeEnergyFunc(im);
    Image MHor = computeCumulMinEnergy(e, false);
    Image MVert = computeCumulMinEnergy(e, true);
    // normalize first so pixels lie between 0 and 1
    MHor = normalize(MHor);
    MVert = normalize(MVert); 
    // get seams
    const vector<int> sHor = findOptSeam(MHor, false);
    const vector<int> sVert = findOptSeam(MVert, true);
    // check size
    const int H = im.height(), W = im.width(), C = im.channels();
    assert(sHor.size() == W);
    assert(sVert.size() == H);
    // display seams
    Image imHor = im;
    Image imVert = im;
    const vector<float> seamColor = {1, 0, 0};
    for (int x = 0; x < W; x++) {
        MHor(x, sHor[x], 0) = 1;
        for (int z = 0; z < C; z++) {
            imHor(x, sHor[x], z) = seamColor[z];
            im(x, sHor[x], z) = seamColor[z];
        }
    }
    for (int y = 0; y < H; y++) {
        MVert(sVert[y], y, 0) = 1; 
        for (int z = 0; z < C; z++) {
            imVert(sVert[y], y, z) = seamColor[z];
            im(sVert[y], y, z) = seamColor[z];
        }
    }
   MHor.write("Output/sHorM.png");
   imHor.write("Output/sHor.png");
   MVert.write("Output/sVertM.png");
   imVert.write("Output/sVert.png");
   im.write("Output/sHorVert.png");
}

void testCarveSeamsShrink() {
    /*
     * Test seam carving on a sample image. Ensure 
     * that the new image is of the desired size and 
     * that the image has been successfully shrunk while
     * retaining its most salient features. Test for width
     * and height shrinkages separately and jointly.
     */
    vector<string> names = {"birds"};
    for (const auto& name: names) {
        Image im("Input/" + name + ".png");
        im = scaleLin(im, 0.3);
        const int W = im.width(), H = im.height();
        const Image shrinkH = carveSeamsShrink(im, W, H / 2);
        assert(shrinkH.width() == W && shrinkH.height() == H / 2);
        const Image shrinkW = carveSeamsShrink(im, W / 2, H);
        assert(shrinkW.width() == W / 2 && shrinkW.height() == H);
        const Image shrinkHW = carveSeamsShrink(im, W / 2, H / 2);
        assert(shrinkHW.width() == W / 2 && shrinkHW.height() == H / 2);
        im.write("Output/noShrink_" + name + ".png");
        shrinkH.write("Output/shrinkH_" + name + ".png");
        shrinkW.write("Output/shrinkW_" + name + ".png");
        shrinkHW.write("Output/shrinkHW_" + name + ".png");
    }
}

void testReindexSeams() {
   /*
    * Print drawn seams to ensure that seams are reindexed correctly.
    * They should not overlap after reindexing.
    */
    Image im("Input/boston.png");
    im = scaleLin(im, 0.5); 
    const int W = im.width(), H = im.height(), C = im.channels();
    vector<vector<int>> seams;
    Image carved = im;
    for (int i = 0; i < W / 2; i++) {
        const Image e = computeEnergyFunc(carved);
        Image M = computeCumulMinEnergy(e, true);
        const vector<int> s = findOptSeam(M, true);
        carved = carveSeam(carved, s, true);
        seams.push_back(std::move(s));
        cout << "Finished creating " << i + 1 << "/" << W / 2 << " seams" << flush << '\r';
    } 
    reindexSeams(seams, W, H, true); 
    // draw reindexed seams
    const vector<float> seamColor = {1, 0, 0};
    Image out = im;
    for (const auto& s: seams) {
        for (int y = 0; y < H; y++) {
            for (int z = 0; z < C; z++) {
                out(s[y], y, z) = seamColor[z];
            }
        }
    }
    out.write("Output/visSeams.png");
}

void testCarveSeamsEnlarge() {
    /*
     * Test seam carving enlarging on a sample image. Ensure 
     * that the new image is of the desired size and 
     * that the image has been successfully enlarged while
     * retaining its most salient features, while having the
     * "least important" parts enlarged. Test enlarging 
     * height and width, separately and jointly.
     */
    vector<string> names = {"boston"};
    for (const auto& name: names) {
        Image im("Input/" + name + ".png");
        im = scaleLin(im, 0.5);
        const int W = im.width(), H = im.height();
        const Image enlargeH = carveSeamsEnlarge(im, W, H * 1.5);
        assert(enlargeH.width() == W && enlargeH.height() == H * 1.5);
        const Image enlargeW = carveSeamsEnlarge(im, W * 1.5, H);
        assert(enlargeW.width() == W * 1.5 && enlargeW.height() == H); 
        const Image enlargeHW = carveSeamsEnlarge(im, W * 1.5, H * 1.5); 
        assert(enlargeHW.width() == W * 1.5 && enlargeHW.height() == H * 1.5);
        im.write("Output/noEnlarge_" + name + ".png");
        enlargeH.write("Output/enlargeH_" + name + ".png");
        enlargeW.write("Output/enlargeW_" + name + ".png");
        enlargeHW.write("Output/enlargeHW_" + name + ".png");
    }
}

int main()
{
    testComputeEnergyFunc();
    testTranspose();
    testComputeCumulMinEnergy();
    testFindOptimalSeam();
//    testCarveSeamsShrink();
//    testReindexSeams();
//    testCarveSeamsEnlarge();
}
