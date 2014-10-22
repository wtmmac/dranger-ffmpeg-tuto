/**
 * 基于FFmpeg的内存读取
 *
 * 基于雷霄骅的Simplest FFmpeg mem Player版本进行修改,感谢原作者
 *
 * 王天明(wtmmac@gmail.com)
 * Blog:http://my.oschina.net/tonywang
 *
 */

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

// new version
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/mathematics.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

//Output YUV420P
#define OUTPUT_YUV420P 0
FILE *fp_open = NULL;

//Callback
int read_buffer(void *opaque, uint8_t *buf, int buf_size) {
	if (!feof(fp_open)) {
		int true_size = fread(buf, 1, buf_size, fp_open);
		return true_size;
	} else {
		return -1;
	}

}

// important
int64_t seek_func(void *opaque, int64_t offset, int whence) {
	FILE *fp = (FILE*) opaque;
	if (whence == AVSEEK_SIZE) {
		return -1;
	}
	fseek(fp, offset, whence);
	return ftell(fp);
}

unsigned char buff[5000];
int bufflen;
int pos;

/*
 int read_packet(void *opaque, uint8_t *buf, int buf_size)
 {
 int size = buf_size;
 if (bufflen - pos < buf_size)
 size = bufflen-pos;
 if (size > 0)
 {
 memcpy(buf, buff+pos, size);
 pos += size;
 }
 return size;
 }
 int64_t seek(void* opaque, int64_t offset, int whence)
 {
 switch (whence)
 {
 case SEEK_SET:
 pos = offset;
 break;
 case SEEK_CUR:
 pos += offset;
 break;
 case SEEK_END:
 pos = bufflen-offset;
 break;
 case AVSEEK_SIZE:
 return bufflen;
 break;
 }
 return pos;
 }
 */
int main(int argc, char* argv[]) {

	AVFormatContext *pFormatCtx;

	int ret1 = 0; //////
	AVInputFormat *piFmt = NULL; //ttttt

	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	char filepath[] = "haoshengyin720.mp4";
	printf("%s\n", filepath);

	av_register_all();
	avformat_network_init();

	fp_open = fopen(filepath, "rb+");

	//AVIOContext中的缓存
	unsigned char *avio_buffer = (unsigned char *) av_malloc(32768);
	AVIOContext *avio_ctx = avio_alloc_context(avio_buffer, 32768, 0, NULL, read_buffer, NULL, NULL);

    // important
	avio_ctx->seekable = 0;
	// printf("can seek:%d\n", avio_ctx->seekable);

	//step2:探测流格式///////////
	ret1 = av_probe_input_buffer(avio_ctx, &piFmt, "", NULL, 0, 0);
	if (ret1 < 0) {
		fprintf(stderr, "probe failed!\n");
		//goto quit;
	} else {
		fprintf(stdout, "probe success!\n");
		fprintf(stdout, "format: %s[%s]\n", piFmt->name, piFmt->long_name);
	}

	pFormatCtx = avformat_alloc_context();
	pFormatCtx->pb = avio_ctx;

	pFormatCtx->flags = AVFMT_FLAG_CUSTOM_IO;

	//return 0;//////////////////////
	// filename 留空非NULL
	if (avformat_open_input(&pFormatCtx, "", piFmt, NULL) != 0) {
		printf("Couldn't open input stream.（无法打开输入流）\n");
		return -1;
	}
	//return 0;//////////////////////
	//if(av_find_stream_info(pFormatCtx)<0){  旧的函数
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.（无法获取流信息）\n");
		return -1;
	}

	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.（没有找到视频流）\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.（没有找到解码器）\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.（无法打开解码器）\n");
		return -1;
	}

	AVFrame *pFrame, *pFrameYUV;
	pFrame = avcodec_alloc_frame();
	pFrameYUV = avcodec_alloc_frame();

	uint8_t *out_buffer = (uint8_t *) av_malloc(
			avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width,
					pCodecCtx->height));
	avpicture_fill((AVPicture *) pFrameYUV, out_buffer, PIX_FMT_YUV420P,
			pCodecCtx->width, pCodecCtx->height);

	//SDL----------------------------
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	int screen_w = 0, screen_h = 0;
	SDL_Surface *screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);

	if (!screen) {
		printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
		return -1;
	}

	SDL_Overlay *bmp;
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,
	SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	//SDL End------------------------
	int ret, got_picture;

	AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
	//Output Information-----------------------------
	printf("File Information---------------------------------\n");
	av_dump_format(pFormatCtx, 0, filepath, 0);
	printf("-------------------------------------------------\n");

#if OUTPUT_YUV420P
	FILE *fp_yuv=fopen("output.yuv","wb+");
#endif

	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
			pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
			PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	//------------------------------
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
					packet);
			if (ret < 0) {
				printf("Decode Error.（解码错误）\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx,
						(const uint8_t* const *) pFrame->data, pFrame->linesize,
						0, pCodecCtx->height, pFrameYUV->data,
						pFrameYUV->linesize);
#if OUTPUT_YUV420P
				int y_size=pCodecCtx->width*pCodecCtx->height;
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);//U
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);//V
#endif
				SDL_LockYUVOverlay(bmp);

				pFrameYUV->data[0] = bmp->pixels[0];
				pFrameYUV->data[2] = bmp->pixels[2];
				pFrameYUV->data[1] = bmp->pixels[1];
				pFrameYUV->linesize[0] = bmp->pitches[0];
				pFrameYUV->linesize[2] = bmp->pitches[2];
				pFrameYUV->linesize[1] = bmp->pitches[1];

				SDL_UnlockYUVOverlay(bmp);
				rect.x = 0;
				rect.y = 0;
				rect.w = screen_w;
				rect.h = screen_h;
				SDL_DisplayYUVOverlay(bmp, &rect);
				//Delay 40ms
				SDL_Delay(40);
			}
		}
		av_free_packet(packet);
	}
	sws_freeContext(img_convert_ctx);

#if OUTPUT_YUV420P
	fclose(fp_yuv);
#endif

	fclose(fp_open);

	SDL_Quit();

	av_free(out_buffer);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
