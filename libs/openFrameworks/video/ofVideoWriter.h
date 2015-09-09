//
//  ofVideoWriter.h
//  ShapeDeform
//
//  Created by roy_shilkrot on 4/7/13.
//
//

#ifndef __ofVideoWriter__
#define __ofVideoWriter__

#include <iostream>
#include "ofPixels.h"

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#endif

class ofVideoWriter {

public:
	ofVideoWriter() :oc(NULL), codec(NULL), initialized(false), frame_count(1), b_recording(false) {}

	/**
	 * setup the video writer
	 * @param output filename, the codec and format will be determined by it. (e.g. "xxx.mpg" will create an MPEG1 file
	 * @param width of the frame
	 * @param height of the frame
	 **/
	void setup(const char* filename, int width, int height, int bitrate = 400000, int framerate = 25, AVPixelFormat inputFormat = AV_PIX_FMT_RGB24, AVPixelFormat outputFormat = AV_PIX_FMT_YUV420P);
	/**
	 * add a frame to the video file
	 * @param the pixels packed in RGB (24-bit RGBRGBRGB...)
	 **/
	void addFrame(const ofPixels pixels);
	/**
	* add a frame to the video file
	* @param the pixels packed in RGB (24-bit RGBRGBRGB...)
	**/
	void addAudioFrame(const float* samples);
	/**
	 * close the video file and release all datastructs
	 **/
	void close();
	/**
	 * is the videowriter initialized?
	 **/
	bool isInitialized() const { return initialized; }
	bool isRecording() const { return b_recording; }
	void start() { if (initialized) b_recording = true; }

private:
	//instance variables
	AVCodec *codec;
	int size, frame_count;
	AVFrame *_outputFrameData, *_inputFrameData;
	struct SwsContext *sws_ctx;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *video_st;
	AVCodecContext* c;

	bool initialized, b_recording;
};

#endif /* defined(__ofVideoWriter__) */
