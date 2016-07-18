package pl.piotr.myvrpet.camera;

public class NativePart {



    public static final int PROCESSING_MODE_NO_PROCESSING = 0;
    public static final int PROCESSING_MODE_CPU = 1;
    public static final int PROCESSING_MODE_OCL_DIRECT = 2;
    public static final int PROCESSING_MODE_OCL_OCV = 3;

    public static native void initCL();
    public static native void closeCL();
    public static native void processFrame(int tex1, int tex2, int w, int h, int mode);
}
