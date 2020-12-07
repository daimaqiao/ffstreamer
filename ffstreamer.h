#ifndef __FFSTREAMER_H__
#define __FFSTREAMER_H__
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
};
#include <string>
#include <iostream>

#include "fflogging.h"

#define FFOK    0
#define FFERR   -1
#define IS_FFOK(ret)    (ret>=FFOK)
#define IS_FFERR(ret)   (ret<=FFERR)

#define FFTRUE  1
#define FFFALSE 0

class FFstreamer {
public:
    FFstreamer(const char* inputName, const char* outputName);
    ~FFstreamer();

    int init();
    int runStreaming();
    void uninit();

private:
    const char* _inputName;
    const char* _outputName;

    AVFormatContext* _avctxIn = NULL;
    AVFormatContext* _avctxOut = NULL;
    AVPacket _avpkt;

    int _ivideo = -1;
    int _iaudio = -1;
    int64_t _nframes = 0;
    int64_t _startTime = 0;

private:
    int initInput();
    int initOutput();
    int initOutputStream(int index);
};

#endif