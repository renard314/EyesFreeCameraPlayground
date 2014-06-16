package de.renard.camerapreview;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.util.Pair;

import com.googlecode.leptonica.android.Pixa;

import java.util.ArrayList;

public class ImageFilter {

	private Bitmap mBitmap = null;
    private Bitmap mBinaryBitmap = null;
    private ArrayList<Rect> mCurrentTextBLocks;

	static {
		System.loadLibrary("lept");
		System.loadLibrary("filter");
	}

	public ImageFilter(int width, int height) {
		nativeStart(width, height);
		mBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        mBinaryBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
	}

	public void processFrame(byte[] frame) {
		int nativePixa = nativeProcessImage(frame, mBitmap, mBinaryBitmap);
        Pixa pixa = new Pixa(nativePixa,mBinaryBitmap.getWidth(),mBinaryBitmap.getHeight());
        mCurrentTextBLocks = pixa.getBoxRects();
        pixa.recycle();
    }

    public Bitmap getBinaryBitmap(){
        return mBinaryBitmap;
    }

    public Bitmap getRGBBitmap() {
        return mBitmap;
    }

    public ArrayList<Rect> getCurrentTextBLocks(){
        return mCurrentTextBLocks;
    }


	public void close() {
        mBitmap.recycle();
        mBinaryBitmap.recycle();
		nativeEnd();
	}

	private native int nativeProcessImage(byte[] frame, Bitmap bitmap, Bitmap binaryBitmap);

	private native void nativeStart(int w, int h);

	private native void nativeEnd();

}
