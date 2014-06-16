package de.renard.camerapreview;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.Toast;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

// ----------------------------------------------------------------------

public class CameraPreview extends Activity implements Runnable,
        SurfaceHolder.Callback, PreviewCallback {
    private static final String LOG_TAG = CameraPreview.class.getSimpleName();

    Camera mCamera;
    int cameraCurrentlyLocked;
    public final static int IMAGE_W = 176;
    public final static int IMAGE_H = 144;
    //	public final static int IMAGE_W = 640;
//	public final static int IMAGE_H = 480;
    Rect mPreviewSrcRect = new Rect();
    Rect mPreviewDestRect = new Rect();

    private ImageFilter mImageProcessing;
    private Paint mPaint;
    private Button mButtonShutter;
    private Button mButtonFocus;
    private byte[] mData = null;
    private boolean mSurfaceCreated = false;

    // the app will draw here
    private SurfaceView mRenderSurfacveView;
    // the camera will draw here
    private SurfaceView mCameraSurfacveView;
    // private List<Size> mSupportedPreviewSizes;
    // executes the image processing in a seperate thread
    ExecutorService mExecutor = Executors.newSingleThreadExecutor();
    private boolean mTakePicture;
    private TextBlockIndicatorView mTextBlockIndicatorView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(
                WindowManager.LayoutParams.FLAG_FULLSCREEN
                        | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.main);

        mPaint = new Paint();

        initSurfaces();
        initButtons();
    }

    private void initSurfaces() {
        mCameraSurfacveView = (SurfaceView) findViewById(R.id.surfaceViewCamera);
        mRenderSurfacveView = (SurfaceView) findViewById(R.id.surfaceViewRender);
        mTextBlockIndicatorView = (TextBlockIndicatorView) findViewById(R.id.textBlockIndicatorView);


        mPreviewSrcRect.right = IMAGE_W;
        mPreviewSrcRect.bottom = IMAGE_H;
        mPreviewDestRect.right = IMAGE_W * 2;
        mPreviewDestRect.bottom = IMAGE_H * 2;
        mCameraSurfacveView.getHolder().addCallback(this);
    }

    private void initButtons() {
        mButtonShutter = (Button) findViewById(R.id.buttonShutter);
        mButtonShutter.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mTakePicture = true;
                mButtonShutter.setVisibility(View.INVISIBLE);
            }
        });
        mButtonFocus = (Button) findViewById(R.id.buttonFocus);
        mButtonFocus.setOnClickListener(new View.OnClickListener() {

            public void onClick(View v) {
                if (mCamera != null) {
                    mButtonFocus.setVisibility(View.INVISIBLE);
                    mCamera.autoFocus(new AutoFocusCallback() {
                        public void onAutoFocus(boolean success, Camera camera) {
                            Log.i(LOG_TAG, "Focus " + success);
                            mButtonFocus.setVisibility(View.VISIBLE);
                        }
                    });
                }
            }
        });
    }

    @Override
    protected void onResume() {
        super.onResume();
        mCamera = Camera.open();
        if (mSurfaceCreated == true) {
            startPreview();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.release();
            mCamera = null;
        }
    }

    /**
     * async processing of frames
     */
    public void run() {
        if (mData == null) {
            return;
        }
        mImageProcessing.processFrame(mData);
        Bitmap binaryBitmap = mImageProcessing.getBinaryBitmap();
        Bitmap rgbBitmap = mImageProcessing.getRGBBitmap();

        Canvas c = mRenderSurfacveView.getHolder().lockCanvas();
        if (c != null) {
            if (binaryBitmap != null) {
                c.save();
                c.translate(mPreviewDestRect.height(), 0);
                c.rotate(90, 0, 0);
                c.drawBitmap(binaryBitmap, null, mPreviewDestRect, mPaint);
                c.restore();
            }
            mRenderSurfacveView.getHolder().unlockCanvasAndPost(c);
            mTextBlockIndicatorView.setTextBlockRects(mImageProcessing.getCurrentTextBLocks(),binaryBitmap.getHeight()/2, binaryBitmap.getWidth()/2);

            if (mTakePicture) {
                FileOutputStream file = null;
                try {
                    file = new FileOutputStream(Environment.getExternalStorageDirectory().getPath() + "/out.jpg");
                    binaryBitmap.compress(Bitmap.CompressFormat.JPEG, 100, file);
                    file = new FileOutputStream(Environment.getExternalStorageDirectory().getPath() + "/out2.jpg");
                    rgbBitmap.compress(Bitmap.CompressFormat.JPEG, 100, file);
                } catch (FileNotFoundException e) {
                    Toast.makeText(getBaseContext(), e.getMessage(), Toast.LENGTH_LONG).show();
                }

                mTakePicture = false;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        mButtonShutter.setVisibility(View.VISIBLE);
                    }
                });

            }


        }
        if (mCamera != null) {
            mCamera.addCallbackBuffer(mData);
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(LOG_TAG, "surfaceCreated");

        if (mCamera != null) {
            mCamera.stopPreview();
            try {
                mCamera.setPreviewDisplay(mCameraSurfacveView.getHolder());
            } catch (IOException e) {
                e.printStackTrace();
            }
            mSurfaceCreated = true;
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(LOG_TAG, "surfaceDestroyed");
        if (mCamera != null) {
            mCamera.stopPreview();
        }
    }

    private void startPreview() {
        if (mCamera != null) {
            mCamera.stopPreview();
            mCamera.setPreviewCallbackWithBuffer(this);
            Camera.Parameters parameters = mCamera.getParameters();
            parameters.setPreviewSize(IMAGE_W, IMAGE_H);
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_MACRO);
            List<Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
            mCamera.setParameters(parameters);
            PixelFormat p = new PixelFormat();
            PixelFormat.getPixelFormatInfo(parameters.getPreviewFormat(), p);
            int bufSize = (IMAGE_W * IMAGE_H * p.bitsPerPixel) / 8;
            mCamera.setDisplayOrientation(90);
            mImageProcessing = new ImageFilter(IMAGE_W, IMAGE_H);
            mCamera.addCallbackBuffer(new byte[bufSize]);
            mCamera.startPreview();
        }

    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        Log.i(LOG_TAG, "surfaceChanged");
        startPreview();
    }

    public void onPreviewFrame(final byte[] data, Camera camera) {
        Log.i(LOG_TAG, "onPreviewFrame");
        if (mCamera == null) {
            return;
        }
        mData = data;
        mExecutor.execute(this);
    }

    private void saveFrameAsBitmap(byte[] frame) {
        try {
            Camera.Parameters parameters = mCamera.getParameters();
            Size size = parameters.getPreviewSize();
            YuvImage image = new YuvImage(frame, parameters.getPreviewFormat(), size.width, size.height, null);
            File file = new File(Environment.getExternalStorageDirectory().getPath() + "/out.jpg");
            FileOutputStream filecon = new FileOutputStream(file);
            image.compressToJpeg(new Rect(0, 0, image.getWidth(), image.getHeight()), 90, filecon);
        } catch (FileNotFoundException e) {
            Toast.makeText(getBaseContext(), e.getMessage(), Toast.LENGTH_LONG).show();
        }
    }
}