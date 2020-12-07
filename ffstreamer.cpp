#include "ffstreamer.h"

FFstreamer::FFstreamer(const char* inputName, const char* outputName):
    _inputName(inputName), _outputName(outputName) {

    assert(_inputName);
    assert(_outputName);
}

FFstreamer::~FFstreamer() {
    uninit();
}

int FFstreamer::init() {
    int ret = avformat_network_init();
    if(IS_FFERR(ret)) {
        LOGE("avformat_network_init returns %d.", ret);
        return ret;
    }
    av_log_set_level(AV_LOG_INFO);

    ret = initInput();
    if(IS_FFERR(ret)) {
        LOGE("Init input error!");
        return ret;
    }

    ret = initOutput();
    if(IS_FFERR(ret)) {
        LOGE("Init output error!");
        return ret;
    }

    return FFOK;
}

int FFstreamer::initInput() {
    int ret = avformat_open_input(&_avctxIn, _inputName, NULL, NULL);
    if(IS_FFERR(ret)) {
        LOGE("avformat_open_input returns %d.", ret);
        return ret;
    }

    ret = avformat_find_stream_info(_avctxIn, NULL);
    if(IS_FFERR(ret)) {
        LOGE("avformat_find_stream_info returns %d.", ret);
        return ret;
    }
    av_dump_format(_avctxIn, 0, _inputName, FFFALSE);

    for(int i = 0; i < _avctxIn->nb_streams; i++) {
        AVMediaType mt = _avctxIn->streams[i]->codecpar->codec_type;
        const char* desc = "";
        if(mt == AVMEDIA_TYPE_VIDEO && _ivideo < 0) {
            desc = "video";
            _ivideo = i;
        } else if(mt == AVMEDIA_TYPE_AUDIO && _iaudio < 0) {
            desc = "audio";
            _iaudio = i;
        } else
            desc = "ignore";

        LOGD("Input stream %d type %d %s.", i, mt, desc);
    }// for

    if(_ivideo < 0) {
        LOGW("Input %s without video stream.", _inputName);
    }

    if(_iaudio < 0) {
        LOGW("Input %s without audio stream.", _inputName);
    }

    LOGD("Use video stream index %d.", _ivideo);
    LOGD("Use audio stream index %d.", _iaudio);
    return FFOK;
}

int FFstreamer::initOutput() {
    int ret = avformat_alloc_output_context2(&_avctxOut, NULL, "flv", _outputName);
    if(IS_FFERR(ret)) {
        LOGE("avformat_alloc_output_context2 returns %d.", ret);
        return ret;
    }
    assert(_avctxOut);

    for(int i = 0; i < _avctxIn->nb_streams; i++) {
        if(i == _iaudio || i == _ivideo)
            initOutputStream(i);
    }// for
    av_dump_format(_avctxOut, 0, _outputName, FFTRUE);

    if(!(_avctxOut->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&_avctxOut->pb, _outputName, AVIO_FLAG_WRITE);
        if(IS_FFERR(ret)) {
            LOGE("avio_open returns %d.", ret);
            return ret;
        }
    }

    return FFOK;
}

int FFstreamer::initOutputStream(int index) {
    AVStream* sin = _avctxIn->streams[index];
    AVStream* sout = avformat_new_stream(_avctxOut, NULL);
    if(!sout) {
        LOGE("stream index %d, avformat_new_stream return NULL.", index);
        return FFERR;
    }

    int ret = avcodec_parameters_copy(sout->codecpar, sin->codecpar);
    if(IS_FFERR(ret)) {
        LOGE("stream index %d, avcodec_parameters_copy returns %d.", index, ret);
        return ret;
    }
    sout->codecpar->codec_tag = 0;

    return FFOK;
}

int FFstreamer::runStreaming() {
    int ret = avformat_write_header(_avctxOut, NULL);
    if(IS_FFERR(ret)) {
        LOGE("avformat_write_header returns %d.", ret);
        return ret;
    }

    _startTime = av_gettime();
    while(FFTRUE) {
        int ret = av_read_frame(_avctxIn, &_avpkt);
        if(IS_FFERR(ret)) {
            if(ret == AVERROR_EOF) {
                LOGI("Read EOF.");
            } else {
                LOGE("av_read_frame returns %d.", ret);
            }
            break;
        }

        if(_avpkt.stream_index != _ivideo && _avpkt.stream_index != _iaudio)
            continue;

        if(_ivideo >= 0) {
            if(_avpkt.pts == AV_NOPTS_VALUE) {
                LOGW("Video stream index %d pts=AV_NOPTS_VALUE", _avpkt.stream_index);
                AVRational tb = _avctxIn->streams[_ivideo]->time_base;
                int64_t duration = (double)AV_TIME_BASE / av_q2d(_avctxIn->streams[_ivideo]->r_frame_rate);
                _avpkt.pts = (double)(_nframes * duration) / (double)(av_q2d(tb) * AV_TIME_BASE);
                _avpkt.dts = _avpkt.pts;
                _avpkt.duration = (double)duration / (double)(av_q2d(tb) * AV_TIME_BASE);
            }

            if(_avpkt.stream_index == _ivideo) {
                AVRational tb = _avctxIn->streams[_ivideo]->time_base;
                AVRational tbq = AV_TIME_BASE_Q;
                int64_t pts = av_rescale_q(_avpkt.dts, tb, tbq);
                int64_t now = av_gettime() - _startTime;
                if(pts > now)
                    av_usleep(pts - now);

                _nframes++;
            }
        }

        AVStream* sin = _avctxIn->streams[_avpkt.stream_index];
        AVStream* sout = _avctxOut->streams[_avpkt.stream_index];
        _avpkt.pts = av_rescale_q_rnd(_avpkt.pts, sin->time_base, sout->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        _avpkt.dts = av_rescale_q_rnd(_avpkt.dts, sin->time_base, sout->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        _avpkt.duration = av_rescale_q(_avpkt.duration, sin->time_base, sout->time_base);
        _avpkt.pos = -1;

        ret = av_interleaved_write_frame(_avctxOut, &_avpkt);

        if(IS_FFERR(ret)) {
            if(ret == AVERROR_EOF) {
                LOGW("Write EOF.");
            } else {
                LOGE("av_interleaved_write_frame returns %d.", ret);
            }
            break;
        }

        av_packet_unref(&_avpkt);
    }// while

    ret = av_write_trailer(_avctxOut);
    if(IS_FFERR(ret)) {
        LOGW("av_write_trailer returns %d.", ret);
    }

    return ret;
}

void FFstreamer::uninit() {
    if(_avctxOut && !(_avctxOut->oformat->flags & AVFMT_NOFILE))
        avio_close(_avctxOut->pb);

    avformat_close_input(&_avctxIn);

    avformat_free_context(_avctxOut);
    _avctxOut = NULL;

    avformat_network_deinit();
}
