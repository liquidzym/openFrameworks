//
//  ofAVWriter.h
//  ShapeDeform
//
//  Created by roy_shilkrot on 4/7/13.
//
//

#ifndef __ofAVWriter__
#define __ofAVWriter__

#include <iostream>
#include "ofPixels.h"

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#endif

// a wrapper around a single output AVStream
typedef struct AudioStream {
	//stream handle
	AVStream *st;

	int samples_count, channels, sampleRate, audioBR;

	AVSampleFormat inFormat, outFormat;
	AVFrame *inSamples, *outSamples;

	//codec information
	AVCodecContext *context;
	AVCodec * codec;

	//resampling context
	struct SwrContext *swr_ctx;
} AudioStream;

// a wrapper around a single output AVStream
typedef struct VideoStream {
	//stream handle
	AVStream *st;

	int size, videoBR, width, height, framerate;

	//pixel format and frame buffers
	AVPixelFormat inFormat, outFormat;
	AVFrame *outFrameData, *inFrameData;

	//codec information
	AVCodecContext *context;
	AVCodec * codec;

	//scaling context
	struct SwsContext *sws_ctx;
} VideoStream;

class ofAVWriter {

public:
	ofAVWriter() :oc(NULL), initialized(false), frame_count(1), b_recording(false), opt(NULL), b_encode_video(false), b_encode_audio(false) {}

	/**
	 * setup the video writer
	 * @param output filename, the codec and format will be determined by it. (e.g. "xxx.mpg" will create an MPEG1 file
	 * @param width of the frame
	 * @param height of the frame
	 **/
	void setup(const char* filename, int width, int height, bool recordVideo = true, bool recordAudio = false, int framerate = 25, int videoBitRate = 400000, int sampleRate = 44100, int audioBitRate = 64000, int channels = 2, AVPixelFormat inputFormat = AV_PIX_FMT_RGB24, AVPixelFormat outputFormat = AV_PIX_FMT_YUV420P);
	/**
	 * add a frame to the video file
	 * @param the pixels packed in RGB (24-bit RGBRGBRGB...)
	 **/
	void addFrame(const ofPixels pixels);
	/**
	* add a frame to the audio file
	* @param the samples in float format
	**/
	void addAudioSample(const float* samples, int bufferSize, int nChannels);
	/**
	 * close the video file and release all datastructs
	 **/
	void close();
	/**
	 * is the videowriter initialized?
	 **/
	bool isInitialized() const { return initialized; }
	bool isRecording() const { return b_recording; }
	void start() { if (initialized) { b_recording = true; ofLogNotice("ofAVWriter") << "Beginning Recording"; } else ofLogError("ofAVWriter") << "has not be initialized. run setup()"; }
	void setSampleRate(int sampleRate) { audio.sampleRate = sampleRate; initialized = false; }
	void setAudioBitRate(int bitRate) { audio.audioBR = bitRate; initialized = false; }
	void setVideoBitRate(int bitRate) { video.videoBR = bitRate; initialized = false; }
	void setWidth(int width) { video.width = width; initialized = false; }
	void setHeight(int height) { video.height = height; initialized = false; }


private:
	unsigned long long frame_count, sample_count;

	ofPixels prevFrame;

	AVOutputFormat *fmt;
	AVFormatContext *oc;
	VideoStream video = { 0 };
	AudioStream audio = { 0 };
	AVDictionary *opt;

	bool initialized, b_recording, has_audio, has_video, b_encode_video, b_encode_audio;

	void add_audio_stream(enum AVCodecID codec_id);
	void add_video_stream(enum AVCodecID codec_id);
	void open_audio();
	int write_frame(const AVRational * time_base, AVStream * st, AVPacket * pkt);
};

#endif /* defined(__ofAVWriter__) */


