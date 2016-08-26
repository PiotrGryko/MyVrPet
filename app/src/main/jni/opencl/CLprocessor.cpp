#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS /*let's give a chance for OpenCL 1.1 devices*/

#include <CL/cl.hpp>


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


const char kernel_radial_sweep[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
  "__kernel void kernel_radial_sweep( \n" \
  "__global int2 *output, \n" \
  "__global int2 *coords, \n" \
  "__write_only image2d_t imgOut"\
  ") { \n" \
  "const int2 pos = coords[get_global_size(1)*get_global_id(0)+get_global_id(1)]; \n" \
  "write_imagef(imgOut, pos, (float4)(1.0f,0.0f,0.0f,0.0f)); \n" \

        "const int2 center = (int2)(600,360);"\
  "for(int i=0;i<10;i++){"
        "const int angle = 36*i;"\
  "const float PI = 3.14;"
        "const float radians = angle*PI/180.0f;"
        "const float a = tan(radians);"
        "float n = center.y - a * center.x;"


        "float equation = pos.x * a+n;"
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

        "} \n" \
        "}"\
        "} \n";


const char kernel_draw_radial_sweep[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \

        "__kernel void kernel_draw_radial_sweep( \n" \
    "__write_only image2d_t imgOut,"\
    "__global int2 *edges \n" \
    "    ) { \n" \
    "    const int2 pos =(int2)(get_global_id(0),get_global_id(1)); \n" \
    "const int2 center = (int2)(600,360);"\
    "for(int i=0;i<10;i++){"
        "const int angle =36*i;"\
        "const float PI = 3.14;"
        "const float radians = angle*PI/180.0f;"
        "const float a = tan(radians);"
        "float n = center.y - a * center.x;"

        "float equation = pos.x * a+n;"
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

        "else if(pos.x>=edges[i].x && angle>135 && angle <225)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if(angle>225 && angle <315 && pos.y>=edges[i].y)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"

        "else if((angle>315 || (angle>=0 && angle <=45)) && pos.x<=edges[i].x)"
        "write_imagef(imgOut, pos, (float4)(1.0f,1.0f,1.0f,0.0f));"


        "}"\
        "} \n" \
        "}"
        "} \n";


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


void procOCL_I2I(int texIn, int texOut, int w, int h, int output[18][2]) {
    cl_int2 result[18];

    if (!haveOpenCL) {
        LOGE("OpenCL isn't initialized");
        return;
    }


    cl::ImageGL imgInBlurr(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texIn);
    cl::ImageGL imgOutBlurr(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texOut);


    std::vector<cl::Memory> images;
    images.push_back(imgInBlurr);
    images.push_back(imgOutBlurr);

    theQueue.enqueueAcquireGLObjects(&images);
    theQueue.finish();


    int maskSize;
    float *mask = createBlurMask(1.5f, &maskSize);




    //maskSize=1;
    // Create buffer for mask and transfer it to the device
    cl::Buffer clMask(theContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                      sizeof(float) * (maskSize * 2 + 1) * (maskSize * 2 + 1), mask);


    cl::Kernel blur(blurrProgram, "kernel_blur"); //TODO: may be done once
    blur.setArg(0, imgInBlurr);
    blur.setArg(1, imgOutBlurr);
    blur.setArg(2, clMask);
    blur.setArg(3, maskSize);


    theQueue.finish();

    //theQueue.enqueueNDRangeKernel(blur, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();


    cl::Buffer bufferSize(theContext, CL_MEM_READ_WRITE, sizeof(long));
    cl::Buffer bufferCoords(theContext, CL_MEM_READ_WRITE, sizeof(cl_int2) * w * h);

    cl::Kernel edge_detection(edgeDetectionProgram,
                              "kernel_edge_detection");
    edge_detection.setArg(0, imgInBlurr);
    edge_detection.setArg(1, imgOutBlurr);
    edge_detection.setArg(2, bufferCoords);
    edge_detection.setArg(3, bufferSize);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(edge_detection, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();

    long size[] = {0};
    theQueue.enqueueReadBuffer(bufferSize, CL_TRUE, 0, sizeof(long), size);


    cl::Buffer outputCoords(theContext, CL_MEM_READ_WRITE,
                            sizeof(cl_int2) * 18, NULL);

    cl::Kernel radial_sweep(radialSweepProgram, "kernel_radial_sweep"); //TODO: may be done once
    radial_sweep.setArg(0, outputCoords);
    radial_sweep.setArg(1, bufferCoords);
    radial_sweep.setArg(2, imgOutBlurr);

    theQueue.finish();

    theQueue.enqueueNDRangeKernel(radial_sweep, cl::NullRange, cl::NDRange(w, h),
                                  cl::NullRange);
    theQueue.finish();

    theQueue.enqueueReadBuffer(outputCoords, CL_TRUE, 0, sizeof(cl_int2) * 18, result);



    cl::Kernel test_radial_sweep(drawRadialSweepProgram,
                                 "kernel_draw_radial_sweep"); //TODO: may be done once
    test_radial_sweep.setArg(0, imgOutBlurr);
    test_radial_sweep.setArg(1, outputCoords);
    theQueue.finish();

    theQueue.enqueueNDRangeKernel(test_radial_sweep, cl::NullRange, cl::NDRange(w, h),
                                  cl::NullRange);


    theQueue.finish();


    for(int i=0;i<18;i++)
    {
        output[i][0]=result[i].s[0];
        output[i][1]=result[i].s[1];

    }

    return;

}


void drawFrameProcCPU(int w, int h, int texOut) {
    LOGD("Processing on CPU");
    int64_t t;

    // let's modify pixels in FBO texture in C++ code (on CPU)
    static cv::Mat m;
    m.create(h, w, CV_8UC4);

    // read
    t = getTimeMs();
    // expecting FBO to be bound
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m.data);
    LOGD("glReadPixels() costs %d ms", getTimeInterval(t));


    /*
   // modify
    t = getTimeMs();
    cv::Laplacian(m, m, CV_8U);
    m *= 10;
    LOGD("Laplacian() costs %d ms", getTimeInterval(t));
*/
    // write back
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texOut);
    // t = getTimeMs();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, m.data);
    LOGD("glTexSubImage2D() costs %d ms", getTimeInterval(t));
}


enum ProcMode {
    PROC_MODE_NO_PROC = 0, PROC_MODE_CPU = 1, PROC_MODE_OCL_DIRECT = 2, PROC_MODE_OCL_OCV = 3
};


