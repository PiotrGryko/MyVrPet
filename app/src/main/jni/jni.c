#include <jni.h>
#include <stddef.h>

void initCL();
void closeCL();
void processFrame(int tex1, int tex2, int w, int h, int mode);

void onDraw(float *perspective, float *eyeView);
void onSurfaceCreated(int w, int h);
void onSurfaceChanged(int w, int h);

void onTouch(float x, float y);
void onHeadTransform(float *quaternion, float *forwardVector,float *rightVector);

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_NativePart_initCL(JNIEnv * env, jclass cls)
{
    initCL();
}

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_NativePart_closeCL(JNIEnv * env, jclass cls)
{
    closeCL();
}

JNIEXPORT void JNICALL Java_pl_piotr_myvrpet_NativePart_processFrame(JNIEnv * env, jclass cls, jint tex1, jint tex2, jint w, jint h, jint mode)
{
    processFrame(tex1, tex2, w, h, mode);
}

JNIEXPORT void JNICALL
Java_pl_piotr_myvrpet_NativePart_onDraw(JNIEnv *env, jclass type, jfloatArray perspective_,
                                               jfloatArray eyeVoew_) {
    jfloat *perspective = (*env)->GetFloatArrayElements(env, perspective_, NULL);
    jfloat *eyeVoew = (*env)->GetFloatArrayElements(env, eyeVoew_, NULL);

    // TODO
    onDraw(perspective,eyeVoew);

    (*env)->ReleaseFloatArrayElements(env, perspective_, perspective, 0);
    (*env)->ReleaseFloatArrayElements(env, eyeVoew_, eyeVoew, 0);
}

JNIEXPORT void JNICALL
Java_pl_piotr_myvrpet_NativePart_onSurfaceCreated(JNIEnv *env, jclass type,jint w, jint h) {

    onSurfaceCreated(w,h);
    // TODO

}

JNIEXPORT void JNICALL
Java_pl_piotr_myvrpet_NativePart_onSurfaceChanged(JNIEnv *env, jclass type, jint w, jint h) {

    onSurfaceChanged(w,h);
    // TODO

}

JNIEXPORT void JNICALL
Java_pl_piotr_myvrpet_NativePart_onTouch(JNIEnv *env, jclass type, jfloat w, jfloat h) {

    // TODO

    onTouch(w,h);
}

JNIEXPORT void JNICALL
Java_pl_piotr_myvrpet_NativePart_onHeadTranform(JNIEnv *env, jclass type, jfloatArray quaternion_, jfloatArray forwardVector_, jfloatArray rightVector_) {
    jfloat *quaternion = (*env)->GetFloatArrayElements(env, quaternion_, NULL);
    jfloat *forwardVector = (*env)->GetFloatArrayElements(env,forwardVector_,NULL);
    jfloat *rightVector = (*env)->GetFloatArrayElements(env,rightVector_,NULL);

    // TODO
    onHeadTransform(quaternion,forwardVector,rightVector);

    (*env)->ReleaseFloatArrayElements(env, quaternion_, quaternion, 0);
    (*env)->ReleaseFloatArrayElements(env, forwardVector_, forwardVector, 0);
    (*env)->ReleaseFloatArrayElements(env, rightVector_,rightVector, 0);


}