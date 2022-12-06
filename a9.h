#ifndef A10_H_PHUDVTKB
#define A10_H_PHUDVTKB

#include "Image.h"

using namespace std;

// Write your declarations here, or extend the Makefile if you add source
// files
Image transpose(const Image& im);
Image normalize(const Image& im);
Image computeEnergyFunc(const Image& im);
Image computeCumulMinEnergy(const Image& im, bool vertical);
vector<int> findOptSeam(const Image& M, bool vertical);
Image carveSeam(const Image& im, const vector<int> s, bool vertical);
Image carveSeamsShrink(const Image& im, int newW, int newH); 
void reindexSeams(vector<vector<int>>& seams, int W, int H, bool vertical);
Image carveSeamsEnlarge(const Image& im, int newW, int newH);

#endif /* end of include guard: A10_H_PHUDVTKB */

