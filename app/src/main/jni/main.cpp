//
// Created by piotr on 18/08/16.
//

#include <CL/cl_platform.h>
#include "engine/engine.h"
#include "opencl/CLcommon.hpp"
#include <math.h>

int width;
int height;
float *modelView;
float projectedTouchX = 0;
float projectedTouchY = 0;
float mHeadForwardVector[4];
float mHeadRotationQuaternion[4];
float mHeadRightVector[4];

extern "C" void processFrame(int tex1, int tex2, int w, int h, int mode) {
    //LOGD("processing mode %d", mode);


    int result[18][1];
    procOCL_I2I(tex1, tex2, w, h, result);

    for (int i = 0; i < 1; i++) {
        //     LOGD("result %d x=%d y=%d", i, result[i][0], result[i][1]);
    }

    /*

    switch (mode) {
        case PROC_MODE_NO_PROC:
        case PROC_MODE_CPU:
            drawFrameProcCPU(w, h, tex2);
            break;
        case PROC_MODE_OCL_DIRECT:
            procOCL_I2I(tex1, tex2, w, h);
            break;

        default:;
            //LOGE("Unexpected processing mode: %d", mode);
    }
*/
//    drawFrameProcCPU(w, h, tex2);


}


int boxSize =2;
float model[16];
extern "C" void onDraw(float *perspective, float *eyeView) {

    // LOGE("on draw!");

    modelView = eyeView;
    Engine_setMVPMatrices(perspective, eyeView, model);
    //  Engine_prepare_draw();
    Engine_glBegin(GL_TRIANGLE_STRIP);
    Engine_glColor3f(0, 1, 0);
    Engine_glVertex3f(-20, -30, -20); // tl
    Engine_glVertex3f(-20, -30, 20); // bl
    Engine_glVertex3f(20, -30, -20); // tr
    Engine_glVertex3f(20, -30, 20); // br

    //
    Engine_glEnd();



    glEnable(GL_CULL_FACE);
    Engine_glBegin(GL_TRIANGLE_STRIP);
    Engine_glColor3f(1, 0, 0);
    //bottom
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY + boxSize); // bl
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY - boxSize); // tr
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY + boxSize); // br
    //top
    //Engine_glColor3f(0, 1, 0);

    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY + boxSize); // bl
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tr
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY + boxSize); // br
    //front
    //Engine_glColor3f(0, 0, 1);
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY + boxSize); // bl
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY + boxSize); // br
    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY + boxSize); // tl
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY + boxSize); // tr
    //back
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY - boxSize); // tr
    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tr
    //left
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY + boxSize); // bl
    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tl
    Engine_glVertex3f(projectedTouchX - boxSize, -30+boxSize*2, projectedTouchY + boxSize); // bl
    //right
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY - boxSize); // tr
    Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY + boxSize); // br
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY - boxSize); // tr
    Engine_glVertex3f(projectedTouchX + boxSize, -30+boxSize*2, projectedTouchY + boxSize); // br

    // Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY - boxSize); // tl
   // Engine_glVertex3f(projectedTouchX - boxSize, -30, projectedTouchY + boxSize); // bl
   // Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY - boxSize); // tr
   // Engine_glVertex3f(projectedTouchX + boxSize, -30, projectedTouchY + boxSize); // br
    Engine_glEnd();
    glDisable(GL_CULL_FACE);


};
extern "C" void onSurfaceCreated(int w, int h) {
    Engine_setup_opengl(w, h);
    LOGE("surface created");
    width = w;
    height = h;

};
extern "C" void onSurfaceChanged(int w, int h) {
    Engine_on_surface_changed(w, h);
    LOGE("surface changed");
    width = w;
    height = h;

};

extern "C" void onHeadTransform(float *quat, float *forwardVector,float *rightVector) {
    for (int i = 0; i < 4; i++) {
        mHeadRotationQuaternion[i] = quat[i];
        mHeadForwardVector[i] = forwardVector[i];
        mHeadRightVector[i]=rightVector[i];
    }
    //LOGD("head rotation %f %f ",quaternion[0],quaternion[3]);
}


extern "C" void onTouch(float x, float y) {

    //float qX = mHeadRotationQuaternion[0];
    //float qY = mHeadRotationQuaternion[1];
    //float qZ = mHeadRotationQuaternion[2];
    //float qW = mHeadRotationQuaternion[3];


    //float rotationMatrix[] =
    //        {1 - 2*qY*qY - 2*qZ*qZ , 2*qX*qY + 2*qW*qZ, 2*qX*qZ - 2*qW*qY,
    //        2*qX*qY - 2*qW*qZ, 1 - 2*qX*qX - 2*qZ*qZ, 2*qY*qZ + 2*qW*qX,
    //        2*qX*qZ + 2*qW*qY, 2*qY*qZ - 2*qW*qX, 1 - 2*qX*qX - 2*qY*qY
    //        };

    //float xPrime = mHeadForwardVector[0]*rotationMatrix[0]+mHeadForwardVector[1]*rotationMatrix[1]+mHeadForwardVector[2]*rotationMatrix[2];
    //float yPrime = mHeadForwardVector[0]*rotationMatrix[3]+mHeadForwardVector[1]*rotationMatrix[4]+mHeadForwardVector[2]*rotationMatrix[5];
    //float zPrime = mHeadForwardVector[0]*rotationMatrix[6]+mHeadForwardVector[1]*rotationMatrix[7]+mHeadForwardVector[2]*rotationMatrix[8];




    float normalizedX = (float) ((x) * 2.0f / width - 1.0);
    float normalizedY = (float) ((y) * 2.0f / height - 1.0);

    // float direction[] = {modelView[2], modelView[6],modelView[10]};

    // float direction[] = forward;
    // float xPrime = direction[0]*quaternion[0]+direction[2]*quaternion[1];
    // float yPrime = direction[0]*quaternion[2]+direction[2]*quaternion[3];

    //LOGD("prime x %f  prime y %f prime z ",xPrime,yPrime,zPrime);

  //  LOGD("prime rotation   %f %f %f %f %f %f %f %f %F ", rotationMatrix[0],rotationMatrix[1],
  //       rotationMatrix[2],rotationMatrix[3],rotationMatrix[4],rotationMatrix[5]
  //  ,rotationMatrix[6],rotationMatrix[7],rotationMatrix[8]);


    LOGD("prime forward %f %f %f ",mHeadForwardVector[0],mHeadForwardVector[1],mHeadForwardVector[2]);
    LOGD("prime right %f %f %f ",mHeadRightVector[0],mHeadRightVector[1],mHeadRightVector[2]);

    //if(mHeadRightVector[2]>0)



    //sin2(t) + cos2(t) = 1
    //sin(t)=\/1-cos2(t)

    //R= (cost -sint
    //    sint  cost)



    int mark = mHeadRightVector[2]>0?1:-1;
    float cos = mHeadRightVector[0];
    float sin = sqrtf(1-cos*cos);

    //float m2drMatrix[] = {cos,-sin,
    //                      sin,cos};

    float normalizedRotatedX = (normalizedX * cos -normalizedY*sin);
    float normalizedRotatedY =( normalizedX  *sin + normalizedY*cos);

    //normalizedRotatedX*=mark;
    //normalizedRotatedY*=mark;
    LOGD("nX=%f nY=%f nrX=%f nrY=%f, mark=%d",normalizedX,normalizedX,normalizedRotatedX,normalizedRotatedY,mark);










    float yDir = mHeadForwardVector[1];
    float delta = -30.0f / yDir;

    projectedTouchX = (normalizedRotatedX + mHeadForwardVector[0]) * delta;
    projectedTouchY = ((mHeadForwardVector[2] + normalizedRotatedY) * delta);
    //projectedTouchX=direction[0];
    //projectedTouchY=direction[2];

    // LOGD("result %f %f %f %f  ",quaternion[0],quaternion[1],quaternion[2],quaternion[3]);


    //projectedTouchX=direction[0];
    //projectedTouchY=direction[2];
    //LOGD("on touch %f %f %d %d  ",x,y,width,height);
    //LOGD("on touch %f %f %f %f  ",normalizedX,normalizedY,projectedTouchX,projectedTouchY);

}