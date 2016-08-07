package pl.piotr.myvrpet;

import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.Arrays;

/**
 * Created by piotr on 26/06/16.
 */
public class Floor
{
        private FloatBuffer floorVertices;
        private FloatBuffer floorColors;
        private FloatBuffer floorNormals;



        private int floorProgram;



        private int floorPositionParam;
        private int floorNormalParam;
        private int floorColorParam;
        private int floorModelParam;
        private int floorModelViewParam;
        private int floorModelViewProjectionParam;
        private int floorLightPosParam;


    private MainActivity mainActivity;


    public float[] coords;


    public Floor(MainActivity mainActivity, float x, float y)
    {
        this.mainActivity=mainActivity;
        coords = WorldLayoutData.getFloor(x,y);

    }

    public void setCoords(float x, float y)
    {
        coords = WorldLayoutData.getFloor(x,y);
        floorVertices.position(0);
        floorVertices.put(coords);
        floorVertices.position(0);
    }


    public void onSurfaceCreated(int vertexShader, int gridShader)
    {


        // make a floor
        ByteBuffer bbFloorVertices = ByteBuffer.allocateDirect(coords.length * 4);
        bbFloorVertices.order(ByteOrder.nativeOrder());
        floorVertices = bbFloorVertices.asFloatBuffer();
        floorVertices.put(coords);
        floorVertices.position(0);

        ByteBuffer bbFloorNormals = ByteBuffer.allocateDirect(WorldLayoutData.FLOOR_NORMALS.length * 4);
        bbFloorNormals.order(ByteOrder.nativeOrder());
        floorNormals = bbFloorNormals.asFloatBuffer();
        floorNormals.put(WorldLayoutData.FLOOR_NORMALS);
        floorNormals.position(0);

        ByteBuffer bbFloorColors = ByteBuffer.allocateDirect(WorldLayoutData.FLOOR_COLORS.length * 4);
        bbFloorColors.order(ByteOrder.nativeOrder());
        floorColors = bbFloorColors.asFloatBuffer();
        floorColors.put(WorldLayoutData.FLOOR_COLORS);
        floorColors.position(0);





        floorProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(floorProgram, vertexShader);
        GLES20.glAttachShader(floorProgram, gridShader);
        GLES20.glLinkProgram(floorProgram);
        GLES20.glUseProgram(floorProgram);

        MainActivity.checkGLError("Floor program");

        floorModelParam = GLES20.glGetUniformLocation(floorProgram, "u_Model");
        floorModelViewParam = GLES20.glGetUniformLocation(floorProgram, "u_MVMatrix");
        floorModelViewProjectionParam = GLES20.glGetUniformLocation(floorProgram, "u_MVP");
        floorLightPosParam = GLES20.glGetUniformLocation(floorProgram, "u_LightPos");

        floorPositionParam = GLES20.glGetAttribLocation(floorProgram, "a_Position");
        floorNormalParam = GLES20.glGetAttribLocation(floorProgram, "a_Normal");
        floorColorParam = GLES20.glGetAttribLocation(floorProgram, "a_Color");

        MainActivity.checkGLError("Floor program params");
    }




    /**
     * Draw the floor.
     * <p/>
     * <p>This feeds in data for the floor into the shader. Note that this doesn't feed in data about
     * position of the light, so if we rewrite our code to draw the floor first, the lighting might
     * look strange.
     */
    public void drawFloor() {
        GLES20.glUseProgram(floorProgram);

        // Set ModelView, MVP, position, normals, and color.
        GLES20.glUniform3fv(floorLightPosParam, 1, mainActivity.lightPosInEyeSpace, 0);
        GLES20.glUniformMatrix4fv(floorModelParam, 1, false, mainActivity.modelFloor, 0);
        GLES20.glUniformMatrix4fv(floorModelViewParam, 1, false, mainActivity.modelView, 0);
        GLES20.glUniformMatrix4fv(floorModelViewProjectionParam, 1, false, mainActivity.modelViewProjection, 0);
        GLES20.glVertexAttribPointer(
                floorPositionParam, MainActivity.COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, floorVertices);
        GLES20.glVertexAttribPointer(floorNormalParam, 3, GLES20.GL_FLOAT, false, 0, floorNormals);
        GLES20.glVertexAttribPointer(floorColorParam, 4, GLES20.GL_FLOAT, false, 0, floorColors);

        GLES20.glEnableVertexAttribArray(floorPositionParam);
        GLES20.glEnableVertexAttribArray(floorNormalParam);
        GLES20.glEnableVertexAttribArray(floorColorParam);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 24);

        //
        //Log.d("XXX","draw floor x "+mainActivity.modelViewProjection[0]+"  "+mainActivity.modelViewProjection[1]);


        MainActivity.checkGLError("drawing floor");
    }


    public void drawInvertedFloor() {
        GLES20.glUseProgram(floorProgram);

        float tmpProjection[] = new float[16];

        Matrix.invertM(tmpProjection, 0,
                mainActivity.modelViewProjection, 0);

        float tmpModel[] = new float[16];

        Matrix.invertM(tmpProjection, 0,
                mainActivity.modelFloor, 0);

        float tmpView[] = new float[16];

        Matrix.invertM(tmpProjection, 0,
                mainActivity.modelView, 0);

        // Set ModelView, MVP, position, normals, and color.
        GLES20.glUniform3fv(floorLightPosParam, 1, mainActivity.lightPosInEyeSpace, 0);
        GLES20.glUniformMatrix4fv(floorModelParam, 1, false, tmpModel, 0);
        GLES20.glUniformMatrix4fv(floorModelViewParam, 1, false, tmpView, 0);
        GLES20.glUniformMatrix4fv(floorModelViewProjectionParam, 1, false,tmpProjection, 0);
        GLES20.glVertexAttribPointer(
                floorPositionParam, MainActivity.COORDS_PER_VERTEX, GLES20.GL_FLOAT, false, 0, floorVertices);
        GLES20.glVertexAttribPointer(floorNormalParam, 3, GLES20.GL_FLOAT, false, 0, floorNormals);
        GLES20.glVertexAttribPointer(floorColorParam, 4, GLES20.GL_FLOAT, false, 0, floorColors);

        GLES20.glEnableVertexAttribArray(floorPositionParam);
        GLES20.glEnableVertexAttribArray(floorNormalParam);
        GLES20.glEnableVertexAttribArray(floorColorParam);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLES, 0, 24);

        //
        //Log.d("XXX","draw floor x "+mainActivity.modelViewProjection[0]+"  "+mainActivity.modelViewProjection[1]);


        MainActivity.checkGLError("drawing floor");
    }





}
