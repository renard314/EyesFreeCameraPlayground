package de.renard.camerapreview;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;

/**
 * Created by renard on 20/02/14.
 */
public class TextBlockIndicatorView extends View {

    private ArrayList<Rect> mRects = new ArrayList<Rect>();
    private final Paint mRectPaint=new Paint();
    private int mWidth;
    private int mHeight;

    public TextBlockIndicatorView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mRectPaint.setStyle(Paint.Style.STROKE);
        mRectPaint.setColor(Color.argb(0xFF,0xAA,0x00,0x66));
        mRectPaint.setStrokeWidth(4);
    }

    public TextBlockIndicatorView(Context context) {
        super(context);
    }

    public void setTextBlockRects(ArrayList<Rect> rects, int width, int height){
        mWidth = width;
        mHeight = height;
        mRects.clear();
        mRects.addAll(rects);

        postInvalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        for(Rect r: mRects){
            canvas.save();
            final float scaleX = ((float)getWidth())/mWidth;
            final float scaleY = ((float)getHeight())/mHeight;
            r.left *= scaleX;
            r.right*=scaleX;
            r.top*=scaleY;
            r.bottom*=scaleY;
            canvas.drawRect(r,mRectPaint);
        }
    }
}
