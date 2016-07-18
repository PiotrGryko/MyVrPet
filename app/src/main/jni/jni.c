
#include <jni.h>

void initCL();
void closeCL();
void processFrame(int tex1, int tex2, int w, int h, int mode);

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_camera_NativePart_initCL(JNIEnv * env, jclass cls)
{
    initCL();
}

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_camera_NativePart_closeCL(JNIEnv * env, jclass cls)
{
    closeCL();
}

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_camera_NativePart_processFrame(JNIEnv * env, jclass cls, jint tex1, jint tex2, jint w, jint h, jint mode)
{
    processFrame(tex1, tex2, w, h, mode);
}
