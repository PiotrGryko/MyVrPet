#define __CL_ENABLE_EXCEPTIONS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS /*let's give a chance for OpenCL 1.1 devices*/

//
// Created by piotr on 16/08/16.
//

#include <exception>
#include <CL/cl.hpp>
#include <EGL/egl.h>
#include "CLcommon.hpp"
#include "CLimpl.hpp"





cl::Context context;
cl::CommandQueue queue;
bool initialized = false;
std::vector<cl::Device> devs;


bool openClInitialized()
{
    return initialized;
}


void dumpInfo() {
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

cl::CommandQueue initQueue()
{
    queue = cl::CommandQueue(context, devs[0]);
    return queue;
}

cl::Program initProgram(char src[])
{
    cl::Program::Sources sources(1, std::make_pair(src,
                                                   sizeof(src)));
    cl::Program program = cl::Program(context, sources);
    program.build(devs);
}

extern "C" cl::Context initOpenCL() {
    dumpInfo();

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
        initialized = false;
        cl::Platform p = cl::Platform::getDefault();
        std::string ext = p.getInfo<CL_PLATFORM_EXTENSIONS>();
        if (ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by PLATFORM");
        props[5] = (cl_context_properties) p();


        context = cl::Context(CL_DEVICE_TYPE_GPU, props);
        devs = context.getInfo<CL_CONTEXT_DEVICES>();
        LOGD("Context returned %d devices, taking the 1st one", devs.size());
        ext = devs[0].getInfo<CL_DEVICE_EXTENSIONS>();
        if (ext.find("cl_khr_gl_sharing") == std::string::npos)
            LOGE("Warning: opencl.CL-GL sharing isn't supported by DEVICE");




        initialized = true;
        return context;
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
