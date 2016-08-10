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


void dumpCLinfo() {
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

//sobel
/*
const char kernel_edge_detection[] = \
  "__constant sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void kernel_edge_detection( \n" \
    "        __read_only image2d_t imgIn, \n" \
    "        __write_only image2d_t imgOut, \n" \

        "       __global int2 *coords, \n" \


        "       __global int *size \n" \



        "    ) { \n" \
    "  \n" \
    "    const int2 coord = {get_global_id(0), get_global_id(1)}; \n" \
    "  \n" \



        "uint4 val1=read_imageui(imgIn, smp, (int2)(coord.x-1,coord.y-1));\n"
        "uint4 val2=read_imageui(imgIn, smp, (int2)(coord.x,coord.y-1));\n"
        "uint4 val3=read_imageui(imgIn, smp, (int2)(coord.x+1,coord.y-1));\n"
        "uint4 val4=read_imageui(imgIn, smp, (int2)(coord.x-1,coord.y));\n"
        "uint4 val5=read_imageui(imgIn, smp, (int2)(coord.x,coord.y));\n"
        "uint4 val6=read_imageui(imgIn, smp, (int2)(coord.x+1,coord.y));\n"
        "uint4 val7=read_imageui(imgIn, smp, (int2)(coord.x-1,coord.y+1));\n"
        "uint4 val8=read_imageui(imgIn, smp, (int2)(coord.x,coord.y+1));\n"
        "uint4 val9=read_imageui(imgIn, smp, (int2)(coord.x+1,coord.y+1));\n"


             "int x1=-1*val1.x+0*val2.x+1*val3.x+-2*val4.x+2*val6.x+-1*val7.x+0*val8.x+1*val9.x;\n"

             "int y1=-1*val1.y+0*val2.y+1*val3.y+-2*val4.y+2*val6.y+-1*val7.y+0*val8.y+1*val9.y;\n"

             "int z1=-1*val1.z+0*val2.z+1*val3.z+-2*val4.z+2*val6.z+-1*val7.z+0*val8.z+1*val9.z;\n"

             "int x2=1*val1.x+2*val2.x+1*val3.x+0*val4.x+0*val6.x+-1*val7.x+-2*val8.x+-1*val9.x;\n"

             "int y2=1*val1.y+2*val2.y+1*val3.y+0*val4.y+0*val6.y+-1*val7.y+-2*val8.y+-1*val9.y;\n"
             "int z2=1*val1.z+2*val2.z+1*val3.z+0*val4.z+0*val6.z+-1*val7.z+-2*val8.z+-1*val9.z;\n"

                 "uint4 val = (uint4)0;"
             "val.x=sqrt((float)(x1*x1+x2*x2));\n"
             "val.y=sqrt((float)(y1*y1+y2*y2));\n"
             "val.z=sqrt((float)(z1*z1+z2*z2));"


             "    write_imageui(imgOut, coord, val); \n" \

        "} \n";
*/

//laplacian
const char kernel_edge_detection[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void kernel_edge_detection( \n" \
    "        __read_only image2d_t imgIn, \n" \
    "        __write_only image2d_t imgOut, \n" \

        "       __global int2 *coords, \n" \


        "       __global int *size \n" \



        "    ) { \n" \
    "  \n" \
    "    const int2 pos = {get_global_id(0), get_global_id(1)}; \n" \
    "  \n" \
    "    float4 sum = (float4) 0.0f; \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(+1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,-1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,+1)); \n" \
    "    sum -= read_imagef(imgIn, sampler, pos) * 4; \n" \
    "    sum *= 10; \n" \


        " if(sum.x > 0.3f && sum.y > 0.3f  && sum.z > 0.3f)"\
       "{"\

        "coords[get_global_id(0)*get_global_id(1)]=pos;"\

        "size[0]+=1;"\

        "} "\

        "    write_imagef(imgOut, pos, sum); \n" \

        "} \n";

/*
const char kernel_blur[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;"\
    "__kernel void gaussian_blur("\
     "      __read_only image2d_t image,"\
      "     __constant float * mask,"\
       "    __global float * blurredImage,"\
        "   __private int maskSize"\
      " ) {"\

       "const int2 pos = {get_global_id(0), get_global_id(1)};"\

       // Collect neighbor values and multiply with Gaussian
       "float sum = 0.0f;"\
       "for(int a = -maskSize; a < maskSize+1; a++) {"\
        "   for(int b = -maskSize; b < maskSize+1; b++) {"\
        "       sum += mask[a+maskSize+(b+maskSize)*(maskSize*2+1)]"\
        "           *read_imagef(image, sampler, pos + (int2)(a,b)).x;"\
        "   }"\
       "}"\

       "blurredImage[pos.x+pos.y*get_global_size(0)] = sum;"\
   "}";
*/

const char kernel_blur[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;"\
    "__kernel void kernel_blur("\
     "      __read_only image2d_t imgIn,"\
      "        __write_only image2d_t imgOut,"\
      "     __constant float * mask,"\
        "   __private int maskSize"\
      " ) {"\

        "const int2 pos = {get_global_id(0), get_global_id(1)};"\

 "    float4 sum = (float4) 0.0f; \n" \
   /*
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(+1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,-1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,+1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos); \n" \
    "    sum /=5; \n"\
*/
 /*

       // Collect neighbor values and multiply with Gaussian
        "    float4 sum = (float4) 0.0f; \n" \
       "for(int a = -maskSize; a < maskSize+1; a++) {"\
            "   for(int b = -maskSize; b < maskSize+1; b++) {"\
        "       sum += mask[a+maskSize+(b+maskSize)*(maskSize*2+1)]"\
        "           *read_imagef(imgIn, sampler, pos + (int2)(a,b)).x;"\
        "   }"\
       "}"\
*/

    "    sum += read_imagef(imgIn, sampler, pos); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,-1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,-1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(1,-1)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(1,0)); \n" \
    "    sum += read_imagef(imgIn, sampler, pos + (int2)(1,1)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,-2)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,-2)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(1,-2)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-1,2)); \n" \
 //   "    sum += read_imagef(imgIn, sampler, pos + (int2)(0,2)); \n" \
 //   "    sum += read_imagef(imgIn, sampler, pos + (int2)(1,2)); \n" \
 //   "    sum += read_imagef(imgIn, sampler, pos + (int2)(-2,-2)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-2,1)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-2,0)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-2,1)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(-2,2)); \n" \
//    "    sum += read_imagef(imgIn, sampler, pos + (int2)(2,-2)); \n" \
 //   "    sum +=read_imagef(imgIn, sampler, pos + (int2)(2,-1)); \n" \
 //   "    sum +=read_imagef(imgIn, sampler, pos + (int2)(2,0)); \n" \
 //   "    sum +=read_imagef(imgIn, sampler, pos + (int2)(2,1)); \n" \
 //   "    sum +=read_imagef(imgIn, sampler, pos + (int2)(2,2)); \n" \

    "    sum/=6;"







"    write_imagef(imgOut, pos, sum); \n" \

        "}";


const char kernel_draw_result[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void kernel_draw_result( \n" \
    "        __write_only image2d_t imgOut, \n" \
     "       __global int2 *coords \n" \

        "    ) { \n" \
    "    const int2 pos = coords[get_global_id(0)]; \n" \

   //     "const int2 pos = {get_global_id(0), get_global_id(1)};"\

        "    write_imagef(imgOut, pos, (float4)(1.0f,0.0f,0.0f,0.0f)); \n" \

        "} \n";


cl::Context theContext;
cl::CommandQueue theQueue;
cl::Program blurrProgram;

cl::Program edgeDetectionProgram;
cl::Program drawResultProgram;

bool haveOpenCL = false;

extern "C" void initCL() {
    dumpCLinfo();

    EGLDisplay mEglDisplay = eglGetCurrentDisplay();
    if (mEglDisplay == EGL_NO_DISPLAY)
        LOGE("initCL: eglGetCurrentDisplay() returned 'EGL_NO_DISPLAY', error = %x", eglGetError());

    EGLContext mEglContext = eglGetCurrentContext();
    if (mEglContext == EGL_NO_CONTEXT)
        LOGE("initCL: eglGetCurrentContext() returned 'EGL_NO_CONTEXT', error = %x", eglGetError());

    cl_context_properties props[] =
            {CL_GL_CONTEXT_KHR, (cl_context_properties) mEglContext,
             CL_EGL_DISPLAY_KHR, (cl_context_properties) mEglDisplay,
             CL_CONTEXT_PLATFORM, 0,
             0};

    try {
        haveOpenCL = false;
        cl::Platform p = cl::Platform::getDefault();
        std::string ext = p.getInfo<CL_PLATFORM_EXTENSIONS>();
        if (ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by PLATFORM");
        props[5] = (cl_context_properties) p();


        theContext = cl::Context(CL_DEVICE_TYPE_GPU, props);
        std::vector<cl::Device> devs = theContext.getInfo<CL_CONTEXT_DEVICES>();
        LOGD("Context returned %d devices, taking the 1st one", devs.size());
        ext = devs[0].getInfo<CL_DEVICE_EXTENSIONS>();
        if (ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by DEVICE");


        theQueue = cl::CommandQueue(theContext, devs[0]);

        cl::Program::Sources mainSrc(1, std::make_pair(kernel_edge_detection,
                                                       sizeof(kernel_edge_detection)));
        edgeDetectionProgram = cl::Program(theContext, mainSrc);
        edgeDetectionProgram.build(devs);


        cl::Program::Sources resultSrc(1, std::make_pair(kernel_draw_result,
                                                         sizeof(kernel_draw_result)));
        drawResultProgram = cl::Program(theContext, resultSrc);
        drawResultProgram.build(devs);

        cl::Program::Sources blurSrc(1, std::make_pair(kernel_blur,
                                                       sizeof(kernel_blur)));
        blurrProgram = cl::Program(theContext, blurSrc);
        blurrProgram.build(devs);


        haveOpenCL = true;
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


void procOCL_I2I(int texIn, int texOut, int w, int h) {
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

    theQueue.enqueueNDRangeKernel(blur, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();



    //   theQueue.enqueueReleaseGLObjects(&images);
 //   theQueue.finish();

    /*
       std::string result = "";
       std::ostringstream converta;   // stream used for the conversion
       std::ostringstream convertb;   // stream used for the conversion
    std::ostringstream convertc;

       int count = 0;


       LOGD("elo 444444444444444444444444444444444444444444444444444444444444");

       for (int a = -maskSize; a < maskSize + 1; a++) {
           for (int b = -maskSize; b < maskSize + 1; b++) {
               converta << a;
               convertb << b;
               convertc <<a+maskSize+(b+maskSize)*(maskSize*2+1);

               result += "sum += mask["+convertc.str()+"]*read_imagef(imgIn, sampler, pos + (int2)(" + converta.str() +
                         "," + convertb.str() + "));\n";
               converta.str("");
               converta.clear();
               convertb.str("");
               convertb.clear();
               convertc.str("");
               convertc.clear();

               if (count == 5) {
                   LOGD("elo \n%s", result.c_str());
                   count = 0;
                   result = "";

               }
               count++;

           }
       }
       LOGD("elo\n%s", result.c_str());
       LOGD("elo 444444444444444444444444444444444444444444444444444444444444");
*/
///
    // printf("elo %s ",result.c_str());
//__android_log_print(ANDROID_LOG_DEBUG, "LOG_TAG", "Need to print : %s",result.c_str());




    ///////////////////////////////










    //LOGE("procOCL_I2I(%d, %d, %d, %d)", texIn, texOut, w, h);
   // cl::ImageGL imgIn(theContext, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texIn);
   /// cl::ImageGL imgOut(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texOut);


   // std::vector<cl::Memory> images;
   // images.push_back(imgIn);
  //  images.push_back(imgOut);

 //   theQueue.enqueueAcquireGLObjects(&images);
  //  theQueue.finish();



    //int coordsY[w]={0};


    cl::Buffer bufferSize(theContext, CL_MEM_READ_WRITE, sizeof(int));
    cl::Buffer bufferCoords(theContext, CL_MEM_READ_WRITE, sizeof(cl_int2) * w * h);
    //cl::Buffer bufferCoordsY(theContext, CL_MEM_READ_WRITE, sizeof(int) * w);



    cl::Kernel edge_detection(edgeDetectionProgram,
                              "kernel_edge_detection"); //TODO: may be done once
    edge_detection.setArg(0, imgOutBlurr);
    edge_detection.setArg(1, imgInBlurr);
    edge_detection.setArg(2, bufferCoords);
    //edge_detection.setArg(3, bufferCoordsY);

    edge_detection.setArg(3, bufferSize);


    theQueue.finish();

    theQueue.enqueueNDRangeKernel(edge_detection, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();

    int size[] = {0};
    theQueue.enqueueReadBuffer(bufferSize, CL_TRUE, 0, sizeof(int), size);
    cl_int2 coords[size[0]];
    theQueue.enqueueReadBuffer(bufferCoords, CL_TRUE, 0, sizeof(cl_int2) * size[0], coords);
    //theQueue.enqueueReadBuffer(bufferCoordsY, CL_TRUE, 0, sizeof(int) * w, coordsY);



    theQueue.finish();


    theQueue.enqueueReleaseGLObjects(&images);
    theQueue.finish();



    /////////////////////////////////////////////




/*

    cl::Kernel draw_result(drawResultProgram, "kernel_draw_result"); //TODO: may be done once
    draw_result.setArg(0, imgInBlurr);
    draw_result.setArg(1, bufferCoords);
    //draw_result.setArg(2, bufferResultY);




    theQueue.finish();

    theQueue.enqueueNDRangeKernel(draw_result, cl::NullRange, cl::NDRange(w * h), cl::NullRange);
    theQueue.finish();
*/







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



extern "C" void processFrame(int tex1, int tex2, int w, int h, int mode) {
    //LOGD("processing mode %d", mode);


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

//    drawFrameProcCPU(w, h, tex2);


}
