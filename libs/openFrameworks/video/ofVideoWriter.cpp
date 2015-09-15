//
//  ofVideoWriter.cpp
//  ShapeDeform
//
//  Created by roy_shilkrot on 4/7/13.
//
//
// taken from ffmpeg's examples code: http://ffmpeg.org/doxygen/trunk/api-example_8c-source.html
// http://ffmpeg.org/doxygen/trunk/doc_2examples_2decoding_encoding_8c-example.html#a33
// http://ffmpeg.org/doxygen/trunk/doc_2examples_2muxing_8c-example.html#a75
//

#ifdef __cplusplus
extern "C" {
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}
#endif

#include "ofUtils.h"
#include "ofVideoWriter.h"

void ofVideoWriter::setup(const char* filename, int width, int height, int bitrate, int framerate, AVPixelFormat inputFormat, AVPixelFormat outputFormat) {
	
	_height = height;
	_width = width;
	_videoBR = bitrate;
	_framerate = framerate;

	ofLogNotice("ofVideoWriter") << "Video encoding: " << filename;
	/* register all the formats and codecs */
	av_register_all();

	/* allocate the output media context */
	avformat_alloc_output_context2(&oc, NULL, NULL, filename);
	if (!oc) {
		ofLogWarning("ofVideoWriter") << "Could not deduce output format from file extension: using MPEG.";
		avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
	}
	if (!oc) {
		ofLogError("ofVideoWriter") << "could not create AVFormat context";
		return;
	}
	fmt = oc->oformat;

	/* Add the audio and video streams using the default format codecs
	 * and initialize the codecs. */
	video_st = NULL;
	avcodec_alloc_context3(codec);
	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		/* find the video encoder */
		AVCodecID avcid = fmt->video_codec;
		codec = avcodec_find_encoder(avcid);
		if (!codec) {
			ofLogError("ofVideoWriter") << "codec not found: " << avcodec_get_name(avcid);
			return;
		}
		else {
			vector<AVPixelFormat> supported_pix_fmt;
			const AVPixelFormat* p = codec->pix_fmts;
			while (*p != AV_PIX_FMT_NONE) {
				ofLogNotice("ofVideoWriter") << "supported pix fmt: " << av_get_pix_fmt_name(*p);
				supported_pix_fmt.push_back(*p); 
				++p;
			}
			bool pixfmtsupported = false;
			for (auto formats : supported_pix_fmt) {
				if (formats == outputFormat)
					pixfmtsupported = true;
			}
			if (p == NULL || *p == AV_PIX_FMT_NONE || !pixfmtsupported) { //if there are no supported pixel formats or the requested format is not one of the supported one
				if (fmt->video_codec == AV_CODEC_ID_RAWVIDEO) {
					
					outputFormat = AV_PIX_FMT_RGB24;
				}
				else {
					outputFormat = AV_PIX_FMT_YUV420P; /* we have to assume the default pix_fmt */
				}
			}
		}

		video_st = avformat_new_stream(oc, codec);
		if (!video_st) {
			ofLogError("ofVideoWriter") << "Could not allocate stream";
			return;
		}
		video_st->id = oc->nb_streams - 1;
		video_st->time_base = AVRational{ 1, framerate };
		c = video_st->codec;

		/* Some formats want stream headers to be separate. */
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	/* Now that all the parameters are set, we can open the audio and
	 * video codecs and allocate the necessary encode buffers. */
	{

		_outputFrameData = av_frame_alloc();
		_outputFrameData->pts = 0;
		_outputFrameData->width = width;
		_outputFrameData->height = height;

		/* put sample parameters */
		c->codec_id = fmt->video_codec;
		c->bit_rate = bitrate;
		/* resolution must be a multiple of two */
		c->width = width;
		c->height = height;
		/* frames per second */
		c->time_base = AVRational{ 1, framerate };
		c->gop_size = 10; /* emit one intra frame every ten frames */
		//setting b frames results in an error with some codecs
		/*if(c->has_b_frames)
			c->max_b_frames = -1;*/
		c->pix_fmt = outputFormat;

		/* open it */
		AVDictionary* options = NULL;
		int ret = avcodec_open2(c, codec, &options);

		if (ret < 0) {
			ofLogError("ofVideoWriter") << "Could not open codec " << codec->long_name;
			return;
		}
		else {
			ofLogNotice("ofVideoWriter") << "opened " << avcodec_get_name(fmt->video_codec);
		}

		/* alloc image and output buffer */
		_outputFrameData->data[0] = NULL;
		_outputFrameData->linesize[0] = -1;
		_outputFrameData->format = c->pix_fmt;

		ret = av_image_alloc(_outputFrameData->data, _outputFrameData->linesize, c->width, c->height, (AVPixelFormat)_outputFrameData->format, 32);
		if (ret < 0) {
			ofLogError("ofVideoWriter") << "Could not allocate raw _outputFrameData buffer";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated _outputFrameData of size %d (ptr %x), linesize %d %d %d %d\n", ret, _outputFrameData->data[0], _outputFrameData->linesize[0], _outputFrameData->linesize[1], _outputFrameData->linesize[2], _outputFrameData->linesize[3]);
			ofLogNotice("ofVideoWriter") << buf;
		}

		_inputFrameData = av_frame_alloc();
		_inputFrameData->format = inputFormat;

		if ((ret = av_image_alloc(_inputFrameData->data, _inputFrameData->linesize, c->width, c->height, (AVPixelFormat)_inputFrameData->format, 24)) < 0) {
			ofLogError("ofVideoWriter") << "cannot allocate RGB temp image";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated _outputFrameData of size %d (ptr %x), linesize %d %d %d %d\n", ret, _inputFrameData->data[0], _inputFrameData->linesize[0], _inputFrameData->linesize[1], _inputFrameData->linesize[2], _inputFrameData->linesize[3]);
			ofLogNotice("ofVideoWriter") << buf;
		}


		size = ret;
	}

	av_dump_format(oc, 0, filename, 1);
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		int ret;
		if ((ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE)) < 0) {
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			ofLogError("ofVideoWriter") << "Could not open " << filename << buf;
			return;
		}
	}
	/* Write the stream header, if any. */
	int ret = avformat_write_header(oc, NULL);
	if (ret < 0) {
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		ofLogError("ofVideoWriter") << "Error occurred when opening output file: " << buf;
		return;
	}

	/* get sws context for pixel format conversion */
	sws_ctx = sws_getContext(c->width, c->height, (AVPixelFormat)_inputFrameData->format,
		c->width, c->height, (AVPixelFormat)_outputFrameData->format,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!sws_ctx) {
		ofLogError("ofVideoWriter") << "Could not initialize the conversion context";
		return;
	}

	initialized = true;
	b_recording = false;
}

/* add a frame to the video file, default RGB 24bpp format */
void ofVideoWriter::addFrame(const ofPixels pixels) {
	if (b_recording) {
		/* copy the buffer */
		memcpy(_inputFrameData->data[0], pixels.getData(), size);

		/* convert pixel formats */
		sws_scale(sws_ctx, _inputFrameData->data, _inputFrameData->linesize, 0, c->height, _outputFrameData->data, _outputFrameData->linesize);

		int ret = -1;
		if (oc->oformat->flags & AVFMT_RAWPICTURE) {
			/* Raw video case - directly store the picture in the packet */
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index = video_st->index;
			pkt.data = _outputFrameData->data[0];
			pkt.size = sizeof(AVPicture);
			ret = av_interleaved_write_frame(oc, &pkt);
		}
		else {
			AVPacket pkt = { 0 };
			int got_packet;
			av_init_packet(&pkt);
			/* encode the image */
			int ret = avcodec_encode_video2(c, &pkt, _outputFrameData, &got_packet);
			if (ret < 0) {
				char buf[256];
				av_strerror(ret, buf, sizeof(buf));
				ofLogError("ofVideoWriter") << "Error encoding video frame: " << buf;
				return;
			}
			/* If size is zero, it means the image was buffered. */
			if (!ret && got_packet && pkt.size) {
				pkt.stream_index = video_st->index;
				/* Write the compressed frame to the media file. */
				ret = av_interleaved_write_frame(oc, &pkt);
			}
			else {
				ret = 0;
			}
		}
		_outputFrameData->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
		frame_count++;
	}
}
/* add a frame to the video file, RGB 24bpp format */
void ofVideoWriter::addAudioFrame(const float* samples) {
	if (b_recording) {
		AVSampleFormat sample;
		///* copy the buffer */
		//memcpy(_inputFrameData->data[0], pixels, size);

		///* convert RGB24 to YUV420 */
		//sws_scale(sws_ctx, _inputFrameData->data, _inputFrameData->linesize, 0, c->height, _outputFrameData->data, _outputFrameData->linesize);

		//AVPacket pkt = { 0 };
		//int got_packet;
		//av_init_packet(&pkt);
		///* encode the image */
		//int ret = avcodec_encode_audio2(c, &pkt, _outputFrameData, &got_packet);
		//if (ret < 0) {
		//	char buf[256];
		//	av_strerror(ret, buf, sizeof(buf));
		//	ofLogError("ofVideoWriter") << "Error encoding video frame: " << buf;
		//	return;
		//}
		///* If size is zero, it means the image was buffered. */
		//if (!ret && got_packet && pkt.size) {
		//	pkt.stream_index = video_st->index;
		//	/* Write the compressed frame to the media file. */
		//	ret = av_interleaved_write_frame(oc, &pkt);
		//}
		//else {
		//	ret = 0;
		//}
		//_outputFrameData->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
		//frame_count++;
	}
}


void ofVideoWriter::close() {
	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(oc);
	/* Close each codec. */

	avcodec_close(video_st->codec);
	av_freep(&(_outputFrameData->data[0]));
	av_free(_outputFrameData);

	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_close(oc->pb);

	/* free the stream */
	avformat_free_context(oc);
	ofLogNotice("ofVideoWriter") << "closed video file";

	initialized = false;
	b_recording = false;
	frame_count = 0;
}