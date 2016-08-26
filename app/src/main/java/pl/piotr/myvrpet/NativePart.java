package pl.piotr.myvrpet;

public class NativePart {


    public static final int PROCESSING_MODE_NO_PROCESSING = 0;
    public static final int PROCESSING_MODE_CPU = 1;
    public static final int PROCESSING_MODE_OCL_DIRECT = 2;
    public static final int PROCESSING_MODE_OCL_OCV = 3;

    public static native void initCL();

    public static native void closeCL();

    public static native void processFrame(int tex1, int tex2, int w, int h, int mode);

    public static native void onDraw(float[] modelViewProjection, float[] eyeView, float[] perspective);

    public static native void onSurfaceCreated(int w, int h);

    public static native void onSurfaceChanged(int w, int h);

    public static native void onTouch(float w, float h);

    public static native void onHeadTranform(float[] quaternion, float[] forwardVector, float[] rightVector);

}