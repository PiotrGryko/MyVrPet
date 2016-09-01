#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS /*let's give a chance for OpenCL 1.1 devices*/



#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <CL/cl_platform.h>
#include <iostream>
#include <android/log.h>
#include <sstream>

#include "CLcommon.hpp"
#include "CLimpl.hpp"


void dumpCLInfo() {
    LOGD("*** OpenCL info ***");
    try {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        LOGD("OpenCL info: Found %d OpenCL platforms", platforms.size());
        for (int i = 0; i < platforms.size(); ++i) {
            std::string name = platforms[i].getInfo<CL_PLATFORM_NAME>();
            std::string version = platforms[i].getInfo<CL_PLATFORM_VERSION>();
            std::string profile = platforms[i].getInfo<CL_PLATFORM_PROFILE>();
            std::string extensions = platforms[i].getInfo<CL_PLATFORM_EXTENSIONS>();
            LOGD("OpenCL info: Platform[%d] = %s, ver = %s, prof = %s, ext = %s",
                 i, name.c_str(), version.c_str(), profile.c_str(), extensions.c_str());
        }

        std::vector<cl::Device> devices;
        platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        for (int i = 0; i < devices.size(); ++i) {
            std::string name = devices[i].getInfo<CL_DEVICE_NAME>();
            std::string extensions = devices[i].getInfo<CL_DEVICE_EXTENSIONS>();
            cl_ulong type = devices[i].getInfo<CL_DEVICE_TYPE>();
            LOGD("OpenCL info: Device[%d] = %s (%s), ext = %s",
                 i, name.c_str(), (type == CL_DEVICE_TYPE_GPU ? "GPU" : "CPU"), extensions.c_str());
        }
    }
    catch (cl::Error &e) {
        LOGE("OpenCL info: error while gathering OpenCL info: %s (%d)", e.what(), e.err());
    }
    catch (std::exception &e) {
        LOGE("OpenCL info: error while gathering OpenCL info: %s", e.what());
    }
    catch (...) {
        LOGE("OpenCL info: unknown error while gathering OpenCL info");
    }
    LOGD("*******************");
}


const char kernel_blur[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;"\
  "__kernel void kernel_blur("\
  "__read_only image2d_t imgIn,"\
  "__write_only image2d_t imgOut,"\
  "__constant float * mask,"\
  "__private int maskSize"\
  " ) {"\
  "const int2 pos = {get_global_id(0), get_global_id(1)};"\
  "float4 sum = (float4) 0.0f;"\

        "sum += read_imagef(imgIn, sampler, pos); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(-1,-1)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(-1,0)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(-1,1)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(0,-1)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(0,1)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(1,-1)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(1,0)); \n" \
  "sum += read_imagef(imgIn, sampler, pos + (int2)(1,1)); \n" \
  "sum/=6;"

        "write_imagef(imgOut, pos, sum); \n" \
  "}";

//laplacian
const char kernel_edge_detection[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "__kernel void kernel_edge_detection( \n" \
    "__read_only image2d_t imgIn, \n" \
    "__write_only image2d_t imgOut, \n" \
    "__global int2 *coords, \n" \
    "__global long *size \n" \
    ") { \n" \
    "const int2 pos = {get_global_id(0), get_global_id(1)}; \n" \
    "float4 sum = (float4) 0.0f; \n" \
    "sum += read_imagef(imgIn, sampler, pos + (int2)(-1,0)); \n" \
    "sum += read_imagef(imgIn, sampler, pos + (int2)(+1,0)); \n" \
    "sum += read_imagef(imgIn, sampler, pos + (int2)(0,-1)); \n" \
    "sum += read_imagef(imgIn, sampler, pos + (int2)(0,+1)); \n" \
    "sum -= read_imagef(imgIn, sampler, pos) * 4; \n" \
    "sum *= 10; \n" \

        " if(sum.x > 0.95f && sum.y > 0.95f  && sum.z > 0.95f)"\
    "{"\
    "coords[get_global_size(1)*get_global_id(0)+get_global_id(1)]=pos;"\
    "} "\

        "write_imagef(imgOut, pos, (float4)(0.0f,0.0f,0.0f,0.0f)); \n" \
    "} \n";



/**
 *
 * line equation is ax+b=y
 * pass cl_int2 buffer where for each n n.x=a and n.y =b
 *
 */



/**
 *
 * This methods gets array of lines and array of points (detected lines) as an input.
 * Returns array of nearest points to center that lays on each line.
 *
 * output = array to save results
 * coors = array of points/detected edges
 * imgOut = img to draw someting on. Temporary should be removed
 * center = center point for radial swip
 * lines =array of cl_float4. a & b from y=ax+b, xCoord and yCoord. Which indicate line direction
 * size = number of lines
 */

const char kernel_radial_sweep[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
  "__kernel void kernel_radial_sweep(" \
  "__global int2 *output," \
  "__global int2 *coords," \
  "__write_only image2d_t imgOut,"\
  "__private int2 center,"
  "__global float3 *lines,"
  "__private int size"
  ") {" \

  "const int2 pos = coords[get_global_size(1)*get_global_id(0)+get_global_id(1)]; \n" \
  "write_imagef(imgOut, pos, (float4)(1.0f,0.0f,0.0f,0.0f)); \n" \

  "for(int i=0;i<size;i++){"

        "const float a = lines[i].x;"
        "const float b = lines[i].y;"
        "const int angle = (int)lines[i].z;"

        "float equation = pos.x * a+b;"

        "if(equation<=pos.y+15 && equation >=pos.y-15){"


        "if((pos.x>=center.x && pos.y>=center.y && angle>=0 && angle <90)"
        "||(pos.x<=center.x && pos.y>=center.y && angle>=90 && angle <180)"
        "||(pos.x<=center.x && pos.y<=center.y && angle>=180 && angle <270)"
        "||(pos.x>=center.x && pos.y<=center.y && angle>=270 && angle <360)"
        "){"


        "if(output[i].x==0 && output[i].y==0)"
        "output[i]=pos;"

        "else if(angle>45 && angle <=135 && pos.y<=output[i].y)"
        "output[i]=pos;"

        "else if(pos.x>=output[i].x && angle>135 && angle <225)"
        "output[i]=pos;"

        "else if(angle>225 && angle <315 && pos.y>=output[i].y)"
        "output[i]=pos;"

        "else if((angle>315 || (angle>=0 && angle <=45)) && pos.x<=output[i].x)"
        "output[i]=pos;"

        "}"


        "}" \

        "}"\



        "}";



/**
 *
 * line quation is ax+b=y
 * pass cl_int2 buffer where for each n n.x=a and n.y =b
 * all lines start from center point
 *
 * img_out = image to draw the result
 * center = center of radial sweep
 * edges = array of points from edge detection alghoritm
 * lines =array of cl_float4. a & b from y=ax+b, xCoord and yCoord. Which indicate line direction
 * size = number of lines
 */




const char kernel_draw_radial_sweep[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \

    "__kernel void kernel_draw_radial_sweep("
    "__write_only image2d_t imgOut,"
    "__private int2 center,"
    "__global int2 *edges,"
    "__global float3 *lines,"
    "__private int size) {"


    "const int2 pos =(int2)(get_global_id(0),get_global_id(1));" \

        "for(int i=0;i<size;i++){"

        "const float a = lines[i].x;"
        "const float b = lines[i].y;"
        "const int angle = (int)lines[i].z;"
        "const float equation = pos.x * a+b;"



        "if(equation<=pos.y+1 && equation >=pos.y-1){"


        "if((pos.x>=center.x && pos.y>=center.y && angle>=0 && angle <90)"
        "||(pos.x<=center.x && pos.y>=center.y && angle>=90 && angle <180)"
        "||(pos.x<=center.x && pos.y<=center.y && angle>=180 && angle <270)"
        "||(pos.x>=center.x && pos.y<=center.y && angle>=270 && angle <360)"
        "){"


        "if(edges[i].x==0 && edges[i].y==0)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if(angle>45 && angle <=135 && pos.y<=edges[i].y)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if(pos.x>=edges[i].x && angle>135 && angle <=225)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if(angle>225 && angle <=315 && pos.y>=edges[i].y)"
       "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if((angle>315 || angle <=45) && pos.x<=edges[i].x)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"




        "}"

        "}"
        "}"
        "}";



cl::Context theContext;
cl::CommandQueue theQueue;
cl::Program blurrProgram;

cl::Program edgeDetectionProgram;
cl::Program radialSweepProgram;
cl::Program drawRadialSweepProgram;


bool haveOpenCL = false;

extern "C" void initCL() {
    dumpCLInfo();



    try {
        haveOpenCL = false;

        theContext = initOpenCL();
        std::vector<cl::Device> devs = theContext.getInfo<CL_CONTEXT_DEVICES>();

        theQueue=initQueue();



        cl::Program::Sources mainSrc(1, std::make_pair(kernel_edge_detection,
                                                       sizeof(kernel_edge_detection)));
        edgeDetectionProgram = cl::Program(theContext, mainSrc);
        edgeDetectionProgram.build(devs);



        cl::Program::Sources resultSrc(1, std::make_pair(kernel_radial_sweep,
                                                         sizeof(kernel_radial_sweep)));
        radialSweepProgram = cl::Program(theContext, resultSrc);
        radialSweepProgram.build(devs);

        cl::Program::Sources blurSrc(1, std::make_pair(kernel_blur,
                                                       sizeof(kernel_blur)));
        blurrProgram = cl::Program(theContext, blurSrc);
        blurrProgram.build(devs);


        cl::Program::Sources testRadialSrc(1, std::make_pair(kernel_draw_radial_sweep,
                                                             sizeof(kernel_draw_radial_sweep)));
        drawRadialSweepProgram = cl::Program(theContext, testRadialSrc);
        drawRadialSweepProgram.build(devs);


        haveOpenCL= true;
    }
    catch (cl::Error &e) {
        LOGE("cl::Error: %s (%d)", e.what(), e.err());
    }
    catch (std::exception &e) {
        LOGE("std::exception: %s", e.what());
    }
    catch (...) {
        LOGE("OpenCL info: unknown error while initializing OpenCL stuff");
    }
    LOGD("initCL completed");
}

extern "C" void closeCL() {
}


#define GL_TEXTURE_2D 0x0DE1
float *createBlurMask(float sigma, int *maskSizePointer) {
    int maskSize = (int) ceil(3.0f * sigma);
    float *mask = new float[(maskSize * 2 + 1) * (maskSize * 2 + 1)];
    float sum = 0.0f;
    for (int a = -maskSize; a < maskSize + 1; a++) {
        for (int b = -maskSize; b < maskSize + 1; b++) {
            float temp = exp(-((float) (a * a + b * b) / (2 * sigma * sigma)));
            sum += temp;
            mask[a + maskSize + (b + maskSize) * (maskSize * 2 + 1)] = temp;
        }
    }
    // Normalize the mask
    for (int i = 0; i < (maskSize * 2 + 1) * (maskSize * 2 + 1); i++)
        mask[i] = mask[i] / sum;
    //mask[i] = 0.1f;
    *maskSizePointer = maskSize;

    return mask;
}


static void clBlurrImage(cl::ImageGL imgIn, cl::ImageGL imgOut, int w, int h)
{

    int maskSize;
    float *mask = createBlurMask(1.5f, &maskSize);

    cl::Buffer clMask(theContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                      sizeof(float) * (maskSize * 2 + 1) * (maskSize * 2 + 1), mask);


    cl::Kernel blur(blurrProgram, "kernel_blur"); //TODO: may be done once
    blur.setArg(0, imgIn);
    blur.setArg(1, imgOut);
    blur.setArg(2, clMask);
    blur.setArg(3, maskSize);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(blur, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();
}

static cl::Buffer clEdgeDetection(cl::ImageGL imgIn, cl::ImageGL imgOut, int w, int h)
{

    cl::Buffer bufferSize(theContext, CL_MEM_READ_WRITE, sizeof(long));
    cl::Buffer bufferCoords(theContext, CL_MEM_READ_WRITE, sizeof(cl_int2) * w * h);

    cl::Kernel edge_detection(edgeDetectionProgram,
                              "kernel_edge_detection");
    edge_detection.setArg(0, imgIn);
    edge_detection.setArg(1, imgOut);
    edge_detection.setArg(2, bufferCoords);
    edge_detection.setArg(3, bufferSize);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(edge_detection, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();
    return bufferCoords;
}



static cl::Buffer clRadialSweep(cl::ImageGL imgOut, int w, int h, cl::Buffer bufferCoords,cl_int2 center, cl::Buffer lines,int size)
{
/**
 *
 * This methods gets array of lines and array of points (detected lines) as an input.
 * Returns array of nearest points to center that lays on each line.
 *
 * output = array to save results
 * coors = array of points/detected edges
 * imgOut = img to draw someting on. Temporary should be removed
 * center = center point for radial swip
 * lines =array of cl_float4. a & b from y=ax+b, xCoord and yCoord. Which indicate line direction
 * size = number of lines
 */


    cl::Buffer outputCoords(theContext, CL_MEM_READ_WRITE,
                            sizeof(cl_int2) * 18, NULL);

    cl::Kernel radial_sweep(radialSweepProgram, "kernel_radial_sweep"); //TODO: may be done once
    radial_sweep.setArg(0, outputCoords);
    radial_sweep.setArg(1, bufferCoords);
    radial_sweep.setArg(2, imgOut);
    radial_sweep.setArg(3, center);
    radial_sweep.setArg(4, lines);
    radial_sweep.setArg(5, size);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(radial_sweep, cl::NullRange, cl::NDRange(w, h),
                                  cl::NullRange);
    theQueue.finish();
    return outputCoords;
}

/**
 *
 * line quation is ax+b=y
 * pass cl_int2 buffer where for each n n.x=a and n.y =b
 * all lines start from center point
 *
 * img_out = image to draw the result
 * center = center of radial sweep
 * edges = array of points from edge detection alghoritm
 * lines = array of line equations
 * size = number of lines
 */

static void clDrawLinesTwo(cl::ImageGL imgOut, int w, int h,cl_int2 center, cl::Buffer outputCoords,cl_int2 *ends, cl::Buffer lines,int size)
{




    cl::Kernel test_radial_sweep(drawRadialSweepProgram,
                                 "kernel_draw_radial_sweep"); //TODO: may be done once
    test_radial_sweep.setArg(0, imgOut);
    test_radial_sweep.setArg(1, center);
    test_radial_sweep.setArg(2, outputCoords);
    test_radial_sweep.setArg(3, lines);
    test_radial_sweep.setArg(4, size);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(test_radial_sweep, cl::NullRange, cl::NDRange(w, h),
                                  cl::NullRange);


    theQueue.finish();
}




static void clPrepareLines(cl_int2 center, cl_float2 *points, int size, cl_float3 *linesOutput)
{


/**
 *
 * Takes center starting point, and array of end points.
 * Returns representation of lines that lays on center and each end point.
 * Each item from return array = {a,b,angle}, where a & b comes from y=ax+b
 *
 *
 */

    float PI = 3.14;

    for(int i=0;i<size;i++)
    {

        float dx  = points[i].s[0]-center.s[0];
        float dy = points[i].s[1]-center.s[1];
        float rads = atan2f(dy,dx);
        float degress = rads*180.0f/PI;
        if(degress<0)
            degress=360+degress;

        float a= (center.s[1]-points[i].s[1])/(center.s[0]-points[i].s[0]);
        float b= points[i].s[1]-(points[i].s[0] * (center.s[1]-points[i].s[1])/(center.s[0]-points[i].s[0]));


        linesOutput[i]={a,b,degress};

        LOGD("prepare lines %d %d %d %f %f %f %f %f ",i,center.s[0],center.s[1],points[i].s[0],points[i].s[1],degress,a,b);
    }

}

cl_int2* procOCL_I2I(int texIn, int texOut, int w, int h, int fanSize, cl_int2 center,cl_float2 *linesPoints, bool debug) {
    cl_int2 result[fanSize];

    if (!haveOpenCL) {
        LOGE("OpenCL isn't initialized");
        return result;
    }


    cl::ImageGL imgIn(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texIn);
    cl::ImageGL imgOut(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texOut);


    std::vector<cl::Memory> images;
    images.push_back(imgIn);
    images.push_back(imgOut);
    theQueue.enqueueAcquireGLObjects(&images);
    theQueue.finish();

    cl::Buffer bufferCoords = clEdgeDetection(imgIn,imgOut,w,h);

    cl_float3 lines[fanSize];
    cl_int2 ends[fanSize];



    clPrepareLines(center,linesPoints,fanSize,lines);


    cl::Buffer linesBuffer(theContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           sizeof(cl_float4) * fanSize, lines);

    cl::Buffer outputCoords = clRadialSweep(imgOut,w,h,bufferCoords,center,linesBuffer,fanSize);
    if(debug)
    clDrawLinesTwo(imgOut,w,h,center,outputCoords,ends,linesBuffer,fanSize);


    theQueue.enqueueReleaseGLObjects(&images);
    theQueue.finish();

    theQueue.enqueueReadBuffer(outputCoords, CL_TRUE, 0, sizeof(cl_int2) * 18, result);



    return result;

}




