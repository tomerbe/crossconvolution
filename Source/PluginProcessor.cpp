/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FftshellAudioProcessor::FftshellAudioProcessor()
    :   parameters (*this, nullptr)
#ifndef JucePlugin_PreferredChannelConfigurations
     , AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(
                                                                           "one",       // parameter ID
                                                                           "one",       // parameter name
                                                                           NormalisableRange<float> (-24.0f, 24.0f),    // range
                                                                           0.0f,         // default value
                                                                           "dB"));
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(
                                                                           "two",       // parameter ID
                                                                           "two",       // parameter name
                                                                           NormalisableRange<float> (-24.0f, 24.0f),    // range
                                                                           0.0f,         // default value
                                                                           "dB"));
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(
                                                                           "three",       // parameter ID
                                                                           "three",       // parameter name
                                                                           NormalisableRange<float> (-24.0f, 24.0f),    // range
                                                                           0.0f,         // default value
                                                                           "dB"));
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(
                                                                           "four",       // parameter ID
                                                                           "four",       // parameter name
                                                                           NormalisableRange<float> (-24.0f, 24.0f),    // range
                                                                           0.0f,         // default value
                                                                           "dB"));
    parameters.createAndAddParameter(std::make_unique<AudioParameterFloat>(
                                                                           "five",       // parameter ID
                                                                           "five",       // parameter name
                                                                           NormalisableRange<float> (-24.0f, 24.0f),    // range
                                                                           0.0f,         // default value
                                                                           "dB"));
    parameters.createAndAddParameter(std::make_unique<AudioParameterBool>(
                                                                           "convolve",       // parameter ID
                                                                           "convolve",       // parameter name
                                                                           0.0f         // default value
                                                                           ));
    parameters.state = ValueTree (Identifier ("FFTShellVT"));

    pi = 4.0f * atanf(1.0f);
    twoPi = 8.0f * atanf(1.0f);
    
    for(int i = 0; i < 12; i++)
    {
        voices[i].active = 0;
        voices[i].midinote = 0;
        voices[i].phaseincrement = 0.009977324263039;
        voices[i].phase = 0.0;
        voices[i].gain = 0.0;
        voices[i].gaininc = 0.0;
    }

    wavesize = 8192;
    waveform = 0;
    waveform = (float *)realloc(waveform, sizeof(float) * wavesize);
    for(int i = 0; i < wavesize; i++)
        *(waveform+i) = sinf((twoPi * i)/wavesize);

    initspect(2);
    setFFTSize(1024);
    
    for(int i = kHalfSizeFFT; i > 0; i--)
    {
        bandGain[i] = log2((float)i) * 4.0/9.0;
    }
    bandGain[0] = bandGain[1];
}

FftshellAudioProcessor::~FftshellAudioProcessor()
{
    if(waveform != 0) {free(waveform); waveform = 0;}
    termspect();
}

void FftshellAudioProcessor::initspect(int32_t pNumInputs)
{
    numOutputs = 2;
    numInputs = pNumInputs;
    
    // zero  buffer pointers
    outBufferL = outBufferR = inBufferL = inBufferR = 0;
    inFFTL = inFFTR = inShiftL = inShiftR = outShiftL = outShiftR = 0;
    inSpectraL = inSpectraR = outSpectraL = outSpectraR = 0;
    synthesisWindow = analysisWindow = 0;
    
    // clear parameters
    bufferPosition = 0;
    inputTimeL = outputTimeL = 0;
    inputTimeR = outputTimeR = 0;
    
    // initialize fft size stuff
    overLapFFT = 2;
    sizeFFT = kSizeFFT;
    blockSizeFFT = sizeFFT >> overLapFFT;
    halfSizeFFT = sizeFFT >> 1;
    oneOverBlockSize = 1.0f / (float)blockSizeFFT;
    synthAccL = (float *)malloc(sizeof(float) * 512);
    synthAccR = (float *)malloc(sizeof(float) * 512);

    allocateMemory();
    clearMemory();
    
    // fftw setup
#if FFT_USED_FFTW
    planIn = rfftw_create_plan(sizeFFT, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
    planOut = rfftw_create_plan(sizeFFT, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
#elif FFT_USED_APPLEVECLIB
    log2n = (int32_t)log2f((float)kMaxSizeFFT);
    setupReal = vDSP_create_fftsetup(log2n, FFT_RADIX2);
    log2n = (int32_t)log2f((float)sizeFFT);
#elif FFT_USED_INTEL
    int points, i;
    IppStatus status;
    ippInit();
    for (i = 0, points = 4; i < 11; i++, points++)
    {
        int sizeFFTSpec, sizeFFTInitBuf, sizeFFTWorkBuf;
        status = ippsFFTGetSize_R_32f(points, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeFFTSpec, &sizeFFTInitBuf, &sizeFFTWorkBuf);
        
        // Alloc FFT buffers
        mFFTSpecBuf[i] = ippsMalloc_8u(sizeFFTSpec);
        mFFTInitBuf = ippsMalloc_8u(sizeFFTInitBuf);
        mFFTWorkBuf[i] = ippsMalloc_8u(sizeFFTWorkBuf);
        
        status = ippsFFTInit_R_32f(&(fftSpec[i]), points, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, mFFTSpecBuf[i], mFFTInitBuf);
        if (mFFTInitBuf) ippFree(mFFTInitBuf);
    }
#endif
    
    // make the windows
    initHammingWindows();
    scaleWindows();
    if(numInputs == 2)
        channelMode = kStereoMode;
    else
        channelMode = kMonoMode;
}

//-----------------------------------------------------------------------------------------
void FftshellAudioProcessor::termspect()
{
    freeMemory();
#if FFT_USED_FFTW
    rfftw_destroy_plan(planIn);
    rfftw_destroy_plan(planOut);
#elif FFT_USED_APPLEVECLIB
    vDSP_destroy_fftsetup(setupReal);
#elif FFT_USED_INTEL
    for (int i = 0; i < 11; i++)
    {
        if (mFFTWorkBuf[i]) ippFree(mFFTWorkBuf[i]);
        if (mFFTSpecBuf[i]) ippFree(mFFTSpecBuf[i]);
    }
#endif
}

bool FftshellAudioProcessor::allocateMemory()
{
    
#if FFT_USED_APPLEVECLIB
    split.realp = 0;
    split.realp = (float *) malloc(kHalfMaxSizeFFT*sizeof(float));
    split.imagp = 0;
    split.imagp = (float *) malloc(kHalfMaxSizeFFT*sizeof(float));
#endif
    inBufferL = 0;
    inBufferR = 0;
    inBufferL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inBufferR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inFFTL = 0;
    inFFTR = 0;
    inFFTL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inFFTR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inShiftL = 0;
    inShiftR = 0;
    inShiftL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inShiftR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inSpectraL = 0;
    inSpectraR = 0;
    inSpectraL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    inSpectraR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    
    outSpectraL = 0;
    outSpectraL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    outSpectraR = 0;
    outSpectraR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    outShiftL = 0;
    outShiftL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    outShiftR = 0;
    outShiftR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    outBufferL = 0;
    outBufferL = (float *) malloc(kMaxSizeFFT*sizeof(float));
    outBufferR = 0;
    outBufferR = (float *) malloc(kMaxSizeFFT*sizeof(float));
    synthesisWindow = 0;
    synthesisWindow = (float *) malloc(kMaxSizeFFT*sizeof(float));
    analysisWindow = 0;
    analysisWindow = (float *) malloc(kMaxSizeFFT*sizeof(float));
    
    for(int i = 0; i<kMaxSizeFFT; i++)
    {
        inBufferL[i] = inBufferR[i] = outBufferL[i] = outBufferR[i] = 0.0f;
        inShiftL[i] = inShiftR[i] = outShiftL[i] = outShiftR[i] = 0.0f;
        inSpectraL[i] = inSpectraR[i] = 0.0f;
        outSpectraL[i] = outSpectraR[i] = 0.0f;
    }
    return(true);
}

void FftshellAudioProcessor::freeMemory()
{
    if(synthAccL != 0) {free(synthAccL); synthAccL = 0; }
    if(synthAccR != 0) {free(synthAccR); synthAccR = 0; }

    
    if(inBufferL != 0) {free(inBufferL); inBufferL = 0; }
    if(inBufferR != 0 ) {free(inBufferR); inBufferR = 0;}
    if(inFFTL != 0 ) {free(inFFTL); inFFTL = 0;}
    if(inFFTR != 0 ) {free(inFFTR); inFFTR = 0;}
    if(outBufferL != 0) {free(outBufferL); outBufferL = 0;}
    if(outBufferR != 0 ) {free(outBufferR); outBufferR = 0;}
    if(inShiftL != 0 ) {free(inShiftL); inShiftL = 0;}
    if(inShiftR != 0 ) {free(inShiftR);inShiftR = 0;}
    if(outShiftL != 0 ) {free(outShiftL);outShiftL = 0;}
    if(outShiftR != 0 ) {free(outShiftR);outShiftR = 0;}
    if(inSpectraL != 0 ) {free(inSpectraL);inSpectraL = 0;}
    if(inSpectraR != 0 ) {free(inSpectraR);inSpectraR = 0;}
    if(outSpectraL != 0 ) {free(outSpectraL);outSpectraL = 0;}
    if(outSpectraR != 0 ) {free(outSpectraR);outSpectraR = 0;}
    if(analysisWindow != 0) {free(analysisWindow); analysisWindow = 0;}
    if(synthesisWindow != 0) {free(synthesisWindow); synthesisWindow = 0;}
#if FFT_USED_APPLEVECLIB
    if(split.realp != 0) {free(split.realp); split.realp = 0;}
    if(split.imagp != 0) {free(split.imagp);split.imagp = 0;}
#endif
}

void FftshellAudioProcessor::clearMemory()
{
    memset(inBufferL, 0, kMaxSizeFFT * sizeof(float));
    memset(inBufferR, 0, kMaxSizeFFT * sizeof(float));
    memset(outBufferL, 0, kMaxSizeFFT * sizeof(float));
    memset(outBufferR, 0, kMaxSizeFFT * sizeof(float));
    memset(inShiftL, 0, kMaxSizeFFT * sizeof(float));
    memset(inShiftR, 0, kMaxSizeFFT * sizeof(float));
    memset(outShiftL, 0, kMaxSizeFFT * sizeof(float));
    memset(outShiftR, 0, kMaxSizeFFT * sizeof(float));
    memset(inSpectraL, 0, kMaxSizeFFT * sizeof(float));
    memset(inSpectraR, 0, kMaxSizeFFT * sizeof(float));
    memset(outSpectraL, 0, kMaxSizeFFT * sizeof(float));
    memset(outSpectraR, 0, kMaxSizeFFT * sizeof(float));
}

void FftshellAudioProcessor::setFFTSize(int32_t newSize)
{
    log2n = (int32_t)log2f((float)newSize);
    newSize = (int32_t)(powf(2.0f, log2n));
    sizeFFT = newSize;
    blockSizeFFT = sizeFFT >> overLapFFT;
    halfSizeFFT = sizeFFT >> 1;
    oneOverBlockSize = 1.0f/(float)blockSizeFFT;
    bufferPosition = 0;
#if FFT_USED_FFTW
    planIn = rfftw_create_plan(sizeFFT, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
    planOut = rfftw_create_plan(sizeFFT, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
#elif FFT_USED_APPLEVECLIB
    //    setupReal = create_fftsetup(log2n, FFT_RADIX2); // no need to do anything if set up for max size
#elif FFT_USED_INTEL
#endif
    clearMemory();
    switch(windowSelected)
    {
        case kHamming:
            initHammingWindows();
            break;
        case kVonHann:
            initVonHannWindows();
            break;
        case kBlackman:
            initBlackmanWindows();
            break;
        case kKaiser:
            initKaiserWindows();
            break;
    }
    scaleWindows();
}

float FftshellAudioProcessor::lin_interpolate(float phase, float harm)
{
    int x1 = phase * wavesize * harm;
    float y1 = waveform[x1 & (wavesize-1)];
    float y2 = waveform[(x1 + 1) & (wavesize-1)];
        
    return (y2 - y1) * ((phase * wavesize * harm) - x1) + y1;
}

void FftshellAudioProcessor::processActual (AudioSampleBuffer& buffer)
{
    int32_t i, v, framesLeft, processframes;
    const float *in1;
    const float *in2;
    float *s1;
    float *s2;
    float *out1;
    float *out2;

    s1 = synthAccL;
    in1 = buffer.getReadPointer(0);
    out1 = buffer.getWritePointer(0);
    if(buffer.getNumChannels() == 1)
    {
        in2 = in1;
        out2 = out1;
        s2 = s1;
        channelMode = kMonoMode;
    }
    else
    {
        in2 = buffer.getReadPointer(1);
        out2 = buffer.getWritePointer(1);
        s2 = synthAccR;
        channelMode = kStereoMode;
    }
    
    framesLeft = buffer.getNumSamples();
    
    // first we need to calculate the phase increment from the frequency
    // and sample rate - this is the number of cycles per sample
    // freq = cyc/sec, sr = samp/sec, phaseinc = cyc/samp = freq/sr
    memset(synthAccL, 0, framesLeft * sizeof(float));
    for(v = 0; v < 12; v++)
    {
        if(voices[v].active == 1)
        {
            for(i = 0; i < framesLeft; i++)
            {
                voices[v].phase += voices[v].phaseincrement;
                while(voices[v].phase >= 1.0f)
                    voices[v].phase -= 1.0f;
                while(voices[v].phase < 0.0f)
                    voices[v].phase += 1.0f;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 1.0f) * 1.0f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 2.0f) * 0.5f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 3.0f) * 0.3f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 4.0f) * 0.25f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 5.0f) * 0.2f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 6.0f) * 0.16f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 7.0f) * 0.14f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 8.0f) * 0.125f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 9.0f) * 0.111f * voices[v].gain;
                *(synthAccL+i) += lin_interpolate(voices[v].phase, 10.0f) * 0.1f * voices[v].gain;
                *(synthAccR+i) = *(synthAccL+i);
                voices[v].gain += voices[v].gaininc;
                if(voices[v].gain > 0.2)
                {
                    voices[v].gain = 0.2;
                    voices[v].gaininc = 0.0;
                }
                else if(voices[v].gain < 0.0)
                {
                    voices[v].gain = 0.0;
                    voices[v].gaininc = 0.0;
                    voices[v].active = 0;
                }
            }
        }
    }
    while (framesLeft > 0)
    {
        // how many frames can we process now
        // with this we insure that we stop on the
        // blockSizeFFT boundary
        if(framesLeft+bufferPosition < blockSizeFFT)
            processframes = framesLeft;
        else
            processframes = blockSizeFFT - bufferPosition;
        // flush out the previous output, copy in the new input...
        memcpy(inBufferL+bufferPosition, s1, processframes*sizeof(float));
        for(i=0; i<processframes; i++)
        {
            if (outBufferL[i + bufferPosition] > 1.5f) outBufferL[i + bufferPosition] = 1.5f;
            if (outBufferL[i + bufferPosition] < -1.5f) outBufferL[i + bufferPosition] = -1.5f;
            
            // copy the old output into the out buffers
            out1[i] = outBufferL[i+bufferPosition];
        }
        if(channelMode == kStereoMode)
        {
            memcpy(inBufferR+bufferPosition, in2, processframes*sizeof(float));
            for(i=0; i<processframes; i++)
            {
                if (outBufferR[i + bufferPosition] > 1.5f) outBufferR[i + bufferPosition] = 1.5f;
                if (outBufferR[i + bufferPosition] < -1.5f) outBufferR[i + bufferPosition] = -1.5f;
                // copy the old output into the out buffers
                out2[i] = outBufferR[i+bufferPosition];
            }
            in2 += processframes;
            out2 += processframes;
            s2 += processframes;
        }
        
        bufferPosition += processframes;
        // if filled a buffer, we process a new block
        if(bufferPosition >= blockSizeFFT)
        {
            bufferPosition = 0;
            processFFTBlock();
        }
        in1 += processframes;
        out1 += processframes;
        s1 += processframes;
       framesLeft -= processframes;
    }
}
// this will generally be overridden to allow FFT size switching from GUI parameter
// called by processFFTBlock
void FftshellAudioProcessor::updateFFT()
{
    ;   // no-op
}

void FftshellAudioProcessor::processFFTBlock()
{
    int32_t    i, j;
    float    outTemp;
    int32_t    maskFFT;
    
    updateFFT();
    maskFFT = sizeFFT - 1;
    //
    inputTimeL += blockSizeFFT;
    inputTimeR += blockSizeFFT;
    outputTimeL += blockSizeFFT;
    outputTimeR += blockSizeFFT;
    inputTimeL = inputTimeL & maskFFT;
    inputTimeR = inputTimeR & maskFFT;
    outputTimeL = outputTimeL & maskFFT;
    outputTimeR = outputTimeR & maskFFT;
    //
    // a - shift output buffer and zero out new location
    memcpy(outBufferL, outBufferL+blockSizeFFT, (sizeFFT - blockSizeFFT) * sizeof(float));
    memset(outBufferL+(sizeFFT - blockSizeFFT), 0, blockSizeFFT * sizeof(float));
    if(channelMode == kStereoMode)
    {
        // a - shift output buffer and zero out new location
        memcpy(outBufferR, outBufferR+blockSizeFFT, (sizeFFT - blockSizeFFT) * sizeof(float));
        memset(outBufferR+(sizeFFT - blockSizeFFT), 0, blockSizeFFT * sizeof(float));
    }
    // a - shift current samples in inShift
    memcpy(inShiftL, inShiftL+blockSizeFFT, (sizeFFT - blockSizeFFT) * sizeof(float));
    // a1 - copy the new stuff in
    memcpy(inShiftL+(sizeFFT - blockSizeFFT), inBufferL, blockSizeFFT * sizeof(float));
    // b - window the block in
    for(i = 0; i < sizeFFT; i++)
    {
        *(inFFTL + inputTimeL) = *(inShiftL + i) * *(analysisWindow + i);
        ++inputTimeL;
        inputTimeL = inputTimeL & maskFFT;
    }
#if FFT_USED_FFTW
    rfftw_one(planIn, inFFTL, inSpectraL);
#elif FFT_USED_APPLEVECLIB
    vDSP_ctoz ((COMPLEX *)inFFTL, 2, &split, 1, halfSizeFFT);
    vDSP_fft_zrip (setupReal, &split, 1, log2n, FFT_FORWARD);
    memcpy(inSpectraL, split.realp, halfSizeFFT * sizeof(float));
    *(inSpectraL + halfSizeFFT) = *(split.imagp);
    for(i = halfSizeFFT + 1, j = halfSizeFFT - 1; i < sizeFFT; i++, j--)
        *(inSpectraL + i) = *(split.imagp + j);
#elif FFT_USED_INTEL
    ippsFFTFwd_RToPerm_32f_I(inFFTL, fftSpec[log2n-4], mFFTWorkBuf[log2n - 4]);
    *(inSpectraL+0) = *(inFFTL+0);
    *(inSpectraL + halfSizeFFT) = *(inFFTL+1);
    for(i = 1, j = 2; j < sizeFFT; j+=2, i++)
        *(inSpectraL + i) = *(inFFTL+j);
    for(i = halfSizeFFT + 1, j = sizeFFT - 1; i < sizeFFT; i++, j-=2)
        *(inSpectraL + i) = *(inFFTL+j);
#endif
    if(channelMode == kStereoMode)
    {
        // a - shift current samples in inShift
        memcpy(inShiftR, inShiftR+blockSizeFFT, (sizeFFT - blockSizeFFT) * sizeof(float));
        // a1 - copy the new stuff in
        memcpy(inShiftR+(sizeFFT - blockSizeFFT), inBufferR, blockSizeFFT * sizeof(float));
        // b - window the block in
        for(i = 0; i < sizeFFT; i++)
        {
             *(inFFTR + inputTimeR) = *(inShiftR + i) * *(analysisWindow + i);
            ++inputTimeR;
            inputTimeR = inputTimeR & maskFFT;
        }
#if FFT_USED_FFTW
        rfftw_one(planIn, inFFTR, inSpectraR);
#elif FFT_USED_APPLEVECLIB
        vDSP_ctoz ((COMPLEX *)inFFTR, 2, &split, 1, halfSizeFFT);
        vDSP_fft_zrip (setupReal, &split, 1, log2n, FFT_FORWARD);
        memcpy(inSpectraR, split.realp, halfSizeFFT * sizeof(float));
        *(inSpectraR + halfSizeFFT) = *(split.imagp);
        for(i = halfSizeFFT + 1, j = halfSizeFFT - 1; i < sizeFFT; i++, j--)
            *(inSpectraR + i) = *(split.imagp + j);
#elif FFT_USED_INTEL
        ippsFFTFwd_RToPerm_32f_I(inFFTR, fftSpec[log2n-4], mFFTWorkBuf[log2n - 4]);
        *(inSpectraR+0) = *(inFFTR+0);
        *(inSpectraR + halfSizeFFT) = *(inFFTR+1);
        for(i = 1, j = 2; j < sizeFFT; j+=2, i++)
            *(inSpectraR + i) = *(inFFTR+j);
        for(i = halfSizeFFT + 1, j = sizeFFT - 1; i < sizeFFT; i++, j-=2)
            *(inSpectraR + i) = *(inFFTR+j);
#endif
    }
    
    processSpect();
    
    // d - IFFT
#if FFT_USED_FFTW
    rfftw_one(planOut, outSpectraL, outShiftL);
#elif FFT_USED_APPLEVECLIB
    memcpy(split.realp, outSpectraL, halfSizeFFT * sizeof(float));
    *(split.imagp) = *(outSpectraL + halfSizeFFT);
    for(i = halfSizeFFT + 1, j = halfSizeFFT - 1; i < sizeFFT; i++, j--)
        *(split.imagp + j) = *(outSpectraL + i);
    vDSP_fft_zrip (setupReal, &split, 1, log2n, FFT_INVERSE);
    vDSP_ztoc (&split, 1, ( COMPLEX * ) outShiftL, 2, halfSizeFFT );
#elif FFT_USED_INTEL
    *(outShiftL+0) = *(outSpectraL+0);
    *(outShiftL+1) = *(outSpectraL + halfSizeFFT);
    for(i = 1, j = 2; j < sizeFFT; j+=2, i++)
        *(outShiftL+j) = *(outSpectraL + i);
    for(i = halfSizeFFT + 1, j = sizeFFT - 1; i < sizeFFT; i++, j-=2)
        *(outShiftL+j) = *(outSpectraL + i);
    ippsFFTInv_PermToR_32f_I(outShiftL, fftSpec[log2n-4], mFFTWorkBuf[log2n - 4]);
#endif
    // e - overlap add
    for(i = 0; i < sizeFFT; i++)
    {
        if((*(outShiftL + outputTimeL) == *(outShiftL + outputTimeL)) != 1)
            *(outShiftL + outputTimeL) = 0.0f;
        outTemp = *(outShiftL + outputTimeL) * *(synthesisWindow + i);
        *(outBufferL+i) += outTemp;
        ++outputTimeL;
        outputTimeL = outputTimeL & maskFFT;
    }
    if(channelMode == kStereoMode)
    {
        // d - IFFT
#if FFT_USED_FFTW
        rfftw_one(planOut, outSpectraR, outShiftR);
#elif FFT_USED_APPLEVECLIB
        memcpy(split.realp, outSpectraR, halfSizeFFT * sizeof(float));
        *(split.imagp) = *(outSpectraR + halfSizeFFT);
        for(i = halfSizeFFT + 1, j = halfSizeFFT - 1; i < sizeFFT; i++, j--)
            *(split.imagp + j) = *(outSpectraR + i);
        vDSP_fft_zrip (setupReal, &split, 1, log2n, FFT_INVERSE);
        vDSP_ztoc (&split, 1, ( COMPLEX * ) outShiftR, 2, halfSizeFFT );
#elif FFT_USED_INTEL
        *(outShiftR+0) = *(outSpectraR+0);
        *(outShiftR+1) = *(outSpectraR + halfSizeFFT);
        for(i = 1, j = 2; j < sizeFFT; j+=2, i++)
            *(outShiftR+j) = *(outSpectraR + i);
        for(i = halfSizeFFT + 1, j = sizeFFT - 1; i < sizeFFT; i++, j-=2)
            *(outShiftR+j) = *(outSpectraR + i);
        ippsFFTInv_PermToR_32f_I(outShiftR, fftSpec[log2n-4], mFFTWorkBuf[log2n - 4]);
#endif
        // e - overlap add
        for(i = 0; i < sizeFFT; i++)
        {
            if((*(outShiftR + outputTimeR) == *(outShiftR + outputTimeR)) != 1)
                *(outShiftR + outputTimeR) = 0.0f;
            outTemp = *(outShiftR + outputTimeR) * *(synthesisWindow + i);
            *(outBufferR+i) += outTemp;
            ++outputTimeR;
            outputTimeR = outputTimeR & maskFFT;
        }
    }
}

void    FftshellAudioProcessor::processSpect()
{
    int i, bGainInt;
    float gain, bGainFrac;
    
    // convolution is a complex multiply
    // real = real1 * real2 - imag1 * imag2
    // imag = real1 * imag2 + real2 * imag1
    // cos = cos1 * cos2 - sin1 * sin2
    // sin = cos1 * sin2 + cos2 * sin1

    // process dc and nyquist, then the rest of the bands
    if(convolve > 0.5)
    {
        outSpectraR[0] = outSpectraL[0] = inSpectraL[0] * inSpectraR[0];    // DC Component
        outSpectraR[halfSizeFFT] = outSpectraL[halfSizeFFT] = inSpectraL[halfSizeFFT] * inSpectraR[halfSizeFFT];    // Nyquist Frequency
        
        for(i = 1; i < halfSizeFFT; ++i)
        {
            outSpectraR[i] = outSpectraL[i] = inSpectraL[i] * inSpectraR[i] - inSpectraL[sizeFFT - i] * inSpectraR[sizeFFT - i];
            
            outSpectraR[sizeFFT - i] = outSpectraL[sizeFFT - i] = inSpectraL[i] * inSpectraR[sizeFFT - i] + inSpectraR[i] * inSpectraL[sizeFFT - i];
        }
        inSpectraL[0] = inSpectraR[0] = outSpectraL[0];
        inSpectraL[halfSizeFFT] = inSpectraR[halfSizeFFT] = outSpectraL[halfSizeFFT];
        for(i = 1; i < halfSizeFFT; ++i)
        {
            inSpectraR[i] = inSpectraL[i] = outSpectraL[i];
            inSpectraL[sizeFFT - i] = inSpectraR[sizeFFT - i] = outSpectraL[sizeFFT - i];
        }
    }
 
    // process dc and nyquist, then the rest of the bands
    outSpectraL[0] = inSpectraL[0] * eqgain[0];    // DC Component
    outSpectraL[halfSizeFFT] = inSpectraL[halfSizeFFT] * eqgain[4];    // Nyquist Frequency
    outSpectraR[0] = inSpectraR[0] * eqgain[0];    // DC Component
    outSpectraR[halfSizeFFT] = inSpectraR[halfSizeFFT] * eqgain[4];    // Nyquist Frequency
    
    for(i = 1; i < halfSizeFFT; ++i)
    {
        bGainInt = (int)(bandGain[i]);
        bGainFrac = bandGain[i] - (float)bGainInt;
        // comments would be helpful - -1
        gain = eqgain[bGainInt] + bGainFrac * (eqgain[bGainInt + 1] - eqgain[bGainInt]);
        
        outSpectraL[i] = inSpectraL[i] * gain;
        outSpectraL[sizeFFT - i] = inSpectraL[sizeFFT - i] * gain;
        
        outSpectraR[i] = inSpectraR[i] * gain;
        outSpectraR[sizeFFT - i] = inSpectraR[sizeFFT - i] * gain;
    }
}
void    FftshellAudioProcessor::initHammingWindows(void)
{
    int32_t    index;
    float    a = 0.54f, b    = 0.46f;
    
    windowSelected = kHamming;
    
    // a - make two hamming windows
    for (index = 0; index < sizeFFT; index++)
        synthesisWindow[index] = analysisWindow[index] = a - b*cosf(twoPi*index/(sizeFFT - 1));
}

void    FftshellAudioProcessor::initVonHannWindows(void)
{
    int32_t    index;
    float    a = 0.50, b    = 0.40;
    
    windowSelected = kVonHann;
    
    // a - make two hamming windows
    for (index = 0; index < sizeFFT; index++)
        synthesisWindow[index] = analysisWindow[index] = a - b*cosf(twoPi*index/(sizeFFT - 1));
}

void    FftshellAudioProcessor::initBlackmanWindows(void)
{
    int32_t    index;
    float    a = 0.42, b    = 0.50, c = 0.08;
    
    windowSelected = kBlackman;
    
    // a - make two hamming windows
    for (index = 0; index < sizeFFT; index++)
        synthesisWindow[index] = analysisWindow[index] = a - b*cosf(twoPi*index/(sizeFFT - 1)) + c*cosf(2.0f*twoPi*index/(sizeFFT - 1));
    
}

void    FftshellAudioProcessor::initKaiserWindows(void)
{
    
    double    bes, xind, floati;
    int32_t    i;
    
    windowSelected = kKaiser;
    
    bes = kaiserIno(6.8);
    xind = (float)(sizeFFT-1)*(sizeFFT-1);
    
    for (i = 0; i < halfSizeFFT; i++)
    {
        floati = (double)i;
        floati = 4.0 * floati * floati;
        floati = sqrt(1. - floati / xind);
        synthesisWindow[i+halfSizeFFT] = kaiserIno(6.8 * floati);
        analysisWindow[halfSizeFFT - i] = synthesisWindow[halfSizeFFT - i] = analysisWindow[i+halfSizeFFT] = (synthesisWindow[i+halfSizeFFT] /= bes);
    }
    analysisWindow[sizeFFT - 1] = synthesisWindow[sizeFFT - 1] = 0.0;
    analysisWindow[0] = synthesisWindow[0] = 0.0;
}

float    FftshellAudioProcessor::kaiserIno(float x)
{
    float    y, t, e, de, sde, xi;
    int32_t i;
    
    y = x / 2.;
    t = 1.e-08;
    e = 1.;
    de = 1.;
    for (i = 1; i <= 25; i++)
    {
        xi = i;
        de = de * y / xi;
        sde = de * de;
        e += sde;
        if (e * t > sde)
            break;
    }
    return(e);
}

void    FftshellAudioProcessor::scaleWindows(void)
{
    int32_t    index;
    float    sum, analFactor, synthFactor;
    
    // b - scale the windows
    sum = 0.0f;
    for (index = 0; index < sizeFFT; index++)
        sum += analysisWindow[index];
    
    synthFactor = analFactor = 2.0f/sum;
    
    for (index = 0; index < sizeFFT; index++)
    {
        analysisWindow[index] *= analFactor;
        synthesisWindow[index] *= synthFactor;
    }
    sum = 0.0;
    for (index = 0; index < sizeFFT; index += blockSizeFFT)
        sum += synthesisWindow[index]*synthesisWindow[index];
    // i think this scaling factor is only needed for vector lib
#if FFT_USED_FFTW
    sum = 1.0f/(sum*sizeFFT);
#elif FFT_USED_APPLEVECLIB
    sum = 1.0f/(sum*sizeFFT*2.0f);
#elif FFT_USED_INTEL
    sum = 1.0f/sum;
#endif
    for (index = 0; index < sizeFFT; index++)
        synthesisWindow[index] *= sum;
}


void    FftshellAudioProcessor::cartToPolar(float *spectrum, float *polarSpectrum)
{
    int32_t    realIndex, imagIndex, ampIndex, phaseIndex;
    float    realPart, imagPart;
    int32_t    bandNumber;
    
    
    for (bandNumber = 0; bandNumber <= halfSizeFFT; bandNumber++)
    {
        realIndex = bandNumber;
        imagIndex = sizeFFT - bandNumber;
        ampIndex = bandNumber<<1;
        phaseIndex = ampIndex + 1;
        if(bandNumber == 0)
        {
            realPart = spectrum[0];
            imagPart = 0.0;
        }
        else if(bandNumber == halfSizeFFT)
        {
            realPart = spectrum[halfSizeFFT];
            imagPart = 0.0;
        }
        else
        {
            realPart = spectrum[realIndex];
            imagPart = spectrum[imagIndex];
        }
        /*
         * compute magnitude & phase value from real and imaginary parts
         */
        
        polarSpectrum[ampIndex] = hypot(realPart, imagPart);
        if(polarSpectrum[ampIndex] < 0.0f)
            polarSpectrum[phaseIndex] = 0.0;
        else
            polarSpectrum[phaseIndex] = -atan2f(imagPart, realPart);
    }
}


void    FftshellAudioProcessor::polarToCart(float *polarSpectrum, float *spectrum)
{
    float    realValue, imagValue;
    int32_t    bandNumber, realIndex, imagIndex, ampIndex, phaseIndex;
    
    /*
     * convert to cartesian coordinates, complex pairs
     */
    for (bandNumber = 0; bandNumber <= halfSizeFFT; bandNumber++)
    {
        realIndex = bandNumber;
        imagIndex = sizeFFT - bandNumber;
        ampIndex = bandNumber<<1;
        phaseIndex = ampIndex + 1;
        if(polarSpectrum[ampIndex] == 0.0)
        {
            realValue = 0.0;
            imagValue = 0.0;
        }
        else if(bandNumber == 0 || bandNumber == halfSizeFFT)
        {
            realValue = polarSpectrum[ampIndex] * cosf(polarSpectrum[phaseIndex]);
            imagValue = 0.0;
        }
        else
        {
            realValue = polarSpectrum[ampIndex] * cosf(polarSpectrum[phaseIndex]);
            imagValue = polarSpectrum[ampIndex] * -sinf(polarSpectrum[phaseIndex]);
        }
        
        
        if(bandNumber == halfSizeFFT)
            realIndex = halfSizeFFT;
        spectrum[realIndex] = realValue;
        if(bandNumber != halfSizeFFT && bandNumber != 0)
            spectrum[imagIndex] = imagValue;
    }
}


//==============================================================================
const String FftshellAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FftshellAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FftshellAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FftshellAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FftshellAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FftshellAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FftshellAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FftshellAudioProcessor::setCurrentProgram (int index)
{
}

const String FftshellAudioProcessor::getProgramName (int index)
{
    return {};
}

void FftshellAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void FftshellAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synthAccL = (float *)realloc(synthAccL, sizeof(float) * samplesPerBlock);
    synthAccR = (float *)realloc(synthAccR, sizeof(float) * samplesPerBlock);
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void FftshellAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    if(synthAccL != 0) {free(synthAccL); synthAccL = 0; }
    if(synthAccR != 0) {free(synthAccR); synthAccR = 0; }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FftshellAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FftshellAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
   MidiBuffer::Iterator it(midiMessages);
    MidiMessage msg(0x80,0,0,0);
    int pos;
    int32_t note, v;
    
    // start with the MIDI processing
    while(it.getNextEvent(msg , pos))
    {
        if(msg.isNoteOn())
        {
            note = msg.getNoteNumber();
            for(v = 0; v < 12; v++)
                if(voices[v].active == 0)
                    break;
            voices[v].active = 1;
            voices[v].midinote = note;
            voices[v].gaininc = 0.2/(0.01 * getSampleRate()); // 10ms attack
            voices[v].phaseincrement = (pow(2.0, (double)note/12.0) * 8.175798915643707)/getSampleRate();
            voices[v].phase = 0.0;
            voices[v].gain = 0.0;
        }
        if(msg.isNoteOff())
        {
            note = msg.getNoteNumber();
            for(v = 0; v < 12; v++)
                if(voices[v].active == 1 && voices[v].midinote == note)
                {
                    voices[v].midinote = 0;
                    voices[v].gaininc = -0.2/(0.5 * getSampleRate()); // 500ms release
                    break;
                }
        }
/*        if(msg.isPitchWheel())
        {
            bendPitch(msg.getPitchWheelValue());
        }*/
    }
    convolve = (*parameters.getRawParameterValue("convolve"));
    eqgain[0] = pow(10.0, *parameters.getRawParameterValue("one") * 0.05);
    eqgain[1] = pow(10.0, *parameters.getRawParameterValue("two") * 0.05);
    eqgain[2] = pow(10.0, *parameters.getRawParameterValue("three") * 0.05);
    eqgain[3] = pow(10.0, *parameters.getRawParameterValue("four") * 0.05);
    eqgain[4] = pow(10.0, *parameters.getRawParameterValue("five") * 0.05);
    
    processActual(buffer);
   // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

}

//==============================================================================
bool FftshellAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* FftshellAudioProcessor::createEditor()
{
    return new FftshellAudioProcessorEditor (*this);
}

//==============================================================================
void FftshellAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FftshellAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FftshellAudioProcessor();
}
