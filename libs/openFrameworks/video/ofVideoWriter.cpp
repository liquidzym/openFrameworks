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

void ofVideoWriter::setup(const char* filename, int width, int height) {
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
	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		/* find the video encoder */
		AVCodecID avcid = fmt->video_codec;
		codec = avcodec_find_encoder(avcid);
		if (!codec) {
			ofLogError("ofVideoWriter") << "codec not found: " << avcodec_get_name(avcid);
			return;
		}
		else {
			const AVPixelFormat* p = codec->pix_fmts;
			while (*p != AV_PIX_FMT_NONE) {
				ofLogNotice("ofVideoWriter") << "supported pix fmt: " << av_get_pix_fmt_name(*p);
				++p;
			}
		}

		video_st = avformat_new_stream(oc, codec);
		if (!video_st) {
			ofLogError("ofVideoWriter") << "Could not allocate stream";
			return;
		}
		video_st->id = oc->nb_streams - 1;
		video_st->time_base = AVRational{ 1, 25 };
		c = video_st->codec;

		/* Some formats want stream headers to be separate. */
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			c->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	/* Now that all the parameters are set, we can open the audio and
	 * video codecs and allocate the necessary encode buffers. */
	{

		picture = av_frame_alloc();//avcodec_alloc_frame();
		picture->pts = 0;
		picture->width = width;
		picture->height = height;

		/* put sample parameters */
		c->bit_rate = 400000;
		/* resolution must be a multiple of two */
		c->width = width;
		c->height = height;
		/* frames per second */
		c->time_base = AVRational{ 1, 25 };
		c->gop_size = 10; /* emit one intra frame every ten frames */
		c->max_b_frames = 1;
		c->pix_fmt = AV_PIX_FMT_YUV420P;

		/* open it */
		AVDictionary* options = NULL;
		if (avcodec_open2(c, codec, &options) < 0) {
			ofLogError("ofVideoWriter") << "could not open codec";
			return;
		}
		else {
			ofLogNotice("ofVideoWriter") << "opened " << avcodec_get_name(fmt->video_codec);
		}

		/* alloc image and output buffer */
		picture->data[0] = NULL;
		picture->linesize[0] = -1;
		picture->format = c->pix_fmt;

		int ret = -1;
		ret = av_image_alloc(picture->data, picture->linesize, c->width, c->height, (AVPixelFormat)picture->format, 32);
		if (ret < 0) {
			ofLogError("ofVideoWriter") << "Could not allocate raw picture buffer";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, picture->data[0], picture->linesize[0], picture->linesize[1], picture->linesize[2], picture->linesize[3]);
			ofLogNotice("ofVideoWriter") << buf;
		}

		picture_rgb24 = av_frame_alloc();//avcodec_alloc_frame();
		picture_rgb24->format = AV_PIX_FMT_RGB24;

		if ((ret = av_image_alloc(picture_rgb24->data, picture_rgb24->linesize, c->width, c->height, (AVPixelFormat)picture_rgb24->format, 24)) < 0) {
			ofLogError("ofVideoWriter") << "cannot allocate RGB temp image";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, picture_rgb24->data[0], picture_rgb24->linesize[0], picture_rgb24->linesize[1], picture_rgb24->linesize[2], picture_rgb24->linesize[3]);
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

	/* get sws context for RGB24 -> YUV420 conversion */
	sws_ctx = sws_getContext(c->width, c->height, (AVPixelFormat)picture_rgb24->format,
		c->width, c->height, (AVPixelFormat)picture->format,
		SWS_BICUBIC, NULL, NULL, NULL);
	if (!sws_ctx) {
		ofLogError("ofVideoWriter") << "Could not initialize the conversion context";
		return;
	}

	initialized = true;
	b_recording = false;
}

/* add a frame to the video file, RGB 24bpp format */
void ofVideoWriter::addFrame(const uint8_t* pixels) {
	if (b_recording) {
		/* copy the buffer */
		memcpy(picture_rgb24->data[0], pixels, size);

		/* convert RGB24 to YUV420 */
		sws_scale(sws_ctx, picture_rgb24->data, picture_rgb24->linesize, 0, c->height, picture->data, picture->linesize);

		AVPacket pkt = { 0 };
		int got_packet;
		av_init_packet(&pkt);
		/* encode the image */
		int ret = avcodec_encode_video2(c, &pkt, picture, &got_packet);
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
		picture->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
		frame_count++;
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
	av_freep(&(picture->data[0]));
	av_free(picture);

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
