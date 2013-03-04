

/*
 * H.264视频图像解码流程
 * 1、av_open_input_file函数获取视频文件，得到AVFormatContext数据，
 * 	  av_find_stream_info更新pFormatCtx数据中的所有元素
 *
 * 2、遍历查找pFormatCtx->streams[i]中，codec_type为CODEC_TYPE_VIDEO的数据流，得到索引下标
 *
 * 3、pCodecCtx=pFormatCtx->streams[videoStream]->codec以及
 * 	  avcodec_find_decoder得到对应的解码器，
 * 	  avcodec_open打开解码器
 *
 * 4、使用avcodec_alloc_frame为解码帧pFrame和pFrameRGB分别分配空间
 *
 * 5、avpicture_get_size得到一幅图片的像素，
 *    av_malloc分配一幅图片的缓存空间
 *    avpicture_fill将上面分配的缓存绑定到pFrame
 *
 * 6、AndroidBitmap_getInfo获取JAVA层bitmap变量的信息
 * 	  AndroidBitmap_lockPixels将空指针锁定到bitmap对象，使用指针操作bitmap
 *
 * 7、解码：
 * 	  av_read_frame读取一帧的数据
 * 	  avcodec_decode_video2解码一帧数据，数据存入pFrame，前提：if判断是否为Video数据
 * 	  sws_getContext和sws_scale进行图像格式转换，数据存入pFrameRGB
 * 	  fill_bitmap将数据存入到pixels所指向的bitmap对象中
 *
 * 8、av_free_packet释放解码包
 *    AndroidBitmap_unlockPixels释放指向bitmap对象的指针pixels
 */



#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <android/log.h>
#include <android/bitmap.h>

/**** FFMPEG库中的头文件 ****/
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

/**** 控制台输出 ****/
#define  LOG_TAG "JNI-lry"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)

/**** 解码过程中的全局变量 ****/
AVFormatContext *pFormatCtx;	//输入流控制容器
AVCodecContext *pCodecCtx;		//解码器容器
AVFrame *pFrame;				//解码帧
AVFrame *pFrameRGB;				//解码帧转rgb帧
int videoStream;				//视频流的索引号

/*******************************
 函数功能：测试FFmpeg库的可用性
 输	  出：控制台输出版本号信息
*******************************/
jstring Java_lry_FFmpegLry_FFmpegLryActivity_GetFFmpegVersion( JNIEnv * env, jobject thiz )
{
    char str[25];
	LOGI("ChangedTimes:10-22");
    sprintf(str,"ver:%d", avcodec_version());
    return(*env)->NewStringUTF(env, str);
}

/**************************************************************
函数功能：是将fFrame中的图像内容以某种方法填充到pixels所指定的区域
输          入：info--bitmap对象的信息
		 pixels--空指针，操作bitmap的句柄
		 pFrame--填入bitmap的data的数据源
问          题：对其中具体的东西并不懂
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
 函数功能：打开文件，获得解码视频的数据源
 备          注：EnvConfigure调用函数
****************************************/
void OpenFile()
{
	int err;
	//输入方式打开文件，获取视频流pFormatCtx
	//hav-test.mp4   code_id:13   mpeg4编码
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
 函数功能：打开解码器
 备          注：EnvConfigure调用函数
*********************************/
void OpenDecoder(AVCodec *pCodecOD)
{
	int i;
	//查找视频流信息
	if(av_find_stream_info(pFormatCtx)<0) {
		LOGE("Unable to get stream info");
		return ;
	}

	//找到streams中codec_type为VIDEO类型的流，下标videoStream
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

	//获取视频流对应的解码器
	pCodecCtx=pFormatCtx->streams[videoStream]->codec;

	pCodecOD=avcodec_find_decoder(pCodecCtx->codec_id);	//lry
	LOGD("CODEC_ID:%d",pCodecCtx->codec_id);
	//pCodec=avcodec_find_decoder(CODEC_ID_H264);	//lry
	if(pCodecOD==NULL) {
		LOGE("Unsupported codec");
		return;
	}

	//打开解码器
	if(avcodec_open(pCodecCtx, pCodecOD)<0) {
		LOGE("Unable to open codec");
		return;
	}

}

/**************************************************
 函数功能：解码过程中用到的缓存变量内存分配和内存初始化
 备          注：EnvConfigure调用函数
***************************************************/
void MemInit()
{
	int numBytes;
	uint8_t *buffer;

	 //为解码帧分配空间
	pFrame=avcodec_alloc_frame();
	pFrameRGB=avcodec_alloc_frame();
	LOGD("Video size is [%d x %d]", pCodecCtx->width, pCodecCtx->height);

	//返回宽和高的乘积，以此开辟图片帧缓存空间
	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

	//使用buffer绑定到pFrameRGB区域
	//pFrameRGB->data[0]=buffer;
	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
			pCodecCtx->width, pCodecCtx->height);
}

/****************************
 函数功能：解码环境配置准备工作
****************************/
void Java_lry_FFmpegLry_FFmpegLryActivity_EnvConfigure(JNIEnv * env, jobject this)
{
	int i;
	AVCodec *pCodec;

	//注册所有的编码器、解码器、协议等等
    av_register_all();
    LOGD("Registered formats");

    //打开待解码文件
	OpenFile();

	//打开解码器
	OpenDecoder(pCodec);

	//解码过程中用到的缓存变量内存分配和内存初始化
	MemInit();
}

/**********************************
函数功能：解码结束后的内存回收
**********************************/
void Java_lry_FFmpegLry_FFmpegLryActivity_EnvFree(JNIEnv * env, jobject this)
{
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	LOGD("Memory free success");
}

/*************************************************************
函数功能：解码H.264视频的一帧，并刷新bitmap对象
解码流程： 1、获取bitmap的Info
 	 	  2、锁定bitmap的地址指针
 	 	  3、得到一帧视频图片
 	 	  4、avcodec_decode_video2解码到pFrame
 	 	  5、sws_getContext和sws_scale将yuv图像转换为rgb图像
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


//获取bitmap信息
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }
    LOGD("Checked on the bitmap");

//锁定pixels地址,使用pixels来操作bitmap
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }
    LOGD("Grabbed the pixels");

    i = 0;
	//使用while i=0 和frameFinished两个条件保证能够解码一帧,直到最后，并显示出来
    //读取一帧图像,解码一帧
    while((i==0) && (av_read_frame(pFormatCtx, &packet)>=0)) {
    	//视频流解码
  		if(packet.stream_index==videoStream) {
			LOGD("Now:packet pts %llu", packet.pts);

			//解码后数据存入pFrame中，pCodecCtx中frame_number加一
			//pFrame和pFrameRGB的数据类型一模一样
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
    
    		if(frameFinished) {
                LOGD("packet pts %llu", packet.pts);

                int target_width = 320;
                int target_height = 240;
				
				//视频尺寸转换
				img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                       pCodecCtx->pix_fmt, 
                       target_width, target_height, PIX_FMT_RGB24, SWS_BICUBIC, 
                       NULL, NULL, NULL);
                if(img_convert_ctx == NULL) {
                    LOGE("could not initialize conversion context\n");
                    return;
                }
				//yuv图像转换为rgb图像
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                // save_frame(pFrameRGB, target_width, target_height, i);
                fill_bitmap(&info, pixels, pFrameRGB);
                i = 1;
    	    }
        }
  		//释放packet
        av_free_packet(&packet);
    }
    //释放Pixels指针
    AndroidBitmap_unlockPixels(env, bitmap);
}


/********************
 *   函数参考说明          *
 ********************/

//视频尺寸转换
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


//yuv图像转换为rgb图像
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
