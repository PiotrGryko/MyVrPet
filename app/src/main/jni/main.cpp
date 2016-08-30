//
// Created by piotr on 18/08/16.
//

#include <CL/cl_platform.h>
#include "engine/engine.h"
#include "opencl/CLcommon.hpp"
#include <math.h>
#include "glm.hpp"
#include "engine/glu_imp.h"

float mModelView[16];
float mPerspective[16];
float mModelViewProjection[16];

float projectedTouchX = 0;
float projectedTouchY = 0;
float mHeadForwardVector[4];
float mHeadRotationQuaternion[4];
float mHeadRightVector[4];


int mProcessingResult[18][2];

float static depth=-20;

float screenHeight;
float screenWidth;



int fanSize =10;
cl_float2 linesPoints[10];
cl_int2 objectPosition;

static void project3Dto2D(float output[3],float x, float y, float z, float maxWidth, float maxHeight)
{
    int viewport[] = {0, 0, maxWidth, maxHeight};

    GLfloat  resultx;
    GLfloat  resulty;
    GLfloat resultz;

    GLint  result = gluProject(x,y,z,mModelView,mPerspective,viewport,&resultx, &resulty,&resultz);

    output[0]=resultx;
    output[1]=resulty;
    output[2]=resultz;
    LOGD("project result %f %f %f ", resultx,resulty,resultz);

}

static void project2Dto3D(float output[3], float x, float y, float maxWidth, float maxHeight)
{

    GLfloat  xNear;
    GLfloat  yNear;
    GLfloat  zNear;


    GLfloat  xFar;
    GLfloat  yFar;
    GLfloat  zFar;

    int viewport[] = {0, 0, maxWidth, maxHeight};



    GLint resultOne = gluUnProject(x, y, 0, mModelView,  mPerspective, viewport, &xNear,&yNear,&zNear);
    GLint resultTwo = gluUnProject(x, y, 1, mModelView,  mPerspective, viewport, &xFar,&yFar,&zFar);



            output[0]=xFar- xNear;
            output[1]=yFar - yNear;
            output[2]=zFar - zNear;


    float delta = -20.0f/output[1];
    output[0]*=delta;
    output[1]*=delta;
    output[2]*=delta;
    LOGD("near %f %f %f far %f %f %f ",xNear,yNear,zNear,xFar,yFar,zFar);

}

extern "C" void processFrame(int tex1, int tex2, int w, int h, int mode) {


    procOCL_I2I(tex1, tex2, w, h, mProcessingResult,fanSize,objectPosition,linesPoints);


    for (int i = 0; i < 10; i++) {
             LOGD("result %d x=%d y=%d", i, mProcessingResult[0][0], mProcessingResult[0][1]);
    }

}


int boxSize = 1;
float model[16];


static void generateFan(float centerX, float centerY, float centerZ, cl_float2 *linesEdges, int count)
{



    float screenCenter[3];
    project3Dto2D(screenCenter,centerX,centerZ,centerY,1280,720);
    objectPosition = {(int)screenCenter[0],(int)screenCenter[1]};

    LOGD("object %f %f %f, projected %f %f %f",centerX,centerZ,centerY,screenCenter[0],screenCenter[1],screenCenter[2]);
    float PI = 3.14;

    float length =5;
    float cost=1;
    float sint=0;
    float angle=0;
    Engine_glBegin(GL_LINE_STRIP);
    Engine_glColor3f(0, 1, 0);
    for(int i=0;i<count;i++) {

        angle =36*i;
        float radians = angle*3.14f/180.0f;
        cost = (float)cos(radians);
        sint = sqrtf(1-cost*cost);
        if(angle>180) sint=-sint;

        float pointX = centerX +cost*length;
        float pointY = centerY + sint*length;

        LOGD("3d center %f %f point %f %f ",centerX,centerY,pointX,pointY);

        Engine_glVertex3f(centerX, centerZ, centerY); // tl
        Engine_glVertex3f(pointX, centerZ, pointY); // bl



        //project points to the screen
        float screenPoint[3];
        project3Dto2D(screenPoint,pointX,centerZ,pointY,1280,720);

        linesEdges[i]={screenPoint[0],screenPoint[1]};


    }
    Engine_glEnd();

}



static void drawCube(float centerX, float centerY, float centerZ, float boxSize) {




    generateFan(centerX,centerY,centerZ,linesPoints,fanSize);


    glEnable(GL_CULL_FACE);
    Engine_glBegin(GL_TRIANGLE_STRIP);
    Engine_glColor3f(1, 0, 0);
    //bottom
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY - boxSize); // tl
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY + boxSize); // bl
    Engine_glVertex3f(centerX + boxSize, centerZ, centerY - boxSize); // tr
    Engine_glVertex3f(centerX + boxSize, centerZ, centerY + boxSize); // br
    //top
    //Engine_glColor3f(0, 1, 0);

    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY - boxSize); // tl
    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY + boxSize); // bl
    Engine_glVertex3f(centerX + boxSize, centerZ + boxSize * 2,
                      centerY - boxSize); // tr
    Engine_glVertex3f(centerX + boxSize, centerZ + boxSize * 2,
                      centerY + boxSize); // br
    //front
    //Engine_glColor3f(0, 0, 1);
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY + boxSize); // bl
    Engine_glVertex3f(centerX + boxSize, centerZ, centerY + boxSize); // br
    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY + boxSize); // tl
    Engine_glVertex3f(centerX + boxSize, centerZ + boxSize * 2,
                      centerY + boxSize); // tr
    //back
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY - boxSize); // tl
    Engine_glVertex3f(centerX + boxSize, centerZ, centerY - boxSize); // tr
    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY - boxSize); // tl
    Engine_glVertex3f(centerX + boxSize,centerZ + boxSize * 2,
                      centerY - boxSize); // tr
    //left
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY - boxSize); // tl
    Engine_glVertex3f(centerX - boxSize, centerZ, centerY + boxSize); // bl
    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY - boxSize); // tl
    Engine_glVertex3f(centerX - boxSize, centerZ + boxSize * 2,
                      centerY + boxSize); // bl
    //right
    Engine_glVertex3f(centerX + boxSize, centerZ , centerY - boxSize); // tr
    Engine_glVertex3f(centerX + boxSize, centerZ , centerY + boxSize); // br
    Engine_glVertex3f(centerX + boxSize, centerZ  + boxSize * 2,
                      centerY - boxSize); // tr
    Engine_glVertex3f(centerX + boxSize, centerZ  + boxSize * 2,
                      centerY + boxSize); // br

    Engine_glEnd();
    glDisable(GL_CULL_FACE);







}

extern "C" void onDraw(float *modelViewProjection, float *eyeView, float *perspective) {

    // LOGE("on draw!");

    for(int i=0;i<16;i++)
    {
        mModelView[i]=eyeView[i];
        mPerspective[i]=perspective[i];
        mModelViewProjection[i]=modelViewProjection[i];
    }

    Engine_setMVPMatrices(modelViewProjection, eyeView, model);
    //  Engine_prepare_draw();
/*

    Engine_glBegin(GL_TRIANGLE_STRIP);
    Engine_glColor3f(0, 1, 0);
    Engine_glVertex3f(-200, depth, -200); // tl
    Engine_glVertex3f(-200, depth, 200); // bl
    Engine_glVertex3f(200, depth, -200); // tr
    Engine_glVertex3f(200, depth, 200); // br
    Engine_glEnd();
*/
/*

    for(int i=0;i<10;i++)
    {
        float x = mProcessingResult[i][0];
        if(x==0)x=640;
        float y = 720-mProcessingResult[i][1];
        if(y==0)y=360;
        //if(mProcessingResult[i][0]>0 || mProcessingResult[i][1]>0)
       // {
        float output[3];
        project2Dto3D(output,x,720-y,1280,720);
        drawCube(output[0],output[2],depth,1);
            //LOGD("draw cube x=%f y=%f",x, y);

       // }
    }
*/
    //LOGD("draw cube x=%f y=%f", projectedTouchX, projectedTouchY);

    drawCube(projectedTouchX, projectedTouchY,depth, boxSize);



};
extern "C" void onSurfaceCreated(int w, int h) {
    Engine_setup_opengl(w, h);
    LOGE("surface created");
    screenWidth = w;
    screenHeight = h;

};
extern "C" void onSurfaceChanged(int w, int h) {
    Engine_on_surface_changed(w, h);
    LOGE("surface changed");
    screenWidth = w;
    screenHeight = h;

};

extern "C" void onHeadTransform(float *quat, float *forwardVector, float *rightVector) {
    for (int i = 0; i < 4; i++) {
        mHeadRotationQuaternion[i] = quat[i];
        mHeadForwardVector[i] = forwardVector[i];
        mHeadRightVector[i] = rightVector[i];
    }
    //LOGD("head rotation %f %f ",quaternion[0],quaternion[3]);
}



extern "C" void onTouch(float x, float y) {



    LOGD("on touch %f %f ",x,y);
    projectedTouchX=x;
    projectedTouchY=y;
    float output[3];
    project2Dto3D(output,x,screenHeight-y,screenWidth,screenHeight);
    projectedTouchX=output[0];
    projectedTouchY=output[2];
   // projectedTouchZ=output[2];

}