#pragma once

#include <cmath>
#include <vector>
#include <algorithm>
#include <sstream>
#include "../WDL/denormal.h"
#include "../WDL/fft.h"


/*

IPlug spectrum analyzer FFT example
(c) Matthew Witmer 2015
<http://lvcaudio.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.


Simple IPlug audio effect that shows how to implement a graphical spectrum analyzer.
This file contains an FFT class (Spect_FFT), a graphical display class for the FFT (gFFTAnalyzer), and a class to calculate and display frequency indicators (gFFTFreqDraw)

To Do:
- more window functions for FFT
- dynamically set fft size for Spect_FFT class and gFFTAnalyzer, so it can be changed by the user
- double check all FFT math stuff
- I am sure there are places to optimize
- Better interpolation method than linear
- Add phase display
- better peak display function
- use different draw for a smoother line
- Other cool things that I can't think of ???

*/


static const double pi = 3.141592653589793238462643383279502884197169399375105820974944;
static const double pi2 = 2. * pi;
static const double pi4 = 4. * pi;

// convert from one linear range to another
template <typename T> T RangeConvert(T OldV, T OldMax, T NewMax, T OldMin = (T)0., T NewMin = (T)0.) {
    if (OldMax == OldMin) return (T)0.;
    else return (((OldV - OldMin) * (NewMax - NewMin)) / (OldMax - OldMin)) + NewMin;
}

template <typename T> T LinInterp(T x0, T y0, T x1, T y1, T x)
{
    const T a = (y1 - y0) / (x1 - x0);
    const T b = -a * x0 + y0;
    return a * x + b;
}

// needed for pre-C++ 11 support
template <class T> inline std::string to_string(const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}


class Spect_FFT {
public:
    enum eWindowType
    {
        win_Hann = 0,
        win_BlackmanHarris,
        win_Hamming,
        win_Flattop,
        win_Rectangular,
    };

    Spect_FFT(IPlugBase* pPlug, const int initialsize, const int initialoverlap) {
        fftSize = initialsize;
        overlapSize = initialoverlap;
        SetBufferSize();
        SetOverlapPosition();
        CalculateSValues();
        WDL_fft_init();
        windowType = win_Hann;
    }
    ~Spect_FFT() {}

    void ClearBuffers() {
        SetBufferSize();
    }

    void SetOverlapSize(const int x) {
        overlapSize = x;
        SetBufferSize();
        SetOverlapPosition();
    }

    void SetFFTSize(const int x) {
        fftSize = x;
        SetBufferSize();
        SetOverlapPosition();
        CalculateSValues();
    }

    void SetWindowType(const int type) {
        windowType = type;
        CalculateWindowFx(windowType);
    }

    void SendInput(double in) {
 
        for (std::vector<sFFTBuffer>::iterator it = vFFTBuffer.begin(); it != vFFTBuffer.end(); ++it) {
            double winIn = in * vWindowFx[it->currentPosition];
            it->vReIm[it->currentPosition].re = winIn;
            it->vReIm[it->currentPosition].im = 0.;
            it->currentPosition++;
          if (it->currentPosition >= fftSize) {
                it->currentPosition = 0;
                Permute(it - vFFTBuffer.begin());
            }
        }
    }

    double GetOutput(const int pos) {
        if (pos >=  0 && pos <= fftSize/2+1 ) {
            return vOutPut[pos];
        }
        else {
            // return 0 for first bin and any bin that is larger than expected
            return 0.;
        }
    }

protected:

    void Permute(const int vIndex) {

        WDL_fft(&vFFTBuffer[vIndex].vReIm[0], fftSize, false);

        for (int i = 0; i < fftSize/2+1; ++i)
        {
           int sortIDX = WDL_fft_permute(fftSize, i);
            WDL_FFT_REAL re = vFFTBuffer[vIndex].vReIm[sortIDX].re;
            WDL_FFT_REAL im = vFFTBuffer[vIndex].vReIm[sortIDX].im;
            vOutPut[i] = std::sqrt(2. * (re * re + im * im) / (S1*S1));
        }
        // set first bin to 0
        vOutPut[0] = 0.;
    }

    void SetOverlapPosition() {
        for (int i = 1; i < overlapSize; i++) {
            vFFTBuffer[i].currentPosition = (int)(RangeConvert(1. / overlapSize, 1., (double)fftSize, 0., 0.));
        }
    }

    void SetBufferSize() {
        if (vWindowFx.size() != fftSize) vWindowFx.resize(fftSize);
        CalculateWindowFx();
        if (vFFTBuffer.size() != overlapSize) vFFTBuffer.resize(overlapSize);
        for (std::vector<sFFTBuffer>::iterator it = vFFTBuffer.begin(); it != vFFTBuffer.end(); ++it) {
            if (it->vReIm.size() != fftSize) it->vReIm.resize(fftSize);
            for (int i = 0; i < fftSize; i++) {
                it->vReIm[i].im = 0.0;
                it->vReIm[i].re = 0.0;
            }
            it->currentPosition = 0;
        }
        if (vOutPut.size() != fftSize/2+1 ) vOutPut.resize(fftSize/2+1);
        for (std::vector<double>::iterator it = vOutPut.begin(); it != vOutPut.end(); ++it) {
            (*it) = 0.;
        }
    }

    void CalculateWindowFx(const int type = -1) {
        int wType = type;
        const double M = fftSize - 1.;
        if (wType == -1) wType = windowType;
        for (int i = 0; i < fftSize; i++) {
            if (wType == win_Hann)  vWindowFx[i] = 0.5 * (1. - std::cos(pi2 * i / M)); 
            else if (wType == win_BlackmanHarris)  vWindowFx[i] = 0.35875 - (0.48829 * cos(pi2*i / M)) + (0.14128*cos(pi4*i / M)) - (0.01168 * cos(6.0 * pi * i / M));  
            else if (wType == win_Hamming) vWindowFx[i] = 0.54 - 0.46 * std::cos(pi2 * i / M); 
            else if (wType == win_Flattop) vWindowFx[i] = 0.21557895 - 0.41663158 * cos(pi2 *i / M) + 0.277263158 * cos(pi4 * i / M) - 0.083578947 * cos(6. * pi * i / M) + 0.006947368 * cos(8. * pi * i / M); 
            else vWindowFx[i] = 1.; //rectangular
        }
    }

    void CalculateSValues() {
        S1 = 0.;
        S2 = 0.;
        for (int i = 0; i < fftSize; i++) {
            double v = 0.5 * (1. - std::cos(pi2 * i / (double)(fftSize - 1)));
            S1 += v;
            S2 += v*v;
        }
    }

    struct sFFTBuffer
    {
        int currentPosition;
        std::vector<WDL_FFT_COMPLEX>WDL_FIXALIGN vReIm;
        
    }WDL_FIXALIGN;
    std::vector<double>vWindowFx;
    std::vector<sFFTBuffer>vFFTBuffer;
    std::vector<double>vOutPut;
    int fftSize, overlapSize, windowType;
    double S1, S2;

};

class gFFTAnalyzer : public IControl
    {
    public:
        gFFTAnalyzer(IPlugBase* pPlug, IRECT pR, IColor c, int par, int sz, bool l)
            : IControl(pPlug, pR), mColor(c), mParam(par), line(l)
        {
            mColor2 = IColor(100, mColor.R, mColor.G, mColor.B);
            fftBins = (double)sz; //total number of fft bins as double
            i = 0;
            dBFloor = -120.;
            ampFloor = DBToAmp(dBFloor);
            sCount = 0;
            val = 0.0;
            minFreq = 1.;
            maxFreq = 44100. / 0.5;
            sampleRate = 44100.;
            width = static_cast<int>(mRECT.W());
            value.resize(sz / 2 + 1 );
            iVal.resize(width);
            iPeak.resize(width);
            OctaveGain = 1.;
            ResetValuestoFloor();
            decayValue = 0.70;
            peakdecayValue = 0.95;
            }

        ~gFFTAnalyzer()
            {
            }
        void SetdbFloor(const double f) { 
            ampFloor = DBToAmp(f);
            dBFloor = f;
            ResetValuestoFloor();
        }

        void SetMinFreq(const double f) { 
            minFreq = BOUNDED(f, 1, maxFreq);
            ResetValuestoFloor();
         }

        void SetMaxFreq(const double f) {
            maxFreq = BOUNDED(f, minFreq, mPlug->GetSampleRate() * 0.5);
            ResetValuestoFloor();
        }

        // per-octave gain (e.g., +3 dB makes pink noise appear flat).  Most analyzers use between +3 and +4.5 dB/octave compensation
        void SetOctaveGain(const double g, const bool isDB) {
            if (isDB) OctaveGain = DBToAmp(g);
            else OctaveGain = g;
        }

        void SendFFT(double v, int c, double sr)
            {
            value[c] = v;
            sampleRate = sr;
            }

        bool Draw(IGraphics* pGraphics)
            {
            double x, y, yPeak;
            double xPrev = mRECT.L;
            double yPrev = mRECT.B;
            double yPrevPeak = yPrev;
            int startBin = 1;
            const double mF = maxFreq / minFreq;
            for (int f = 0; f < width; f++) {
                const double FreqForBin = minFreq * std::pow(mF, (double)f / (double)(width-1));
                bool isSearch = false;
                while (!isSearch)
                {
                    const double b1 = BOUNDED((double)(startBin - 1) * sampleRate / fftBins, 0., sampleRate) ;
                    const double b2 = BOUNDED((double)startBin * sampleRate / fftBins, 0., sampleRate);

                    if(b1 <= FreqForBin && b2 >= FreqForBin ) {
                        const double interpV = LinInterp(b1, value[startBin - 1], b2, value[startBin], FreqForBin);
                        iVal[f] = std::max(interpV, iVal[f] * decayValue);
                        iPeak[f] = std::max(interpV, iPeak[f] * peakdecayValue);
                        startBin = std::max(startBin-2, 1);
                        isSearch = true;
                    }
                    startBin++;
                    if (startBin >= value.size()) isSearch = true;
                }
            }

            for (int b = 0; b < width; b++)
            {
                x = b + mRECT.L;
                const double binFreq = minFreq * std::pow(mF, (double)b / (double)(width));
                const double oct = std::log10(binFreq / minFreq) / 0.30102999;
                const double gainO = std::pow(OctaveGain, oct);
                
                double dbv = AmpToDB(iVal[b] * gainO);
                y = RangeConvert(BOUNDED(dbv, dBFloor, 0.), 0., (double)mRECT.T, dBFloor, (double)mRECT.B);
                double pdv = AmpToDB(iPeak[b] * gainO);
                yPeak = RangeConvert(BOUNDED(pdv, dBFloor, 0.), 0., (double)mRECT.T, dBFloor, (double)mRECT.B);

                if (!line) pGraphics->DrawVerticalLine(&mColor2, x, mRECT.B, y);

                if (b == 0) yPeak = yPrevPeak = y;

                pGraphics->DrawLine(&mColor, xPrev, yPrevPeak, x, yPeak);
                    xPrev = x;
                    yPrev = y;
                    yPrevPeak = yPeak;
                }
            return true;
            }

        void SetColors(IColor fill, IColor peakLine) {
            mColor = peakLine;
            mColor2 = fill;
        }

        bool IsDirty() { return true;}

        void ResetValuestoFloor() {
            const double ampF = 0.;
            for (int f = 0; f < width; f++) {
                iVal[f] = ampF;
                iPeak[f] = ampF;
            }
            for (std::vector<double>::iterator it = value.begin(); it != value.end(); ++it)
            {
                *it = ampF;
            }
        }

    private:
        int fftWidth, sCount, mParam, mScaleP;
        std::vector <double> value;
        std::vector<double>iVal;
        std::vector<double>iPeak;
        double val, fftBins, sampleRate;
        int i, width;
        double decayValue, peakdecayValue;
        IColor mColor, mColor2;
        bool line;
        double minFreq, maxFreq;
        double dBFloor, ampFloor;
        double OctaveGain;
    };

    class gFFTFreqDraw : public IControl {
    public:
        gFFTFreqDraw(IPlugBase* pPlug, IRECT pR, IColor c, IText *fonttxt)
        : IControl (pPlug, pR), mColor(c) {
            txt = *fonttxt;
            txt.mAlign = IText::kAlignCenter;
            minFreq = 20.;
            maxFreq = 20000.;
            space = txt.mSize + 4;
            CalcSpacing();
        }
        ~gFFTFreqDraw() {}

        void SetMinFreq(const double f) { 
            minFreq = BOUNDED(f, 1, maxFreq);
            CalcSpacing();
        }

        void SetMaxFreq(const double f) { 
            maxFreq = BOUNDED(f, minFreq, mPlug->GetSampleRate() * 0.5); 
            CalcSpacing();
        }
  
        bool Draw(IGraphics* pGraphics)
        {
            for (int i = 0; i < HLineDraw.size() ; i++) {
                pGraphics->DrawVerticalLine(&mColor, HLineDraw[i], mRECT.B - 20, mRECT.T);
            }

            for (int i = 0; i < FreqTxtDraw.size() ; i++) {
                char hz[7];
                std::strcpy(hz, FreqTxtDraw[i].freq.c_str());
                IRECT box(FreqTxtDraw[i].pos - (space / 2), mRECT.B - 15, FreqTxtDraw[i].pos + (space / 2), mRECT.B);
                pGraphics->DrawIText(&txt, hz, &box);
            }
            return true;
        }

    private:
        void CalcSpacing() {
            int prevTxt = space / 2;
            FreqTxtDraw.resize(0);
            HLineDraw.resize(0);
            bool addToList = false;
            txtLocation toAdd;
            std::ostringstream s;
            int minFreq10 = minFreq;
            if (minFreq10 % 10 != 0) minFreq10 = (10 - (int)(minFreq) % 10) + (int)(minFreq);
            for (int i = minFreq10; i < maxFreq; i += 10) {
                const int bin = (int)( (std::log10((double)i / (double)minFreq) / (std::log10((double)maxFreq / (double)minFreq))) * (double)(mRECT.W() )  );
                if (i < 80 && i % 10 == 0) {
                    if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
                        addToList = true;
                        s << i;
                    }
                }
                else if (i >= 80 && i < 799 && i % 100 == 0) {
                    if (bin > prevTxt + space && bin < mRECT.W() - space / 2){
                        addToList = true;
                        s << i;
                    }
                }
                else if (i >= 800 && i < 14999 && i % 1000 == 0) {
                    if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
                        addToList = true;
                        s << (int)(i * 0.001);
                        s << 'k';
                    }
                }
                else if (i % 5000 == 0) {
                    if (bin > prevTxt + space && bin < mRECT.W() - space / 2) {
                        addToList = true;
                        s << (int)(i * 0.001);
                        s << 'k';
                    }
                }

                if (addToList) {
                    addToList = false;
                    toAdd.pos = bin + mRECT.L;
                    const std::string freq_as_string(s.str());
                    toAdd.freq = freq_as_string;
                    FreqTxtDraw.push_back(toAdd);
                    HLineDraw.push_back(toAdd.pos);
                    s.clear();
                    s.str("");
                    prevTxt = bin;
                }
            }
        }

        struct txtLocation
        {
            int pos;
            std::string WDL_FIXALIGN freq;
        }WDL_FIXALIGN;

        std::vector<int>HLineDraw;
        std::vector<txtLocation>FreqTxtDraw;
        IColor mColor;
        IText txt;
        int space;
        double minFreq, maxFreq;
    };

