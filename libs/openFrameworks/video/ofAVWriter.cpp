//
//  ofAVWriter.cpp
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
#include <libswresample/swresample.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avassert.h>
#include <libavutil/timestamp.h>
}
#endif

//#define INBUF_SIZE 4096
//#define AUDIO_INBUF_SIZE 20480
//#define AUDIO_REFILL_THRESH 4096

#include "ofUtils.h"
#include "ofAVWriter.h"

void ofAVWriter::setup(const char * filename, int width, int height, bool recordVideo, bool recordAudio, int framerate, int videoBitRate, int sampleRate, int audioBitRate, int channels, AVPixelFormat inputFormat, AVPixelFormat outputFormat)
{
	video.width = width;
	video.height = height;
	b_encode_video = recordVideo;
	b_encode_audio = recordAudio;
	video.videoBR = videoBitRate;
	video.framerate = framerate;
	audio.sampleRate = sampleRate;
	audio.audioBR = audioBitRate;
	audio.channels = channels;
	video.inFormat = inputFormat;
	video.outFormat = outputFormat;

	prevFrame.allocate(width, height, OF_PIXELS_RGB);

	int ret;

	ofLogNotice("ofAVWriter") << "Video encoding: " << filename;
	/* register all the formats and codecs */
	av_register_all();

	/* allocate the output media context */
	avformat_alloc_output_context2(&oc, NULL, NULL, filename);
	if (!oc) {
		ofLogWarning("ofAVWriter") << "Could not deduce output format from file extension: using MPEG.";
		avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
	}
	if (!oc) {
		ofLogError("ofAVWriter") << "could not create AVFormat context";
		return;
	}
	fmt = oc->oformat;

	/* Add the audio and video streams using the default format codecs
	* and initialize the codecs. */
	if (fmt->video_codec != AV_CODEC_ID_NONE && b_encode_video) {
		add_video_stream(fmt->video_codec);
		has_video = true;
	}
	if (fmt->audio_codec != AV_CODEC_ID_NONE && b_encode_audio) {
		add_audio_stream(fmt->audio_codec);
		has_audio = true;
	}

	if (has_audio)
		open_audio();

	av_dump_format(oc, 0, filename, 1);
	/* open the output file, if needed */
	if (!(fmt->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE)) < 0) {
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			ofLogError("ofAVWriter") << "Could not open " << filename << buf;
			return;
		}
	}
	/* Write the stream header, if any. */
	ret = avformat_write_header(oc, &opt);// NULL);
	if (ret < 0) {
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		ofLogError("ofAVWriter") << "Error occurred when opening output file: " << buf;
		return;
	}

	initialized = true;
	b_recording = false;
}

/* add a frame to the video file, default RGB 24bpp format */
void ofAVWriter::addFrame(const ofPixels pixels) {
	if (b_recording) {
		/* copy the buffer */
		memcpy(video.inFrameData->data[0], pixels.getData(), video.size);

		/* convert pixel formats */
		sws_scale(video.sws_ctx, video.inFrameData->data, video.inFrameData->linesize, 0, video.context->height, video.outFrameData->data, video.outFrameData->linesize);

		int ret = -1;
		if (oc->oformat->flags & AVFMT_RAWPICTURE) {
			/* Raw video case - directly store the picture in the packet */
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index = video.st->index;
			pkt.data = video.outFrameData->data[0];
			pkt.size = sizeof(AVPicture);
			//ret = av_interleaved_write_frame(oc, &pkt);
			write_frame(&video.context->time_base, video.st, &pkt);
		}
		else {
			AVPacket pkt = { 0 };
			int got_packet;
			av_init_packet(&pkt);
			/* encode the image */
			int ret = avcodec_encode_video2(video.context, &pkt, video.outFrameData, &got_packet);
			if (ret < 0) {
				char buf[256];
				av_strerror(ret, buf, sizeof(buf));
				ofLogError("ofAVWriter") << "Error encoding video frame: " << buf;
				return;
			}
			/* If size is zero, it means the image was buffered. */
			//if (!ret && got_packet && pkt.size) {
			if (got_packet) {
				//pkt.stream_index = video.st->index;
				/* Write the compressed frame to the media file. */
				//ret = av_interleaved_write_frame(oc, &pkt);
				ret = write_frame(&video.context->time_base, video.st, &pkt);
			}
			else {
				ret = 0;
			}
		}
		video.outFrameData->pts += av_rescale_q(1, video.st->codec->time_base, video.st->time_base);
		frame_count++;
		prevFrame = pixels;
	}
}
/* add samples to the audio, floating point format */
void ofAVWriter::addAudioSample(const float* samples, int bufferSize, int nChannels) {
	if (b_recording) {
		/*if (has_video &&  av_compare_ts(frame_count, video.st->codec->time_base, sample_count, audio.st->codec->time_base) <= 0) {
			addFrame(prevFrame);
		}
		else {*/
		int ret;
		int dst_nb_samples;
		//* copy the buffer */
		//memcpy(audio.inSamples->data[0], samples, audio.inSamples->nb_samples*audio.inSamples->channels);
		memcpy(audio.inSamples->data[0], samples, bufferSize*nChannels);

		AVPacket pkt = { 0 };
		int got_packet;
		av_init_packet(&pkt);

		//* encode the audio */
		dst_nb_samples = av_rescale_rnd(swr_get_delay(audio.swr_ctx, audio.context->sample_rate) + audio.inSamples->nb_samples,
			audio.context->sample_rate, audio.context->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == audio.inSamples->nb_samples);

		/* when we pass a frame to the encoder, it may keep a reference to it
		* internally;
		* make sure we do not overwrite it here
		*/
		ret = av_frame_make_writable(audio.outSamples);
		if (ret < 0) {
			ofLogWarning("ofAVWriter") << "an error occured when trying to encode audio";
			return;
		}


		/* convert to destination format */
		ret = swr_convert(audio.swr_ctx, audio.outSamples->data, dst_nb_samples, (const uint8_t **)audio.inSamples->data, audio.inSamples->nb_samples);
		if (ret < 0) {
			ofLogWarning("ofAVWriter") << "Error while converting";
			return;
		}

		audio.outSamples->pts = av_rescale_q(audio.samples_count, AVRational{ 1, audio.context->sample_rate }, audio.context->time_base);
		audio.samples_count += dst_nb_samples;

		ret = avcodec_encode_audio2(audio.context, &pkt, audio.outSamples, &got_packet);
		if (ret < 0) {
			char buf[256];
			av_strerror(ret, buf, sizeof(buf));
			ofLogWarning("ofAVWriter") << "Error encoding audio frame: " << buf;
			return;
		}

		if (got_packet) {
			ret = write_frame(&audio.context->time_base, audio.st, &pkt);
			if (ret < 0) {
				char buf[256];
				av_strerror(ret, buf, sizeof(buf));
				ofLogWarning("ofAVWriter") << "Error while writing audio frame : " << buf;
				return;
			}
			sample_count += audio.outSamples->nb_samples;
		}
		//}
	}
}


void ofAVWriter::close() {
	b_recording = false;

	ofLogNotice("ofAVWriter") << "Writing out video - frames: " << frame_count << "samples: " << sample_count;
	ofLogNotice("ofAVWriter") << "Writing out video - video(sec): " << frame_count/(float)video.framerate << "audio(sec): " << sample_count/(float)audio.sampleRate;

	/* Write the trailer, if any. The trailer must be written before you
	 * close the CodecContexts open when you wrote the header; otherwise
	 * av_write_trailer() may try to use memory that was freed on
	 * av_codec_close(). */
	av_write_trailer(oc);
	/* Close each codec. */

	/* Close each codec. */
	if (has_video) {
		avcodec_close(video.st->codec);
		av_frame_free(&video.inFrameData);
		av_frame_free(&video.outFrameData);
		sws_freeContext(video.sws_ctx);
	}
	if (has_audio) {
		avcodec_close(audio.st->codec);
		av_frame_free(&audio.inSamples);
		av_frame_free(&audio.outSamples);
		swr_free(&audio.swr_ctx);
	}

	if (!(fmt->flags & AVFMT_NOFILE))
		/* Close the output file. */
		avio_close(oc->pb);

	/* free the stream */
	avformat_free_context(oc);
	ofLogNotice("ofAVWriter") << "closed video file";

	initialized = false;
	frame_count = 0;
}

void ofAVWriter::add_audio_stream(enum AVCodecID codec_id)
{
	int i;

	/* find the encoder */
	audio.codec = avcodec_find_encoder(codec_id);
	if (!(audio.codec)) {
		ofLogWarning("ofAVWriter") << "Could not find encoder for" << avcodec_get_name(codec_id);
		return;
	}

	audio.st = avformat_new_stream(oc, audio.codec);
	if (!audio.st) {
		ofLogWarning("ofAVWriter") << "Could not allocate stream";
		return;
	}
	audio.st->id = oc->nb_streams - 1;
	audio.context = audio.st->codec;

	audio.context->sample_fmt = (audio.codec)->sample_fmts ?
		(audio.codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
	audio.context->bit_rate = audio.audioBR;
	audio.context->sample_rate = audio.sampleRate;
	if ((audio.codec)->supported_samplerates) {
		audio.context->sample_rate = (audio.codec)->supported_samplerates[0];
		for (i = 0; (audio.codec)->supported_samplerates[i]; i++) {
			if ((audio.codec)->supported_samplerates[i] == audio.sampleRate)
				audio.context->sample_rate = audio.sampleRate;
		}
	}
	audio.context->channels = av_get_channel_layout_nb_channels(audio.context->channel_layout);
	audio.context->channel_layout = AV_CH_LAYOUT_STEREO;
	if ((audio.codec)->channel_layouts) {
		audio.context->channel_layout = (audio.codec)->channel_layouts[0];
		for (i = 0; (audio.codec)->channel_layouts[i]; i++) {
			if ((audio.codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
				audio.context->channel_layout = AV_CH_LAYOUT_STEREO;
		}
	}
	audio.context->channels = av_get_channel_layout_nb_channels(audio.context->channel_layout);
	audio.st->time_base = AVRational{ 1, audio.context->sample_rate };

	/* Some formats want stream headers to be separate. */
	if (oc->oformat->flags & AVFMT_GLOBALHEADER)
		audio.context->flags |= CODEC_FLAG_GLOBAL_HEADER;
}
void ofAVWriter::add_video_stream(enum AVCodecID codec_id)
{
	if (fmt->video_codec != AV_CODEC_ID_NONE) {
		/* find the video encoder */
		AVCodecID avcid = fmt->video_codec;
		video.codec = avcodec_find_encoder(avcid);
		if (!video.codec) {
			ofLogError("ofAVWriter") << "codec not found: " << avcodec_get_name(avcid);
			return;
		}
		else {
			vector<AVPixelFormat> supported_pix_fmt;
			const AVPixelFormat* p = video.codec->pix_fmts;
			while (*p != AV_PIX_FMT_NONE) {
				ofLogNotice("ofAVWriter") << "supported pix fmt: " << av_get_pix_fmt_name(*p);
				supported_pix_fmt.push_back(*p);
				++p;
			}
			bool pixfmtsupported = false;
			for (auto formats : supported_pix_fmt) {
				if (formats == video.outFormat)
					pixfmtsupported = true;
			}
			if (!pixfmtsupported) {
				if (p == NULL || *p == AV_PIX_FMT_NONE) { //if there are no supported pixel formats or the requested format is not one of the supported one
					if (fmt->video_codec == AV_CODEC_ID_RAWVIDEO) {

						video.outFormat = AV_PIX_FMT_RGB24;
					}
					else {
						video.outFormat = AV_PIX_FMT_YUV420P; /* we have to assume the default pix_fmt */
					}
				}
			}
		}

		video.st = avformat_new_stream(oc, video.codec);
		if (!video.st) {
			ofLogWarning("ofAVWriter") << "Could not allocate stream";
			return;
		}
		video.st->id = oc->nb_streams - 1;
		video.context = video.st->codec;
		video.context->codec_id = codec_id;

		video.context->bit_rate = video.videoBR;
		/* Resolution must be a multiple of two. */
		video.context->width = video.width;
		video.context->height = video.height;
		/* timebase: This is the fundamental unit of time (in seconds) in terms
		* of which frame timestamps are represented. For fixed-fps content,
		* timebase should be 1/framerate and timestamp increments should be
		* identical to 1. */
		video.st->time_base = AVRational{ 1, video.framerate };
		video.context->time_base = video.st->time_base;

		video.context->gop_size = 12; /* emit one intra frame every twelve frames at most */

		video.context->pix_fmt = video.outFormat;
		if (video.context->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
			/* just for testing, we also add B frames */
			video.context->max_b_frames = 2;
		}
		if (video.context->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
			/* Needed to avoid using macroblocks in which some coeffs overflow.
			* This does not happen with normal video, it just happens here as
			* the motion of the chroma plane does not match the luma plane. */
			video.context->mb_decision = 2;
		}

		/* open the codec */
		int ret = avcodec_open2(video.context, video.codec, &opt);
		av_dict_free(&opt);
		if (ret < 0) {
			ofLogError("ofAVWriter") << "Could not open codec " << video.codec->long_name;
			return;
		}
		else {
			ofLogNotice("ofAVWriter") << "opened " << avcodec_get_name(fmt->video_codec);
		}

		/* alloc image input and output buffer */
		video.outFrameData = av_frame_alloc();
		video.outFrameData->pts = 0;
		video.outFrameData->width = video.width;
		video.outFrameData->height = video.height;
		video.outFrameData->data[0] = NULL;
		video.outFrameData->linesize[0] = -1;
		video.outFrameData->format = video.context->pix_fmt;

		ret = av_image_alloc(video.outFrameData->data, video.outFrameData->linesize, video.context->width, video.context->height, (AVPixelFormat)video.outFrameData->format, 32);
		if (ret < 0) {
			ofLogError("ofAVWriter") << "Could not allocate raw video.outFrameData buffer";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated frame data of size %d (ptr %x), linesize %d %d %d %d\n", ret, video.outFrameData->data[0], video.outFrameData->linesize[0], video.outFrameData->linesize[1], video.outFrameData->linesize[2], video.outFrameData->linesize[3]);
			ofLogNotice("ofAVWriter") << buf;
		}

		video.inFrameData = av_frame_alloc();
		video.inFrameData->format = video.inFormat;

		if ((ret = av_image_alloc(video.inFrameData->data, video.inFrameData->linesize, video.context->width, video.context->height, (AVPixelFormat)video.inFrameData->format, 24)) < 0) {
			ofLogError("ofAVWriter") << "cannot allocate RGB input frame buffer";
			return;
		}
		else {
			char buf[256];
			sprintf(buf, "allocated frame data of size %d (ptr %x), linesize %d %d %d %d\n", ret, video.inFrameData->data[0], video.inFrameData->linesize[0], video.inFrameData->linesize[1], video.inFrameData->linesize[2], video.inFrameData->linesize[3]);
			ofLogNotice("ofAVWriter") << buf;
		}
		video.size = ret;

		/* get sws context for pixel format conversion */
		video.sws_ctx = sws_getContext(video.context->width, video.context->height, (AVPixelFormat)video.inFrameData->format,
			video.context->width, video.context->height, (AVPixelFormat)video.outFrameData->format,
			SWS_BICUBIC, NULL, NULL, NULL);
		if (!video.sws_ctx) {
			ofLogError("ofAVWriter") << "Could not initialize the conversion context";
			return;
		}

		/* Some formats want stream headers to be separate. */
		if (oc->oformat->flags & AVFMT_GLOBALHEADER)
			video.context->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
}


void ofAVWriter::open_audio()
{
	int nb_samples;
	int ret;

	/* open it */
	ret = avcodec_open2(audio.context, audio.codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		char buf[256];
		av_strerror(ret, buf, sizeof(buf));
		ofLogWarning("ofAVWriter") << "Could not open audio codec: " << buf;
		return;
	}

	if (audio.context->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE)
		nb_samples = 10000;
	else
		nb_samples = audio.context->frame_size;

	audio.outSamples = av_frame_alloc();

	if (!audio.outSamples) {
		ofLogWarning("ofAVWriter") << "Error allocating an audio frame";
		return;
	}

	audio.outSamples->format = audio.context->sample_fmt;
	audio.outSamples->channel_layout = audio.context->channel_layout;
	audio.outSamples->sample_rate = audio.context->sample_rate;
	audio.outSamples->nb_samples = nb_samples;

	/*ret = av_samples_alloc(audio.outSamples->data, audio.outSamples->linesize, audio.context->channels, audio.outSamples->nb_samples, (AVSampleFormat)audio.outSamples->format, 0);
	if (ret < 0) {
		ofLogError("ofAVWriter") << "Could not allocate raw video.outFrameData buffer";
		return;
	}
	else {
		char buf[256];
		sprintf(buf, "allocated frame data of size %d (ptr %x), linesize %d %d %d %d\n", ret, video.outFrameData->data[0], video.outFrameData->linesize[0], video.outFrameData->linesize[1], video.outFrameData->linesize[2], video.outFrameData->linesize[3]);
		ofLogNotice("ofAVWriter") << buf;
	}*/

	if (nb_samples) {
		ret = av_frame_get_buffer(audio.outSamples, 0);
		if (ret < 0) {
			ofLogWarning("ofAVWriter") << "Error allocating an audio buffer";
			return;
		}
	}
	audio.inSamples = av_frame_alloc();

	if (!audio.inSamples) {
		ofLogWarning("ofAVWriter") << "Error allocating an audio frame";
		return;
	}

	audio.inSamples->format = AV_SAMPLE_FMT_FLTP;
	audio.inSamples->channel_layout = audio.context->channel_layout;
	audio.inSamples->sample_rate = audio.context->sample_rate;
	audio.inSamples->nb_samples = nb_samples;

	if (nb_samples) {
		ret = av_frame_get_buffer(audio.inSamples, 0);
		if (ret < 0) {
			ofLogWarning("ofAVWriter") << "Error allocating an audio buffer";
			return;
		}
	}

	/* create resampler context */
	audio.swr_ctx = swr_alloc();
	if (!audio.swr_ctx) {
		ofLogWarning("ofAVWriter") << "Could not allocate resampler context";
		return;
	}

	/* set options */
	av_opt_set_int(audio.swr_ctx, "in_channel_count", audio.channels, 0);
	av_opt_set_int(audio.swr_ctx, "in_sample_rate", audio.sampleRate, 0);
	av_opt_set_sample_fmt(audio.swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
	av_opt_set_int(audio.swr_ctx, "out_channel_count", audio.context->channels, 0);
	av_opt_set_int(audio.swr_ctx, "out_sample_rate", audio.context->sample_rate, 0);
	av_opt_set_sample_fmt(audio.swr_ctx, "out_sample_fmt", audio.context->sample_fmt, 0);

	/* initialize the resampling context */
	if ((ret = swr_init(audio.swr_ctx)) < 0) {
		ofLogWarning("ofAVWriter") << "Failed to initialize the resampling context";
		return;
	}
}

int ofAVWriter::write_frame(const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
	/* rescale output packet timestamp values from codec to stream timebase */
	av_packet_rescale_ts(pkt, *time_base, st->time_base);
	pkt->stream_index = st->index;

	/* Write the compressed frame to the media file. */
	return av_interleaved_write_frame(oc, pkt);
}