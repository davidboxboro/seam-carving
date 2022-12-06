#include <iostream>
#include <cstdlib>
#include <algorithm>

#include "a9.h"
#include "filtering.h"

using namespace std;

Image normalize(const Image& im) {
    const float maxPix = im.max();
    cout << "maxPix=" << maxPix << '\n';
    return im / maxPix;
}

Image transpose(const Image& im) {
    const int W = im.width(), H = im.height(), C = im.channels();
    Image out(H, W, C);
    for (int x = 0; x < W; x++) {
        for (int y = 0; y < H; y++) {
            for (int z = 0; z < C; z++) {
                out(y, x, z) = im(x, y, z);
            }
        }
    }
    return out;
}

Image computeEnergyFunc(const Image& im) {
    const Image gX = gradientX(im);
    const Image gY = gradientY(im);
    const int W = im.width(), H = im.height(), C = im.channels();
    Image e(W, H, 1);
    for (int x = 0; x < W; x++) {
        for (int y = 0; y < H; y++) {
            float sum = 0;
            for (int z = 0; z < C; z++) {
                sum += abs(gX(x, y, z));
                sum += abs(gY(x, y, z));
            }
            e(x, y) = sum;
        }
    }
    return e;
}

Image computeCumulMinEnergy(const Image& e, bool vertical) {
    // for vertical
    if (vertical) {
        const Image eT = transpose(e);
        const Image MT = computeCumulMinEnergy(eT, false);
        const Image M = transpose(MT);
        return M;
    }
    // for horizontal
    const int W = e.width(), H = e.height();
    Image M(W, H, 1);
    for (int x = 0; x < W; x++) {
        for (int y = 0; y < H; y++) {
            M(x, y) = e(x, y);
            if (x == 0) {
                continue;
            }
            float prevMin = FLT_MAX;
            const int yCandMin = max(y - 1, 0);
            const int yCandMax = min(y + 1, H - 1);
            for (int yCand = yCandMin; yCand <= yCandMax; yCand++) {
                if (prevMin > M(x - 1, yCand)) {
                    prevMin = M(x - 1, yCand);
                }
            }
            M(x, y) += prevMin;
        }
    } 
    return M;
}

vector<int> findOptSeam(const Image& M, bool vertical) {
    if (vertical) {
        // if M is vertical, then we transpose M so that 
        // findOptSeam() operates on M top-to-bottom instead
        // of left-to-right
        const Image MT = transpose(M);
        return findOptSeam(MT, false);
    }
    const int W = M.width(), H = M.height();
    vector<int> s;
    // get start position
    int yEnd;
    float minEnergy = FLT_MAX;
    for (int y = 0; y < H; y++) {
        if (minEnergy > M(W - 1, y)) {
            minEnergy = M(W - 1, y);
            yEnd = y;
        } 
    }
    s.push_back(yEnd);
    // find rest of path
    int y = yEnd;
    for (int x = W - 2; x >= 0; x--) {
        float prevMin = FLT_MAX;
        const int yCandMin = max(y - 1, 0);
        const int yCandMax = min(y + 1, H - 1);
        for (int yCand = yCandMin; yCand <= yCandMax; yCand++) {
            if (prevMin > M(x, yCand)) {
                prevMin = M(x, yCand);
                y = yCand;
            }
        }
        // add position to vector
        s.push_back(y);
    }
    // order seam from left to right
    reverse(s.begin(), s.end());
    return s;
}

Image carveSeam(const Image& im, const vector<int> s, bool vertical) {
    if (vertical) {
        const Image imT = transpose(im);
        const Image outT = carveSeam(imT, s, false);
        return transpose(outT);
    }
    // horizontal seam removal
    const int W = im.width(), H = im.height(), C = im.channels();
    // height decreaseses by 1
    Image out(W, H - 1, C);
    for (int x = 0; x < W; x++) {
        int yOut = 0;
        for (int y = 0; y < H; y++) {
            if (y == s[x]) {
                continue;
            }
            for (int z = 0; z < C; z++) {
                out(x, yOut, z) = im(x, y, z);
            }
            yOut++;
        } 
    }
    return out;
}

Image dupSeam(const Image& im, const vector<int> s, bool vertical) {
    if (vertical) {
        const Image imT = transpose(im);
        const Image outT = dupSeam(imT, s, false);
        return transpose(outT);
    }
    // horizontal seam duplication 
    const int W = im.width(), H = im.height(), C = im.channels();
    // height increaseses by 1
    Image out(W, H + 1, C);
    for (int x = 0; x < W; x++) {
        int yOut = 0;
        for (int y = 0; y < H; y++) {
            for (int z = 0; z < C; z++) {
                out(x, yOut, z) = im(x, y, z);
            }
            yOut++;
            // duplicate pixel
            if (y == s[x]) {
                for (int z = 0; z < C; z++) {
                    out(x, yOut, z) = im(x, y, z);
                }
                yOut++;
            }
        } 
    }
    return out;
}

Image carveSeamsShrink(const Image& im, int newW, int newH) {
    /*
     * Carve seams in image, reducing size to newW x newH.
     */
    const int W = im.width(), H = im.height();
    Image out = im;
    const int nVertSeam = W - newW;
    const int nHorSeam = H - newH;
    const vector<int> nSeams = {nVertSeam, nHorSeam};
    const vector<int> verts = {true, false};
    // keep track of progress 
    int totalSeams = nVertSeam + nHorSeam;
    int finishedSeams = 0;
    // carve vertical seams first 
    for (int i = 0; i < 2; i++) {
        const int nSeam = nSeams[i];
        bool vert = verts[i];
        for (int j = 0; j < nSeam; j++) {
            const Image e = computeEnergyFunc(out);
            Image M = computeCumulMinEnergy(e, vert);
            const vector<int> s = findOptSeam(M, vert);
            out = carveSeam(out, s, vert);
            // print number of finished seams 
            finishedSeams++;
            cout << "Finished carving " << finishedSeams << "/" << totalSeams << " seams" << flush << '\r';
        } 
    }
    cout << endl;
    return out;
}

void reindexSeams(vector<vector<int>>& seams, int W, int H, bool vertical) {
    if (vertical) {
        return reindexSeams(seams, H, W, false);
    }
    // assume seams are horizontal
    const int S = seams.size();
    for (int i = 0; i < S; i++) {
        vector<int>& s = seams[i];
        for (int j = i + 1; j < S; j++) {
            vector<int>& s2 = seams[j]; 
            // adjust s2 indices upward
            for (int x = 0; x < W; x++) {
                if (s[x] <= s2[x]) {
                    s2[x]++;
                } 
            }
        } 
        cout << "Finished reindexing " << i + 1 << "/" << S << " seams" << flush << '\r';
    }
    cout << endl;
}

Image carveSeamsEnlarge(const Image& im, int newW, int newH) {
    const int W = im.width(), H = im.height();
    const int nVertSeam = newW - W;
    const int nHorSeam = newH - H;
    const vector<int> nSeams = {nVertSeam, nHorSeam};
    const vector<int>verts = {true, false};
    // carve vertical seams first 
    Image out = im;
    for (int i = 0; i < 2; i++) {
        const int nSeam = nSeams[i];
        bool vert = verts[i];
        // first find the lowest energy nSeam seams
        const int interW = out.width(), interH = out.height();
        Image tmp = out; // only used to find which seams to duplicate
        vector<vector<int>> seams;
        for (int j = 0; j < nSeam; j++) {
            const Image e = computeEnergyFunc(tmp);
            Image M = computeCumulMinEnergy(e, vert);
            const vector<int> s = findOptSeam(M, vert);
            tmp = carveSeam(tmp, s, vert);
            seams.push_back(std::move(s));
            cout << "Prepare duplicating " << j + 1 << "/" << nSeam << " seams" << flush << '\r';
        }
        cout << endl;
        // reindex the seams to fit on the original image
        reindexSeams(seams, interW, interH, vert); 
        // add pixels
        const int sLen = vert ? interH : interW;
        const int S = seams.size();
        for (int i = 0; i < S; i++) {
            vector<int>& s = seams[i];
            // fix all subsequent seams
            for (int j = i + 1; j < S; j++) {
                vector<int>& s2 = seams[j];
                for (int x = 0; x < sLen; x++) {
                    if (s[x] <= s2[x]) {
                        s2[x]++; 
                    }
                } 
            }
            // draw the seam 
            out = dupSeam(out, s, vert);
            cout << "Finished duplicating " << i + 1 << "/" << S << " seams" << flush << '\r';
        }
        cout << endl;
    }
    cout << "Finished enlarging" << endl;
    return out;
}

