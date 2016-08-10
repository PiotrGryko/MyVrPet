/*
 * Copyright 2014 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package pl.piotr.myvrpet;

import com.google.vr.sdk.audio.GvrAudioEngine;
import com.google.vr.sdk.base.Eye;
import com.google.vr.sdk.base.GvrActivity;
import com.google.vr.sdk.base.GvrView;
import com.google.vr.sdk.base.HeadTransform;
import com.google.vr.sdk.base.Viewport;

import android.content.Context;
import android.opengl.GLES20;
import android.opengl.GLU;
import android.opengl.Matrix;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import org.opencv.android.CameraGLSurfaceView;


import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import pl.piotr.myvrpet.camera.CameraOpecCVRendererBase;
import pl.piotr.myvrpet.camera.CameraOpenCvRendererV15;
import pl.piotr.myvrpet.camera.CameraOpenCvRendererV21;
import pl.piotr.myvrpet.camera.NativePart;

/**
 * A Google VR sample application.
 * </p><p>
 * The TreasureHunt scene consists of a planar ground grid and a floating
 * "treasure" cube. When the user looks at the cube, the cube will turn gold.
 * While gold, the user can activate the Carboard trigger, which will in turn
 * randomly reposition the cube.
 */
public class MainActivity extends GvrActivity implements GvrView.StereoRenderer {


    static {
        System.loadLibrary("hello-jni");
    }



    protected float[] modelCube;
    protected float[] modelPosition;

    private static final String TAG = "MainActivity";

    private static final float Z_NEAR = 0.1f;
    private static final float Z_FAR = 100.0f;

    private static final float CAMERA_Z = 0.01f;
    private static final float TIME_DELTA = 0.3f;

    private static final float YAW_LIMIT = 0.12f;
    private static final float PITCH_LIMIT = 0.12f;

    public static final int COORDS_PER_VERTEX = 3;

    // We keep the light always position just above the user.
    private static final float[] LIGHT_POS_IN_WORLD_SPACE = new float[]{0.0f, 2.0f, 0.0f, 1.0f};

    // Convenience vector for extracting the position from a matrix via multiplication.
    private static final float[] POS_MATRIX_MULTIPLY_VEC = {0, 0, 0, 1.0f};

    private static final float MIN_MODEL_DISTANCE = 3.0f;
    private static final float MAX_MODEL_DISTANCE = 7.0f;

    private static final String SOUND_FILE = "cube_sound.wav";

    public final float[] lightPosInEyeSpace = new float[4];


    public float[] camera;
    public float[] view;
    public float[] headView;
    public float[] modelViewProjection;
    public float[] modelView;
    public float[] modelFloor;

    private float[] tempPosition;
    private float[] headRotation;

    private float objectDistance = MAX_MODEL_DISTANCE / 2.0f;
    private float floorDepth = -20f;

    private Vibrator vibrator;

    private GvrAudioEngine gvrAudioEngine;
    private volatile int soundId = GvrAudioEngine.INVALID_ID;


    private CameraOpecCVRendererBase cameraOpenCvRenderer;
    private Pet pet;
    private Floor floor;
    private Floor floor2;

    private float touchX;
    private float touchY;

    //private HelloWorld test;


    /**
     * Converts a raw text file, saved as a resource, into an OpenGL ES shader.
     *
     * @param type  The type of shader we will be creating.
     * @param resId The resource ID of the raw text file about to be turned into a shader.
     * @return The shader object handler.
     */
    private int loadGLShader(int type, int resId) {
        String code = readRawTextFile(resId);
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, code);
        GLES20.glCompileShader(shader);

        // Get the compilation status.
        final int[] compileStatus = new int[1];
        GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compileStatus, 0);

        // If the compilation failed, delete the shader.
        if (compileStatus[0] == 0) {
            Log.e(TAG, "Error compiling shader: " + GLES20.glGetShaderInfoLog(shader));
            GLES20.glDeleteShader(shader);
            shader = 0;
        }

        if (shader == 0) {
            throw new RuntimeException("Error creating shader.");
        }

        return shader;
    }

    /**
     * Checks if we've had an error inside of OpenGL ES, and if so what that error is.
     *
     * @param label Label to report in case of error.
     */
    public static void checkGLError(String label) {
        int error;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.e(TAG, label + ": glError " + error);
            throw new RuntimeException(label + ": glError " + error);
        }
    }

    /**
     * Sets the view to our GvrView and initializes the transformation matrices we will use
     * to render our scene.
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);


        initializeGvrView();

        modelCube = new float[16];
        camera = new float[16];
        view = new float[16];
        modelViewProjection = new float[16];
        modelView = new float[16];
        modelFloor = new float[16];
        tempPosition = new float[4];
        // Model first appears directly in front of user.
        modelPosition = new float[]{0.0f, 0.0f, -MAX_MODEL_DISTANCE / 2.0f};
        headRotation = new float[4];
        headView = new float[16];
        vibrator = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);

        // Initialize 3D audio engine.
        gvrAudioEngine = new GvrAudioEngine(this, GvrAudioEngine.RenderingMode.BINAURAL_HIGH_QUALITY);

        // cameraRenderer = new CameraRenderer();


        CameraGLSurfaceView.CameraTextureListener listener = new CameraGLSurfaceView.CameraTextureListener() {
            @Override
            public void onCameraViewStarted(int width, int height) {
                Log.d(TAG, "camera view started ");
            }

            @Override
            public void onCameraViewStopped() {
                Log.d(TAG, "camera view stopped");

            }

            @Override
            public boolean onCameraTexture(int texIn, int texOut, int width, int height) {

                //Log.d(TAG,"on camera texture");


                //if(procMode == NativePart.PROCESSING_MODE_NO_PROCESSING)
                ///    return false;

                NativePart.processFrame(texIn, texOut, width, height, 2);
                return false;

                //return false;
            }
        };


        if (android.os.Build.VERSION.SDK_INT >= 21) {
            cameraOpenCvRenderer = new CameraOpenCvRendererV21(listener, this);
            Log.d(TAG,"camera renderer v21");
        }
        else {
            cameraOpenCvRenderer = new CameraOpenCvRendererV15(listener);
            Log.d(TAG,"camera renderer v15");

        }
        pet = new Pet(this);
        floor = new Floor(this,0,0);
        floor2 = new Floor(this,0.2f,0.2f);

        //test = new HelloWorld(this);



    }


    public void initializeGvrView() {
        setContentView(R.layout.common_ui);


        GvrView gvrView = (GvrView) findViewById(R.id.gvr_view);
        gvrView.setEGLConfigChooser(8, 8, 8, 8, 16, 8);


        gvrView.setVRModeEnabled(false);


        gvrView.setRenderer(this);


        gvrView.setTransitionViewEnabled(true);
        gvrView.setOnCardboardBackButtonListener(
                new Runnable() {
                    @Override
                    public void run() {
                        onBackPressed();
                    }
                });
        setGvrView(gvrView);

        gvrView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {

                switch(event.getAction())
                {
                    case MotionEvent.ACTION_DOWN:
                    case MotionEvent.ACTION_MOVE:

                    {
                        touchX=event.getX();
                        touchY=event.getY();

                        Log.d("XXX","on touch event "+ touchX +" "+touchY);
                    }
                    break;
                }

                return false;
            }
        });
    }

    @Override
    public void onPause() {
        gvrAudioEngine.pause();
        cameraOpenCvRenderer.onPause();
        super.onPause();
    }

    @Override
    public void onResume() {
        super.onResume();
        cameraOpenCvRenderer.onResume();
        gvrAudioEngine.resume();
    }

    @Override
    public void onRendererShutdown() {
        Log.i(TAG, "onRendererShutdown");
    }

    private int screenWidth;
    private int screenHeight;
    @Override
    public void onSurfaceChanged(int width, int height) {
        Log.i(TAG, "onSurfaceChanged");
        screenHeight=height;
        screenWidth=width;
        cameraOpenCvRenderer.onSurfaceChanged(null, width, height);
        // cameraRenderer.onSurfaceChanged(null, width, height);
        //test.onSurfaceChanged(width, height);
    }

    /**
     * Creates the buffers we use to store information about the 3D world.
     * <p>
     * <p>OpenGL doesn't use Java arrays, but rather needs data in a format it can understand.
     * Hence we use ByteBuffers.
     *
     * @param config The EGL configuration used when creating the surface.
     */
    @Override
    public void onSurfaceCreated(EGLConfig config) {


        Log.i(TAG, "onSurfaceCreated");


        GLES20.glClearColor(0.1f, 0.1f, 0.1f, 0.5f); // Dark background so text shows up well.


        cameraOpenCvRenderer.onSurfaceCreated(null, config);
        //  cameraRenderer.onSurfaceCreated();


        int vertexShader = loadGLShader(GLES20.GL_VERTEX_SHADER, R.raw.light_vertex);
        int gridShader = loadGLShader(GLES20.GL_FRAGMENT_SHADER, R.raw.grid_fragment);
        int passthroughShader = loadGLShader(GLES20.GL_FRAGMENT_SHADER, R.raw.passthrough_fragment);

        pet.onSurfaceCreated(vertexShader, passthroughShader);
        floor.onSurfaceCreated(vertexShader, gridShader);
        floor2.onSurfaceCreated(vertexShader, gridShader);


        Matrix.setIdentityM(modelFloor, 0);
        Log.d("VVV","model floor "+Arrays.toString(modelFloor));

        //Matrix.translateM(modelFloor, 0, 0, floorDepth, 0); // Floor appears below user.

        Log.d("VVV","model floor transformed "+Arrays.toString(modelFloor));

        // Avoid any delays during start-up due to decoding of sound files.
        new Thread(
                new Runnable() {
                    @Override
                    public void run() {
                        // Start spatial audio playback of SOUND_FILE at the model postion. The returned
                        //soundId handle is stored and allows for repositioning the sound object whenever
                        // the cube position changes.
                        gvrAudioEngine.preloadSoundFile(SOUND_FILE);
                        soundId = gvrAudioEngine.createSoundObject(SOUND_FILE);
                        gvrAudioEngine.setSoundObjectPosition(
                                soundId, modelPosition[0], modelPosition[1], modelPosition[2]);
                        gvrAudioEngine.playSound(soundId, true);
                    }
                })
                .start();

        updateModelPosition();

        checkGLError("onSurfaceCreated");

        NativePart.initCL();



    }

    /**
     * Updates the cube model position.
     */
    protected void updateModelPosition() {
        Matrix.setIdentityM(modelCube, 0);
        Matrix.translateM(modelCube, 0, modelPosition[0], modelPosition[1], modelPosition[2]);

        // Update the sound location to match it with the new cube position.
        if (soundId != GvrAudioEngine.INVALID_ID) {
            gvrAudioEngine.setSoundObjectPosition(
                    soundId, modelPosition[0], modelPosition[1], modelPosition[2]);
        }
        checkGLError("updateCubePosition");
    }

    /**
     * Converts a raw text file into a string.
     *
     * @param resId The resource ID of the raw text file about to be turned into a shader.
     * @return The context of the text file, or null in case of error.
     */
    private String readRawTextFile(int resId) {
        InputStream inputStream = getResources().openRawResource(resId);
        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append("\n");
            }
            reader.close();
            return sb.toString();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Prepares OpenGL ES before we draw a frame.
     *
     * @param headTransform The head transformation in the new frame.
     */
    @Override
    public void onNewFrame(HeadTransform headTransform) {
        setCubeRotation();

        // Build the camera matrix and apply it to the ModelView.

        Matrix.setLookAtM(camera, 0, 0.0f, 0.0f, CAMERA_Z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        headTransform.getHeadView(headView, 0);

        // Update the 3d audio engine with the most recent head rotation.
        headTransform.getQuaternion(headRotation, 0);


        gvrAudioEngine.setHeadRotation(
                headRotation[0], headRotation[1], headRotation[2], headRotation[3]);
        // Regular update call to GVR audio engine.
        gvrAudioEngine.update();

        checkGLError("onReadyToDraw");
    }

    protected void setCubeRotation() {
        Matrix.rotateM(modelCube, 0, TIME_DELTA, 0.5f, 0.5f, 1.0f);
    }

    /**
     * Draws a frame for an eye.
     *
     * @param eye The eye to render. Includes all required transformations.
     */
    @Override
    public void onDrawEye(Eye eye) {



        //    GLES20.glEnable(GLES20.GL_DEPTH_TEST);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT | GLES20.GL_DEPTH_BUFFER_BIT);
        cameraOpenCvRenderer.onDrawFrame(null);

        //  cameraRenderer.onDrawFrame(null);

        //test.onDrawFrame();

        checkGLError("colorParam");


        // Apply the eye transformation to the camera.

        Matrix.multiplyMM(view, 0, eye.getEyeView(), 0, camera, 0);

        // Set the position of the light
        Matrix.multiplyMV(lightPosInEyeSpace, 0, view, 0, LIGHT_POS_IN_WORLD_SPACE, 0);

        // Build the ModelView and ModelViewProjection matrices
        // for calculating cube position and light.
        GLES20.glEnable(GLES20.GL_CULL_FACE);
        float[] perspective = eye.getPerspective(Z_NEAR, Z_FAR);
        Matrix.multiplyMM(modelView, 0, view, 0, modelCube, 0);
        Matrix.multiplyMM(modelViewProjection, 0, perspective, 0, modelView, 0);
        pet.drawCube();
        GLES20.glDisable(GLES20.GL_CULL_FACE);


        //Set modelView for the floor, so we draw floor in the correct location
        Matrix.multiplyMM(modelView, 0, view, 0, modelFloor, 0);
        Matrix.multiplyMM(modelViewProjection, 0, perspective, 0, modelView, 0);
        floor.drawFloor();
        drawInversedFloor(touchX,touchY,getResources().getDisplayMetrics().widthPixels,getResources().getDisplayMetrics().heightPixels,floorDepth);


        //Ray(eye,screenWidth,screenHeight,touchX,touchY);

    }


    public void drawInversedFloor(float x, float y, float screenW, float screenH, float depth) {


        // Auxiliary matrix and vectors
        // to deal with ogl.
        float[] invertedMatrix, transformMatrix,
                normalizedInPoint, outPoint;
        invertedMatrix = new float[16];
        transformMatrix = new float[16];
        normalizedInPoint = new float[4];
        outPoint = new float[4];

        // Invert y coordinate, as android uses
        // top-left, and ogl bottom-left.
        int oglTouchY = (int) (screenH - y);

       /* Transform the screen point to clip
       space in ogl (-1,1) */

        normalizedInPoint[0] =
                (float) ((x) * 2.0f / screenW - 1.0);
        normalizedInPoint[1] =
                (float) ((oglTouchY) * 2.0f / screenH - 1.0);
        normalizedInPoint[2] = - 1.0f;
        normalizedInPoint[3] = 1.0f;

        float normalizedX  =                (float) ((x) * 2.0f / screenW - 1.0);
        oglTouchY=(int)y;
        float normalizedY= (float) ((oglTouchY) * 2.0f / screenH - 1.0);
        // Log.d("XXX","normalized point "+Arrays.toString(normalizedInPoint));

/*
        normalizedInPoint[0] =
                (float) ((x) * 2.0f / screenW - 1.0);
        normalizedInPoint[2] =
                (float) ((oglTouchY) * 2.0f / screenH - 1.0);
        normalizedInPoint[1] = floorDepth;
        normalizedInPoint[3] = 1.0f;
*/
       /* Obtain the transform matrix and
       then the inverse. */
        // Print("Proj", getCurrentProjection(gl));
        // Print("Model", getCurrentModelView(gl));

        //Matrix.multiplyMM(
        //        transformMatrix, 0,
        //        modelViewProjection, 0,
        //        view, 0);
        Matrix.invertM(invertedMatrix, 0,
                modelViewProjection, 0);

       /* Apply the inverse to the point
       in clip space */


      //  normalizedInPoint[0]=1;
      //  normalizedInPoint[1]=1f;

        Matrix.multiplyMV(
                outPoint, 0,
                invertedMatrix, 0,
                normalizedInPoint, 0);
        Matrix.multiplyMV(outPoint,0,modelView, 0, outPoint, 0);

        //Log.d("XXX","out point "+Arrays.toString(outPoint));


        //floor2.setCoords(outPoint[0]*outPoint[3],outPoint[1]*outPoint[3]);
        //floor2.drawFloor();


        //floor2.setCoords(normalizedInPoint[0],normalizedInPoint[1]);
        //floor2.drawFloor();

        //Log.d("XXX","draw inverted floor "+ Arrays.toString(outPoint));

        //Log.d("XXX","draw inverted floor x "+normalizedInPoint[0]*floorDepth +"  "+normalizedInPoint[1]*floorDepth);

       // Log.d("XXX","draw inverted floor x "+outPoint[0] / outPoint[3] +" "+ outPoint[1] / outPoint[3] +" "+ outPoint[2] / outPoint[3]);

        float[] direction = {modelView[2],modelView[6],modelView[10]};

        float yDir = direction[1];
        float delta = -20.0f/yDir;

        direction[0]=(normalizedX+direction[0])*delta;
        direction[1]=direction[1]*delta;
        direction[2]=((direction[2]+normalizedY)*delta);


        float[] point = {0,0,0};
        float scale = floorDepth/direction[2];
        float out[] = {view[2]*scale,view[6]*scale,view[10]*scale};

        //Log.d("XXX","draw inverted floor "+ Arrays.toString(out));

        floor2.setCoords(direction[0],direction[2]);
        floor2.drawFloor();
        //Log.d("XXX","depth:"+floorDepth+",  draw inverted floor "+ Arrays.toString(direction));


        float[] ray_nds = {0, 0, 1};

    }

    public void Ray(Eye eye,int width, int height, float xTouch, float yTouch) {

        int[] viewport = {0, 0, width, height};

        float[] nearCoOrds = new float[3];
        float[] farCoOrds = new float[3];
        float[] temp = new float[4];
        float[] temp2 = new float[4];
        // get the near and far ords for the click

        float winx = xTouch, winy =(float)viewport[3] - yTouch;

//        Log.d(TAG, "modelView is =" + Arrays.toString(matrixGrabber.mModelView));
//        Log.d(TAG, "projection view is =" + Arrays.toString( matrixGrabber.mProjection ));

        float[] perspective = eye.getPerspective(Z_NEAR, Z_FAR);

        //Matrix.multiplyMM(modelView, 0, headView, 0, modelFloor, 0);



        int result = GLU.gluUnProject(winx, winy, 1.0f, modelView, 0, perspective, 0, viewport, 0, temp, 0);

      //  Log.d("VVV","model view "+Arrays.toString(temp));
       // Log.d("VVV","flooor1 "+Arrays.toString(floor.coords));
        //Matrix.translateM(modelFloor, 0, 0, floorDepth, 0); // Floor appears below user.

        Matrix.multiplyMV(temp2, 0, modelView, 0, temp, 0);

      ///  Log.d("VVV","flooor2 "+Arrays.toString(temp2));
     /*
        if(result == GL10.GL_TRUE){
            nearCoOrds[0] = temp2[0] / temp2[3];
            nearCoOrds[1] = temp2[1] / temp2[3];
            nearCoOrds[2] = temp2[2] / temp2[3];

        }
*/
 //       result = GLU.gluUnProject(winx, winy, 0, modelView, 0, perspective, 0, viewport, 0, temp, 0);

       // Log.d("VVV","result "+(temp[0] / temp[3])+"  "+(temp[1] / temp[3]));

        floor2.setCoords((temp2[0] / temp2[3]),(temp2[1] / temp2[3]));

       //floor2.setCoords(0.5f,0.5f);
       // floor2.drawInvertedFloor();

        floor2.drawFloor();
        /*
        Matrix.multiplyMV(temp2,0,modelView, 0, temp, 0);
        if(result == GL10.GL_TRUE){
            farCoOrds[0] = temp2[0] / temp2[3];
            farCoOrds[1] = temp2[1] / temp2[3];
            farCoOrds[2] = temp2[2] / temp2[3];
        }
       */
       // Log.d("XXX","near "+Arrays.toString(nearCoOrds));
       // Log.d("XXX","far  "+Arrays.toString(farCoOrds));

    }

    @Override
    public void onFinishFrame(Viewport viewport) {
    }


    /**
     * Called when the Cardboard trigger is pulled.
     */
    @Override
    public void onCardboardTrigger() {
        Log.i(TAG, "onCardboardTrigger");

        if (isLookingAtObject()) {
            hideObject();
        }

        // Always give user feedback.
        vibrator.vibrate(50);
    }

    /**
     * Find a new random position for the object.
     * <p>
     * <p>We'll rotate it around the Y-axis so it's out of sight, and then up or down by a little bit.
     */
    protected void hideObject() {
        float[] rotationMatrix = new float[16];
        float[] posVec = new float[4];

        // First rotate in XZ plane, between 90 and 270 deg away, and scale so that we vary
        // the object's distance from the user.
        float angleXZ = (float) Math.random() * 180 + 90;
        Matrix.setRotateM(rotationMatrix, 0, angleXZ, 0f, 1f, 0f);
        float oldObjectDistance = objectDistance;
        objectDistance =
                (float) Math.random() * (MAX_MODEL_DISTANCE - MIN_MODEL_DISTANCE) + MIN_MODEL_DISTANCE;
        float objectScalingFactor = objectDistance / oldObjectDistance;
        Matrix.scaleM(rotationMatrix, 0, objectScalingFactor, objectScalingFactor, objectScalingFactor);
        Matrix.multiplyMV(posVec, 0, rotationMatrix, 0, modelCube, 12);

        float angleY = (float) Math.random() * 80 - 40; // Angle in Y plane, between -40 and 40.
        angleY = (float) Math.toRadians(angleY);
        float newY = (float) Math.tan(angleY) * objectDistance;

        modelPosition[0] = posVec[0];
        modelPosition[1] = newY;
        modelPosition[2] = posVec[2];

        updateModelPosition();
    }

    /**
     * Check if user is looking at object by calculating where the object is in eye-space.
     *
     * @return true if the user is looking at the object.
     */
    public boolean isLookingAtObject() {
        // Convert object space to camera space. Use the headView from onNewFrame.
        Matrix.multiplyMM(modelView, 0, headView, 0, modelCube, 0);
        Matrix.multiplyMV(tempPosition, 0, modelView, 0, POS_MATRIX_MULTIPLY_VEC, 0);

        float pitch = (float) Math.atan2(tempPosition[1], -tempPosition[2]);
        float yaw = (float) Math.atan2(tempPosition[0], -tempPosition[2]);

        return Math.abs(pitch) < PITCH_LIMIT && Math.abs(yaw) < YAW_LIMIT;
    }

    public void onDestroy()
    {
        super.onDestroy();
        NativePart.closeCL();
    }


}
