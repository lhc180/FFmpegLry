

/*
 * H.264��Ƶͼ���������
 * 1��av_open_input_file������ȡ��Ƶ�ļ����õ�AVFormatContext���ݣ�
 * 	  av_find_stream_info����pFormatCtx�����е�����Ԫ��
 *
 * 2����������pFormatCtx->streams[i]�У�codec_typeΪCODEC_TYPE_VIDEO�����������õ������±�
 *
 * 3��pCodecCtx=pFormatCtx->streams[videoStream]->codec�Լ�
 * 	  avcodec_find_decoder�õ���Ӧ�Ľ�������
 * 	  avcodec_open�򿪽�����
 *
 * 4��ʹ��avcodec_alloc_frameΪ����֡pFrame��pFrameRGB�ֱ����ռ�
 *
 * 5��avpicture_get_size�õ�һ��ͼƬ�����أ�
 *    av_malloc����һ��ͼƬ�Ļ���ռ�
 *    avpicture_fill���������Ļ���󶨵�pFrame
 *
 * 6��AndroidBitmap_getInfo��ȡJAVA��bitmap��������Ϣ
 * 	  AndroidBitmap_lockPixels����ָ��������bitmap����ʹ��ָ�����bitmap
 *
 * 7�����룺
 * 	  av_read_frame��ȡһ֡������
 * 	  avcodec_decode_video2����һ֡���ݣ����ݴ���pFrame��ǰ�᣺if�ж��Ƿ�ΪVideo����
 * 	  sws_getContext��sws_scale����ͼ���ʽת�������ݴ���pFrameRGB
 * 	  fill_bitmap�����ݴ��뵽pixels��ָ���bitmap������
 *
 * 8��av_free_packet�ͷŽ����
 *    AndroidBitmap_unlockPixels�ͷ�ָ��bitmap�����ָ��pixels
 */



#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include <android/bitmap.h>

/**** FFMPEG���е�ͷ�ļ� ****/
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

/**** ����̨��� ****/
#define  LOG_TAG "JNI-lry"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)

/**** ��������е�ȫ�ֱ��� ****/
AVFormatContext *pFormatCtx;	//��������������
AVCodecContext *pCodecCtx;		//����������
AVFrame *pFrame;				//����֡
AVFrame *pFrameRGB;				//����֡תrgb֡
int videoStream;				//��Ƶ����������

/*******************************
 �������ܣ�����FFmpeg��Ŀ�����
 ��	  ��������̨����汾����Ϣ
*******************************/
jstring Java_lry_FFmpegLry_FFmpegLryActivity_GetFFmpegVersion( JNIEnv * env, jobject thiz )
{
    char str[25];
	LOGI("ChangedTimes:10-22");
    sprintf(str,"ver:%d", avcodec_version());
    return(*env)->NewStringUTF(env, str);
}

/**************************************************************
�������ܣ��ǽ�fFrame�е�ͼ��������ĳ�ַ�����䵽pixels��ָ��������
��          �룺info--bitmap�������Ϣ
		 pixels--��ָ�룬����bitmap�ľ��
		 pFrame--����bitmap��data������Դ
��          �⣺�����о���Ķ���������
****************************************************************/
static void fill_bitmap(AndroidBitmapInfo*  info, void *pixels, AVFrame *pFrame)
{
    uint8_t *frameLine;

    int  yy;
    for (yy = 0; yy < info->height; yy++) {
        uint8_t*  line = (uint8_t*)pixels;
        frameLine = (uint8_t *)pFrame->data[0] + (yy * pFrame->linesize[0]);

        int xx;
        for (xx = 0; xx < info->width; xx++) {
            int out_offset = xx * 4;
            int in_offset = xx * 3;

            line[out_offset] = frameLine[in_offset];
            line[out_offset+1] = frameLine[in_offset+1];
            line[out_offset+2] = frameLine[in_offset+2];
            line[out_offset+3] = 0;
        }
        pixels = (char*)pixels + info->stride;
    }
}

/**************************************
 �������ܣ����ļ�����ý�����Ƶ������Դ
 ��          ע��EnvConfigure���ú���
****************************************/
void OpenFile()
{
	int err;
	//���뷽ʽ���ļ�����ȡ��Ƶ��pFormatCtx
	//hav-test.mp4   code_id:13   mpeg4����
	//240x320.h264   code_id:28   h264
	//352x288.264    code_id:28   h264
//	err = av_open_input_file(&pFormatCtx, "file:/mnt/sdcard/video/352x288.264", NULL, 0, NULL);
	err = av_open_input_file(&pFormatCtx, "file:/storage/sdcard0/external_sdcard/video/352x288.264", NULL, 0, NULL);
	LOGD("Called open file");
	if(err!=0) {
		LOGE("Couldn't open file");
		return;
	}
	LOGD("Opened file");
}

/********************************
 �������ܣ��򿪽�����
 ��          ע��EnvConfigure���ú���
*********************************/
void OpenDecoder(AVCodec *pCodecOD)
{
	int i;
	//������Ƶ����Ϣ
	if(av_find_stream_info(pFormatCtx)<0) {
		LOGE("Unable to get stream info");
		return ;
	}

	//�ҵ�streams��codec_typeΪVIDEO���͵������±�videoStream
	videoStream = -1;
	for (i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if(videoStream==-1) {
		LOGE("Unable to find video stream");
		return;
	}
	LOGD("Video stream is [%d]", videoStream);

	//��ȡ��Ƶ����Ӧ�Ľ�����
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;

	pCodecOD=avcodec_find_decoder(pCodecCtx->codec_id);	//lry
	LOGD("CODEC_ID:%d",pCodecCtx->codec_id);
	//pCodec=avcodec_find_decoder(CODEC_ID_H264);	//lry
	if(pCodecOD==NULL) {
		LOGE("Unsupported codec");
		return;
	}

	//�򿪽�����
	if(avcodec_open(pCodecCtx, pCodecOD)<0) {
		LOGE("Unable to open codec");
		return;
	}

}

/**************************************************
 �������ܣ�����������õ��Ļ�������ڴ������ڴ��ʼ��
 ��          ע��EnvConfigure���ú���
***************************************************/
void MemInit()
{
	int numBytes;
	uint8_t *buffer;

	 //Ϊ����֡����ռ�
	pFrame=avcodec_alloc_frame();
	pFrameRGB=avcodec_alloc_frame();
	LOGD("Video size is [%d x %d]", pCodecCtx->width, pCodecCtx->height);

	//���ؿ�͸ߵĳ˻����Դ˿���ͼƬ֡����ռ�
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	//ʹ��buffer�󶨵�pFrameRGB����
	//pFrameRGB->data[0]=buffer;
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);
}

/****************************
 �������ܣ����뻷������׼������
****************************/
void Java_lry_FFmpegLry_FFmpegLryActivity_EnvConfigure(JNIEnv * env, jobject this)
{
	int i;
	AVCodec *pCodec;

	//ע�����еı���������������Э��ȵ�
    av_register_all();
    LOGD("Registered formats");

    //�򿪴������ļ�
	OpenFile();

	//�򿪽�����
	OpenDecoder(pCodec);

	//����������õ��Ļ�������ڴ������ڴ��ʼ��
	MemInit();
}

/**********************************
�������ܣ������������ڴ����
**********************************/
void Java_lry_FFmpegLry_FFmpegLryActivity_EnvFree(JNIEnv * env, jobject this)
{
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	LOGD("Memory free success");
}

/*************************************************************
�������ܣ�����H.264��Ƶ��һ֡����ˢ��bitmap����
�������̣� 1����ȡbitmap��Info
 	 	  2������bitmap�ĵ�ַָ��
 	 	  3���õ�һ֡��ƵͼƬ
 	 	  4��avcodec_decode_video2���뵽pFrame
 	 	  5��sws_getContext��sws_scale��yuvͼ��ת��Ϊrgbͼ��
**************************************************************/
void Java_lry_FFmpegLry_FFmpegLryActivity_drawFrame(JNIEnv * env, jobject this, jstring bitmap)
{
    AndroidBitmapInfo  info;
    void*              pixels;
    int                ret;


    int err;
    int i;
    int frameFinished = 0;
    AVPacket packet;
    static struct SwsContext *img_convert_ctx;
    int64_t seek_target;


//��ȡbitmap��Ϣ
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }
    LOGD("Checked on the bitmap");

//����pixels��ַ,ʹ��pixels������bitmap
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }
    LOGD("Grabbed the pixels");

    i = 0;
	//ʹ��while i=0 ��frameFinished����������֤�ܹ�����һ֡,ֱ����󣬲���ʾ����
    //��ȡһ֡ͼ��,����һ֡
    while((i==0) && (av_read_frame(pFormatCtx, &packet)>=0)) {
    	//��Ƶ������
  		if(packet.stream_index==videoStream) {
			LOGD("Now:packet pts %llu", packet.pts);

			//��������ݴ���pFrame�У�pCodecCtx��frame_number��һ
			//pFrame��pFrameRGB����������һģһ��
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
    
    		if(frameFinished) {
                LOGD("packet pts %llu", packet.pts);

                int target_width = 320;
                int target_height = 240;
				
				//��Ƶ�ߴ�ת��
				img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                       pCodecCtx->pix_fmt, 
                       target_width, target_height, PIX_FMT_RGB24, SWS_BICUBIC, 
                       NULL, NULL, NULL);
                if(img_convert_ctx == NULL) {
                    LOGE("could not initialize conversion context\n");
                    return;
                }
				//yuvͼ��ת��Ϊrgbͼ��
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                // save_frame(pFrameRGB, target_width, target_height, i);
                fill_bitmap(&info, pixels, pFrameRGB);
                i = 1;
    	    }
        }
  		//�ͷ�packet
        av_free_packet(&packet);
    }
    //�ͷ�Pixelsָ��
    AndroidBitmap_unlockPixels(env, bitmap);
}


/********************
 *   �����ο�˵��          *
 ********************/

//��Ƶ�ߴ�ת��
/*
 * Allocates and returns a SwsContext. You need it to perform
 * scaling/conversion operations using sws_scale().
 *
 * @param srcW the width of the source image
 * @param srcH the height of the source image
 * @param srcFormat the source image format
 * @param dstW the width of the destination image
 * @param dstH the height of the destination image
 * @param dstFormat the destination image format
 * @param flags specify which algorithm and options to use for rescaling
 * @return a pointer to an allocated context, or NULL in case of error
 */
//img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,pCodecCtx->pix_fmt,target_width, target_height, PIX_FMT_RGB24, SWS_BICUBIC,NULL, NULL, NULL);


//yuvͼ��ת��Ϊrgbͼ��
/*
 * Scales the image slice in srcSlice and puts the resulting scaled
 * slice in the image in dst. A slice is a sequence of consecutive
 * rows in an image.
 *
 * Slices have to be provided in sequential order, either in
 * top-bottom or bottom-top order. If slices are provided in
 * non-sequential order the behavior of the function is undefined.
 *
 * @param context   the scaling context previously created with
 *                  sws_getContext()
 * @param srcSlice  the array containing the pointers to the planes of
 *                  the source slice
 * @param srcStride the array containing the strides for each plane of
 *                  the source image
 * @param srcSliceY the position in the source image of the slice to
 *                  process, that is the number (counted starting from
 *                  zero) in the image of the first row of the slice
 * @param srcSliceH the height of the source slice, that is the number
 *                  of rows in the slice
 * @param dst       the array containing the pointers to the planes of
 *                  the destination image
 * @param dstStride the array containing the strides for each plane of
 *                  the destination image
 * @return          the height of the output slice
 */
//sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
