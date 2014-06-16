#include <jni.h>
#include <android/log.h>
#include "common.h"
#include "string.h"
#include "android/bitmap.h"
#include "allheaders.h"
#include "text_search.h"
#include <sstream>
#include <iostream>
#include <sstream>
#include <GLES/gl.h>

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef max
#define max(a,b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#define min(a,b) ({typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#endif


jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv *env;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		LOGE("Failed to get the environment using GetEnv()");
		return -1;
	}

	return JNI_VERSION_1_6;
}

const int bytes_per_pixel = 2;
Pix* mPix32 = NULL;
Pix* mPixDest = NULL;
Pix* mPix8 = NULL;
Pix* mPix8d = NULL;
Pix* mPix8Black = NULL;
l_int32 wpl32;
Numa* numa = NULL;

void Java_de_renard_camerapreview_ImageFilter_nativeStart(JNIEnv *env, jobject javathis, jint w, jint h) {
	mPix32 = pixCreate(w, h, 32);
	mPix8 = pixCreate(w, h, 8);
	wpl32 = pixGetWpl(mPix32);
	mPix8Black = pixCreate(w,h,8);
	pixSetAllArbitrary(mPix8Black,255);
}
void Java_de_renard_camerapreview_ImageFilter_nativeEnd(JNIEnv *env, jobject javathis) {
	pixDestroy(&mPix32);
	pixDestroy(&mPix8);
	pixDestroy(&mPix8Black);
}


static inline void yuvToPixFast(unsigned char* pY, Pix* pix32, Pix* pix8) {
	int i, j;
	int nR, nG, nB;
	int nY, nU, nV;
	l_uint32* data = pixGetData(pix32);
	l_uint32* data8 = pixGetData(pix8);
	l_int32 height = pixGetHeight(pix32);
	l_int32 width = pixGetWidth(pix32);
	l_int32 wpl = pixGetWpl(pix32);
	l_int32 wpl8 = pixGetWpl(pix8);
	l_uint8 **lineptrs = pixSetupByteProcessing(pix8, NULL, NULL);
	l_uint8* line8;

	unsigned char* pUV = pY + width * height;

	for (i = 0; i < height; i++) {
		nU = 0;
		nV = 0;
		unsigned char* uvp = pUV + (i >> 1) * width;
		line8 = lineptrs[i];
		memcpy(line8, pY, wpl8 * 4);

		for (j = 0; j < width; j++) {

			if ((j & 1) == 0) {
				nV = (0xff & *uvp++) - 128;
				nU = (0xff & *uvp++) - 128;
			}
			// Yuv Convert
			nY = *(pY++);
			nY -= -16;

			if (nY < 0) {
				nY = 0;
			}
			int y1192 = nY * 1192;

			nB = y1192 + 2066 * nU;
			nG = y1192 - 833 * nV - 400 * nU;
			nR = y1192 + 1634 * nV;

			if (nR < 0) {
				nR = 0;
			} else if (nR > 262143) {
				nR = 262143;
			}
			if (nG < 0) {
				nG = 0;
			} else if (nG > 262143) {
				nG = 262143;
			}
			if (nB < 0) {
				nB = 0;
			} else if (nB > 262143) {
				nB = 262143;
			}
			*data++ = ((nR << 14) & 0xff000000) | ((nG << 6) & 0xff0000) | ((nB >> 2) & 0xff00) | (0xff);
		}
	}
	pixCleanupByteProcessing(pix8, lineptrs);
}
static void printGLString(const char *name, GLenum s) {
	const char *v = (const char *) glGetString(s);
	LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGE("after %s() glError (0x%x)\n", op, error);
	}
}
/*


void Java_de_renard_android_PreviewRenderer_nativeLoadTexture(JNIEnv *env, jobject javathis, jint offsetx, jint offsety, jint textureName) {
	GLuint texture = (GLuint) textureName;
	PIX *pix = mPix32;

	l_int32 w, h, d;
	pixGetDimensions(pix, &w, &h, &d);

	l_uint32* data = NULL;
	Pix* pix32 = NULL;
	if (d==32){
		pix32 = pixCopy(NULL,pix);
		pixSetRGBComponent(pix32,mPix8Black,L_ALPHA_CHANNEL);
		pixEndianByteSwap(pix32);
		data= pixGetData(pix32);

	} else if (d == 8) {
		pix32 = pixConvert8To32MaxAlpha(pix);
		data = pixGetData(pix32);
	} else if (d == 1) {
		pix32 = pixConvert1To32(NULL, pix, 0xffffffff, 0xff000000);
		data = pixGetData(pix32);
	}
	glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)offsetx, (GLint)offsety, w, h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) data);
	if (pix32 != NULL) {
		pixDestroy(&pix32);
	}
	checkGlError("glTexSubImage2D");
}

void JJava_de_renard_android_PreviewRenderer_nativeProcessImage(JNIEnv *env, jobject javathis, jbyteArray frame, jint orientation, jint filterType) {

	ostringstream debugstring;
	startTimer();
	jbyte *data_buffer = env->GetByteArrayElements(frame, NULL);
	l_uint8 *byte_buffer = (l_uint8 *) data_buffer;
	yuvToPixFast(byte_buffer, mPix32, mPix8);
	env->ReleaseByteArrayElements(frame, data_buffer, JNI_ABORT);
	debugstring << "convert to RGB: " << stopTimer() << std::endl;
}
*/

void writeBitmap(JNIEnv *env, Pix* pixs, jobject bitmap) {

	l_int32 w, h, d;
	AndroidBitmapInfo info;
	void* pixels;
	int ret;

	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}

	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return;
	}

	pixGetDimensions(pixs, &w, &h, &d);

	if (w != info.width || h != info.height) {
		LOGE("Bitmap width and height do not match Pix dimensions!");
		LOGE("Pix(%i,%i)",w,h);
		LOGE("Bitmap(%i,%i)",info.width,info.height);
		return;
	}

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		return;
	}

	pixEndianByteSwap(pixs);

	l_uint8 *dst = (l_uint8 *) pixels;
	l_uint8 *src = (l_uint8 *) pixGetData(pixs);
	l_int32 dstBpl = info.stride;
	l_int32 srcBpl = 4 * pixGetWpl(pixs);

	for (int dy = 0; dy < info.height; dy++) {
		l_uint8 *dstx = dst;
		l_uint8 *srcx = src;

		if (d == 32) {
			memcpy(dst, src, 4 * info.width);
		} else if (d == 8) {
			for (int dw = 0; dw < info.width; dw++) {
				dstx[0] = dstx[1] = dstx[2] = srcx[0];
				dstx[3] = 0xFF;

				dstx += 4;
				srcx += 1;
			}
		} else if (d == 1) {
			for (int dw = 0; dw < info.width; dw++) {
				dstx[0] = dstx[1] = dstx[2] =
						(1 << (7 - (dw & 7)) & srcx[0]) ? 0x00 : 0xFF;
				//dstx[0] = dstx[1] = dstx[2] = (srcx[0] & (dw % 8)) ? 0xFF : 0x00;
				dstx[3] = 0xFF;

				dstx += 4;
				srcx += ((dw % 8) == 7) ? 1 : 0;
			}
		}

		dst += dstBpl;
		src += srcBpl;
	}
	pixEndianByteSwap(pixs);
	AndroidBitmap_unlockPixels(env, bitmap);
}

l_int32 renderTransformedBoxa(PIX *pixt, BOXA *boxa, l_int32 i) {
	l_int32 j, n, rval, gval, bval;
	BOX *box;

	n = boxaGetCount(boxa);
	rval = (1413 * i) % 256;
	gval = (4917 * i) % 256;
	bval = (7341 * i) % 256;
	for (j = 0; j < n; j++) {
		box = boxaGetBox(boxa, j, L_CLONE);
		pixRenderHashBoxArb(pixt, box, 10, 3, i % 4, 1, rval, gval, bval);
		boxDestroy(&box);
	}
	return 0;
}

jfloat getTextLineHeight(Pix* pixg){
	Pix* pixEdge = pixSobelEdgeFilter(pixg, L_ALL_EDGES);
	Pix* pixb;
	l_int32 width = pixGetWidth(pixEdge);
	l_int32 height = pixGetHeight(pixEdge);
	pixOtsuAdaptiveThreshold(pixEdge, width, height, 0, 0, 0, NULL, &pixb);
	l_float32 textLineSpacing = pixGetTextLineSpacing(pixb);
	pixDestroy
}

jint Java_de_renard_camerapreview_ImageFilter_nativeProcessImage(JNIEnv *env,
		jobject javathis, jbyteArray frame, jobject bitmap, jobject binaryBitmap) {

	ostringstream debugstring;
	startTimer();
	jbyte *data_buffer = env->GetByteArrayElements(frame, NULL);
	l_uint8 *byte_buffer = (l_uint8 *) data_buffer;
	yuvToPixFast(byte_buffer, mPix32, mPix8);
	Pix* rotated = pixRotate90(mPix8,1);
    Pix* pixb;
    /*
    Boxa* textRegions = pixFindTextRegions(rotated,&pixb);
    pixDestroy(&rotated);
    l_int32 w = pixGetWidth(mPix32);
    l_int32 h = pixGetHeight(mPix32);
//    Boxa* rotatedBoxa = boxaRotateOrth(textRegions,h,w,3);
//
//    renderTransformedBoxa(mPix32,rotatedBoxa,255);
//    boxaDestroy(&textRegions);
//    boxaDestroy(&rotatedBoxa);

*/
    //pixb = pixTreshold2(rotated);
    Pixa* pixaTextBlocks= pixFindTextBlocks(rotated);

    l_int32 width = pixGetWidth(rotated) / 2;
    l_int32 height = pixGetHeight(rotated) / 2;

    //	//assemble all text blocks into one pix to display it
    pixb = pixCreate(width, height, 1);
    l_int32 n = pixaGetCount(pixaTextBlocks);
    for (int i = 0; i < n; i++) {
        Pix* pix = pixaGetPix(pixaTextBlocks, i, L_CLONE);
    	Box* box = pixaGetBox(pixaTextBlocks, i, L_CLONE);
    	l_int32 x, y, w, h;
    	boxGetGeometry(box, &x, &y, &w, &h);
    	pixRasterop(pixb, x, y, w, h, PIX_SRC, pix, 0, 0);
    	boxDestroy(&box);
        pixDestroy(&pix);
     }


    pixDestroy(&rotated);

	env->ReleaseByteArrayElements(frame, data_buffer, JNI_ABORT);
	debugstring << "convert to RGB: " << stopTimer() << std::endl;

	writeBitmap(env, mPix32, bitmap);
    //renderTransformedBoxa(pixb,textRegions,255);

	Pix* rotatedb = pixRotate90(pixb,-1);
	Pix* rotatedbExpanded = pixExpandReplicate(rotatedb,2);

	writeBitmap(env, rotatedbExpanded, binaryBitmap);
	pixDestroy(&pixb);
	pixDestroy(&rotatedb);
	pixDestroy(&rotatedbExpanded);

	LOGI(debugstring.str().c_str());
	return (jint) pixaTextBlocks;

}

#ifdef __cplusplus
}
#endif
