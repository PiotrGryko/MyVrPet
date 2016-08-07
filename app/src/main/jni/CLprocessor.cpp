#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS /*let's give a chance for OpenCL 1.1 devices*/

#include <CL/cl.hpp>


#include <GLES2/gl2.h>
#include <EGL/egl.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <CL/cl_platform.h>

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

        "  \n" \
        " if(sum.x > 0.5f && sum.y > 0.5f  && sum.z > 0.5f)"\
       "{"\

        "coords[get_global_id(0)*get_global_id(1)]=pos;"\

        "size[0]+=1;"\

        "} "\

        "    write_imagef(imgOut, pos, sum); \n" \

        "} \n";


const char kernel_draw_result[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void kernel_draw_result( \n" \
    "        __write_only image2d_t imgOut, \n" \
     "       __global int2 *coords \n" \

        "    ) { \n" \
    "  \n" \
    "    const int2 pos = coords[get_global_id(0)]; \n" \

        "    write_imagef(imgOut, pos, (float4)(1.0f,0.0f,0.0f,0.0f)); \n" \

        "} \n";


cl::Context theContext;
cl::CommandQueue theQueue;
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

void procOCL_I2I(int texIn, int texOut, int w, int h) {
    if (!haveOpenCL) {
        LOGE("OpenCL isn't initialized");
        return;
    }

    //LOGE("procOCL_I2I(%d, %d, %d, %d)", texIn, texOut, w, h);
    cl::ImageGL imgIn(theContext, CL_MEM_READ_ONLY, GL_TEXTURE_2D, 0, texIn);
    cl::ImageGL imgOut(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texOut);


    std::vector<cl::Memory> images;
    images.push_back(imgIn);
    images.push_back(imgOut);

    theQueue.enqueueAcquireGLObjects(&images);
    theQueue.finish();



    //int coordsY[w]={0};


    cl::Buffer bufferSize(theContext, CL_MEM_READ_WRITE, sizeof(int));
    cl::Buffer bufferCoords(theContext, CL_MEM_READ_WRITE, sizeof(cl_int2) * w * h);
    //cl::Buffer bufferCoordsY(theContext, CL_MEM_READ_WRITE, sizeof(int) * w);



    cl::Kernel edge_detection(edgeDetectionProgram,
                              "kernel_edge_detection"); //TODO: may be done once
    edge_detection.setArg(0, imgIn);
    edge_detection.setArg(1, imgOut);
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




    /*
    for(int i=0;i<w;i++)
    {
        int x = coords[i].s[0];
        int y = coords[i].s[1];


        if(x!=0||y!=0)
         LOGD("%d %d %d ",i,x,y);

//        coordsX[i]=i;
//        coordsY[i]=i;

    }
*/

    ///////////////////////////////


    //cl::Buffer bufferResult(theContext, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(int) * w,&coords,NULL);
    //cl::Buffer bufferResultY(theContext, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, sizeof(int) * w,&coordsY,NULL);

/*


    cl::Kernel draw_result(drawResultProgram, "kernel_draw_result"); //TODO: may be done once
    draw_result.setArg(0, imgOut);
    draw_result.setArg(1, bufferCoords);
    //draw_result.setArg(2, bufferResultY);




    theQueue.finish();

    theQueue.enqueueNDRangeKernel(draw_result, cl::NullRange, cl::NDRange(w * h), cl::NullRange);
    theQueue.finish();

*/



    ////////////////////

    /*
    for (int i = 0; i < w; i++) {


        int x = coords[i].s[0];
        int y = coords[i].s[1];
        if ((x != 0 || y != 0) && i!=0)
            LOGD("%d, %d %d ", i, x, y);
    }
     */



    theQueue.enqueueReleaseGLObjects(&images);
    theQueue.finish();


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
