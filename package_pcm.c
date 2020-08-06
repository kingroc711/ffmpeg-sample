#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>


static void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *output){
    int ret;

    /* send the frame for encoding */
    ret = avcodec_send_frame(ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
        exit(1);
    }

    /* read all the available output packets (in general there may be any
     * number of them */
    while (ret >= 0) {
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }

        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
}

int main(int argc, char **argv)
{
    const char *in_filename = "audiofile.pcm";
	const char *out_filename = "audiofile.wav";
	AVCodec *audio_codec;
	AVCodecContext *audio_codec_context = NULL;
	AVFrame *frame;
    AVPacket *pkt;
    int ret;
    FILE *out_file, *in_file;

    audio_codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    if (!audio_codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    audio_codec_context = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_context) {
        fprintf(stderr, "Could not allocate audio codec context\n");
        exit(1);
    }

    /* put sample parameters */
    audio_codec_context->bit_rate = 64000;

    /* check that the encoder supports s16 pcm input */
    audio_codec_context->sample_fmt = AV_SAMPLE_FMT_S16;
    audio_codec_context->sample_rate    = 48000;
    audio_codec_context->channels       = 1;
    audio_codec_context->channel_layout = av_get_default_channel_layout(1);
    
	/* open it */
    if (avcodec_open2(audio_codec_context, audio_codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    out_file = fopen(out_filename, "wb");
    if (!out_file) {
        fprintf(stderr, "Could not open %s\n", out_filename);
        exit(1);
    }


    in_file = fopen(in_filename, "rb");
    if (!in_file) {
        fprintf(stderr, "Could not open %s\n", in_filename);
        exit(1);
    }

    /* packet for holding encoded output */
    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "could not allocate the packet\n");
        exit(1);
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame->nb_samples     = 640*2;
    frame->format         = audio_codec_context->sample_fmt;
    frame->channel_layout = audio_codec_context->channel_layout;

    /* allocate the data buffers */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }
	
	int perFrameDataSize = frame->linesize[0];
	printf("DateSize : %d\n", perFrameDataSize);
	size_t pcm_buf_len = perFrameDataSize;
	unsigned char pcm_buf[pcm_buf_len];
	while(fread(pcm_buf, 1, pcm_buf_len, in_file) == pcm_buf_len){
        ret = av_frame_make_writable(frame);
        if (ret < 0){
			printf("frame can not write \n");
            exit(1);
		}
		memset(frame->data[0], 0, perFrameDataSize);
		memcpy(frame->data[0], pcm_buf, pcm_buf_len);
        encode(audio_codec_context, frame, pkt, out_file);
	}

    /* flush the encoder */
    encode(audio_codec_context, NULL, pkt, out_file);

    fclose(out_file);
    fclose(in_file);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&audio_codec_context);

    return 0;
}
