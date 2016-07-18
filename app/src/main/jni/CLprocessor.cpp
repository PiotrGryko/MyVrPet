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




void dumpCLinfo()
{
    LOGD("*** OpenCL info ***");
    try
    {
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        LOGD("OpenCL info: Found %d OpenCL platforms", platforms.size());
        for (int i = 0; i < platforms.size(); ++i)
        {
            std::string name = platforms[i].getInfo<CL_PLATFORM_NAME>();
            std::string version = platforms[i].getInfo<CL_PLATFORM_VERSION>();
            std::string profile = platforms[i].getInfo<CL_PLATFORM_PROFILE>();
            std::string extensions = platforms[i].getInfo<CL_PLATFORM_EXTENSIONS>();
            LOGD( "OpenCL info: Platform[%d] = %s, ver = %s, prof = %s, ext = %s",
                  i, name.c_str(), version.c_str(), profile.c_str(), extensions.c_str() );
        }

        std::vector<cl::Device> devices;
        platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);

        for (int i = 0; i < devices.size(); ++i)
        {
            std::string name = devices[i].getInfo<CL_DEVICE_NAME>();
            std::string extensions = devices[i].getInfo<CL_DEVICE_EXTENSIONS>();
            cl_ulong type = devices[i].getInfo<CL_DEVICE_TYPE>();
            LOGD( "OpenCL info: Device[%d] = %s (%s), ext = %s",
                  i, name.c_str(), (type==CL_DEVICE_TYPE_GPU ? "GPU" : "CPU"), extensions.c_str() );
        }
    }
    catch(cl::Error& e)
    {
        LOGE( "OpenCL info: error while gathering OpenCL info: %s (%d)", e.what(), e.err() );
    }
    catch(std::exception& e)
    {
        LOGE( "OpenCL info: error while gathering OpenCL info: %s", e.what() );
    }
    catch(...)
    {
        LOGE( "OpenCL info: unknown error while gathering OpenCL info" );
    }
    LOGD("*******************");
}


const char clLaplacian[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void Laplacian( \n" \
    "        __read_only image2d_t imgIn, \n" \
    "        __write_only image2d_t imgOut \n" \
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
    "  \n" \
    "    write_imagef(imgOut, pos, sum*10); \n" \

        "} \n";

const char testString[] = \

        "__kernel void test( \n" \
    "        __global int  *in, \n" \
    "        __global int *out, \n" \
    "       __global int *result \n" \
    "    ) { \n" \

                "result[get_global_id(0)] = in[get_global_id(0)] + out[get_global_id(0)];"

                "} \n";




const char clGetEdgesCoords[] = \
  "__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; \n" \
    "\n" \
    "__kernel void GetEdges( \n" \
    "        __read_only image2d_t imgIn, \n" \
            "       __global float *resultX, \n" \
                    "       __global float *resultY \n" \


    "    ) { \n" \
    "  \n" \
    "    const int2 pos = {get_global_id(0), get_global_id(1)}; \n" \

        //"result[get_global_id(0) * get_global_id(1)] = read_imagef(imgIn, sampler, pos); \n"
        "resultX[get_global_id(0)*get_global_id(1)]=get_global_id(0);   \n"\
        "resultY[get_global_id(0)*get_global_id(1)]=get_global_id(1);   \n"\

        "} \n";






cl::Context theContext;
cl::CommandQueue theQueue;
cl::Program theProgI2I, testProg, edgesCoordsProg;
bool haveOpenCL = false;

extern "C" void initCL()
{
    dumpCLinfo();

    EGLDisplay mEglDisplay = eglGetCurrentDisplay();
    if (mEglDisplay == EGL_NO_DISPLAY)
        LOGE("initCL: eglGetCurrentDisplay() returned 'EGL_NO_DISPLAY', error = %x", eglGetError());

    EGLContext mEglContext = eglGetCurrentContext();
    if (mEglContext == EGL_NO_CONTEXT)
        LOGE("initCL: eglGetCurrentContext() returned 'EGL_NO_CONTEXT', error = %x", eglGetError());

    cl_context_properties props[] =
    {   CL_GL_CONTEXT_KHR,   (cl_context_properties) mEglContext,
        CL_EGL_DISPLAY_KHR,  (cl_context_properties) mEglDisplay,
        CL_CONTEXT_PLATFORM, 0,
        0 };

    try
    {
        haveOpenCL = false;
        cl::Platform p = cl::Platform::getDefault();
        std::string ext = p.getInfo<CL_PLATFORM_EXTENSIONS>();
        if(ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by PLATFORM");
        props[5] = (cl_context_properties) p();



        theContext = cl::Context(CL_DEVICE_TYPE_GPU, props);
        std::vector<cl::Device> devs = theContext.getInfo<CL_CONTEXT_DEVICES>();
        LOGD("Context returned %d devices, taking the 1st one", devs.size());
        ext = devs[0].getInfo<CL_DEVICE_EXTENSIONS>();
        if(ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by DEVICE");


        theQueue = cl::CommandQueue(theContext, devs[0]);

        cl::Program::Sources mainSrc(1, std::make_pair(clLaplacian, sizeof(clLaplacian)));
        theProgI2I = cl::Program(theContext, mainSrc);
        theProgI2I.build(devs);


        cl::Program::Sources testSrc(1, std::make_pair(testString, sizeof(testString)));
        testProg = cl::Program(theContext, testSrc);
        testProg.build(devs);


        cl::Program::Sources edgesCoordsSrc(1, std::make_pair(clGetEdgesCoords, sizeof(clGetEdgesCoords)));
        edgesCoordsProg = cl::Program(theContext, edgesCoordsSrc);
        edgesCoordsProg.build(devs);


        haveOpenCL = true;
    }
    catch(cl::Error& e)
    {
        LOGE("cl::Error: %s (%d)", e.what(), e.err());
    }
    catch(std::exception& e)
    {
        LOGE("std::exception: %s", e.what());
    }
    catch(...)
    {
        LOGE( "OpenCL info: unknown error while initializing OpenCL stuff" );
    }
    LOGD("initCL completed");
}

extern "C" void closeCL()
{
}

#define GL_TEXTURE_2D 0x0DE1
void procOCL_I2I(int texIn, int texOut, int w, int h)
{
    if(!haveOpenCL)
    {
        LOGE("OpenCL isn't initialized");
        return;
    }

    LOGE("procOCL_I2I(%d, %d, %d, %d)", texIn, texOut, w, h);
    cl::ImageGL imgIn (theContext, CL_MEM_READ_ONLY,  GL_TEXTURE_2D, 0, texIn);
    cl::ImageGL imgOut(theContext, CL_MEM_READ_WRITE, GL_TEXTURE_2D, 0, texOut);



    std::vector < cl::Memory > images;
    images.push_back(imgIn);
    images.push_back(imgOut);

    int64_t t = getTimeMs();
    theQueue.enqueueAcquireGLObjects(&images);
    theQueue.finish();
    //LOGD("enqueueAcquireGLObjects() costs %d ms", getTimeInterval(t));



    t = getTimeMs();
    cl::Kernel Laplacian(theProgI2I, "Laplacian"); //TODO: may be done once
    Laplacian.setArg(0, imgIn);
    Laplacian.setArg(1, imgOut);
    theQueue.finish();
    //LOGD("Kernel() costs %d ms", getTimeInterval(t));

    t = getTimeMs();
    theQueue.enqueueNDRangeKernel(Laplacian, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();
    //LOGD("enqueueNDRangeKernel() costs %d ms", getTimeInterval(t));

    t = getTimeMs();
    theQueue.enqueueReleaseGLObjects(&images);
    theQueue.finish();

    //LOGD("enqueueReleaseGLObjects() costs %d ms", getTimeInterval(t));



    ///////////////////////////////////////////////



    /**
     *
     * TEST GETTING EDGES COORDS
     *
     */

    /*


    //images.pop_back();
    //images.pop_back();

    //images.push_back(imgOut);

    //theQueue.enqueueAcquireGLObjects(&images);
    theQueue.finish();
    //LOGD("enqueueAcquireGLObjects() costs %d ms", getTimeInterval(t));

    cl::Buffer bufferResultX(theContext,CL_MEM_READ_WRITE,sizeof(float)*w*h);
    cl::Buffer bufferResultY(theContext,CL_MEM_READ_WRITE,sizeof(float)*w*h);




    cl::Kernel getCoords(edgesCoordsProg, "GetEdges"); //TODO: may be done once
    getCoords.setArg(0, imgOut);
    getCoords.setArg(1, bufferResultX);
    getCoords.setArg(2, bufferResultY);

    theQueue.finish();
    //LOGD("Kernel() costs %d ms", getTimeInterval(t));

    t = getTimeMs();
    theQueue.enqueueNDRangeKernel(getCoords, cl::NullRange, cl::NDRange(w, h), cl::NullRange);
    theQueue.finish();
    //LOGD("enqueueNDRangeKernel() costs %d ms", getTimeInterval(t));

    t = getTimeMs();
    theQueue.enqueueReleaseGLObjects(&images);
    theQueue.finish();
    //LOGD("enqueueReleaseGLObjects() costs %d ms", getTimeInterval(t));


    float tmpX[w*h];
    float tmpY[w*h];


    //theQueue.enqueueReadBuffer(bufferResultX,CL_TRUE,0,sizeof(float)*w*h,tmpX);


    //for(int i=0;i<w*h;i++){

    //    LOGD("result  %d %s", i, tmp[i]);

    //}


*/


    ////////////////////////////////////////



    /**
     *
     * TEST KERNEL PROGRAM, SUM OF TWO ARRAYS
     *
     */


    ///////////////////////////////////////////////////////////////////////

    int size =w*10;

    int in[size];
    int out[size];

    int tmp[size];


    cl::Buffer bufferIn(theContext,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,sizeof(int)*size,&in,NULL);
    cl::Buffer bufferOut(theContext,CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,sizeof(int)*size,&out,NULL);
    cl::Buffer bufferResult(theContext,CL_MEM_READ_WRITE,sizeof(int)*size);


    cl::Kernel Test(testProg, "test"); //TODO: may be done once
    Test.setArg(0, bufferIn);
    Test.setArg(1, bufferOut);
    Test.setArg(2, bufferResult);
    theQueue.enqueueNDRangeKernel(Test, cl::NullRange, cl::NDRange(w,h), cl::NullRange);

    theQueue.finish();
    //LOGD("Kernel() costs %d ms", getTimeInterval(t));

    theQueue.enqueueReadBuffer(bufferResult,CL_TRUE,0,sizeof(int)*size,tmp);
    theQueue.finish();



   // for(int i=0;i<size;i++){

    //    LOGD("result  %d %d", i, tmp[i]); //only 0

    //}

    ////////////////////////////////////////////////////////////





}


void drawFrameProcCPU(int w, int h, int texOut)
{
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


enum ProcMode {PROC_MODE_NO_PROC=0, PROC_MODE_CPU=1, PROC_MODE_OCL_DIRECT=2, PROC_MODE_OCL_OCV=3};



extern "C" void processFrame(int tex1, int tex2, int w, int h, int mode)
{
    //LOGD("processing mode %d", mode);


    switch(mode)
    {
        case PROC_MODE_NO_PROC:
    case PROC_MODE_CPU:
        drawFrameProcCPU(w, h, tex2);
        break;
    case PROC_MODE_OCL_DIRECT:
        procOCL_I2I(tex1, tex2, w, h);
        break;

    default:
        ;
        //LOGE("Unexpected processing mode: %d", mode);
    }

//    drawFrameProcCPU(w, h, tex2);


}
