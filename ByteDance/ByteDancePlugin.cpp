//
//  ByteDancePlugin.cpp
//  ByteDancePlugin
//
//  Created by 张乾泽 on 2019/8/6.
//  Copyright © 2019 Agora Corp. All rights reserved.
//
#include "ByteDancePlugin.h"
#include <string.h>
#include <string>
#include "Utils.h"
#include <stdlib.h>
#include <iostream>
#include "bef_effect_ai_yuv_process.h"
#include <chrono>


#define MAX_PATH 512
#define MAX_PROPERTY_NAME 256
#define MAX_PROPERTY_VALUE 512

static bool mNamaInited = false;
using namespace std::chrono;

#if defined(_WIN32)
PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1u,
    PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW,
    PFD_TYPE_RGBA,
    32u,
    0u, 0u, 0u, 0u, 0u, 0u,
    8u,
    0u,
    0u,
    0u, 0u, 0u, 0u,
    24u,
    8u,
    0u,
    PFD_MAIN_PLANE,
    0u,
    0u, 0u };
#endif

ByteDancePlugin::ByteDancePlugin()
{
}

ByteDancePlugin::~ByteDancePlugin()
{
    
}

bool ByteDancePlugin::initOpenGL()
{
#ifdef _WIN32
	HWND hw = CreateWindowExA(
		0, "EDIT", "", ES_READONLY,
		0, 0, 1, 1,
		NULL, NULL,
		GetModuleHandleA(NULL), NULL);
	HDC hgldc = GetDC(hw);
	int spf = ChoosePixelFormat(hgldc, &pfd);
    if(spf == 0) {
        return false;
    }
	int ret = SetPixelFormat(hgldc, spf, &pfd);
    if(!ret) {
        return false;
    }
	HGLRC hglrc = wglCreateContext(hgldc);
	wglMakeCurrent(hgldc, hglrc);

	//hglrc就是创建出的OpenGL context
	printf("hw=%08x hgldc=%08x spf=%d ret=%d hglrc=%08x\n",
		hw, hgldc, spf, ret, hglrc);
#else
	CGLPixelFormatAttribute attrib[13] = { kCGLPFAOpenGLProfile,
		(CGLPixelFormatAttribute)kCGLOGLPVersion_Legacy,
		kCGLPFAAccelerated,
		kCGLPFAColorSize, (CGLPixelFormatAttribute)24,
		kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
		kCGLPFADoubleBuffer,
		kCGLPFASampleBuffers, (CGLPixelFormatAttribute)1,
		kCGLPFASamples, (CGLPixelFormatAttribute)4,
		(CGLPixelFormatAttribute)0 };
	CGLPixelFormatObj pixelFormat = NULL;
	GLint numPixelFormats = 0;
	CGLContextObj cglContext1 = NULL;
	CGLChoosePixelFormat(attrib, &pixelFormat, &numPixelFormats);
	CGLError err = CGLCreateContext(pixelFormat, NULL, &cglContext1);
	CGLSetCurrentContext(cglContext1);
    
    GLint sync = 1;
    CGLSetParameter(cglContext1, kCGLCPSwapInterval, &sync);
	if (err) {
		return false;
	}
    _glContext = cglContext1;
#endif
	return true;
}

unsigned char *ByteDancePlugin::yuvData(VideoPluginFrame* videoFrame)
{
    int ysize = videoFrame->yStride * videoFrame->height;
    int usize = videoFrame->uStride * videoFrame->height / 2;
    int vsize = videoFrame->vStride * videoFrame->height / 2;
    unsigned char *temp = (unsigned char *)malloc(ysize + usize + vsize);
    
    memcpy(temp, videoFrame->yBuffer, ysize);
    memcpy(temp + ysize, videoFrame->uBuffer, usize);
    memcpy(temp + ysize + usize, videoFrame->vBuffer, vsize);
    return (unsigned char *)temp;
}

int ByteDancePlugin::yuvSize(VideoPluginFrame* videoFrame)
{
    int ysize = videoFrame->yStride * videoFrame->height;
    int usize = videoFrame->uStride * videoFrame->height / 2;
    int vsize = videoFrame->vStride * videoFrame->height / 2;
    return ysize + usize + vsize;
}

void ByteDancePlugin::videoFrameData(VideoPluginFrame* videoFrame, unsigned char *yuvData)
{
    int ysize = videoFrame->yStride * videoFrame->height;
    int usize = videoFrame->uStride * videoFrame->height / 2;
    int vsize = videoFrame->vStride * videoFrame->height / 2;
    
    memcpy(videoFrame->yBuffer, yuvData,  ysize);
    memcpy(videoFrame->uBuffer, yuvData + ysize, usize);
    memcpy(videoFrame->vBuffer, yuvData + ysize + usize, vsize);
}

bool ByteDancePlugin::onPluginRenderVideoFrame(unsigned int uid, VideoPluginFrame *videoFrame)
{
    return true;
}

bool ByteDancePlugin::onPluginCaptureVideoFrame(VideoPluginFrame *videoFrame)
{
    if(mReleased) {
        if(mNamaInited) {
//            fuDestroyAllItems();
            mNamaInited = false;
            //no need to update bundle option as bundles will be reloaded anyway
            mNeedUpdateBundles = false;
            //need to reload bundles once resume from stopping
            mNeedLoadBundles = true;
        }

        LOG_F(INFO, "FU Plugin Marked as released, skip");
        return true;
    }

    do {
        bef_effect_result_t ret = 0;
#if defined(_WIN32)
        int tid = GetCurrentThreadId();
#else
        uint64_t tid;
        pthread_threadid_np(NULL, &tid);
#endif
        if(tid != previousThreadId && mNamaInited) {
//            fuOnDeviceLost();
            mNamaInited = false;
            //no need to update bundle option as bundles will be reloaded anyway
            mNeedUpdateBundles = false;
            //need to reload bundles once resume from stopping
            mNeedLoadBundles = true;
        }
        previousThreadId = tid;

        
        // 2. initialize if not yet done
        if (!mNamaInited) {
            //load nama and initialize
            if (false == initOpenGL()) {
                break;
            }
            ret = bef_effect_ai_create(&m_renderMangerHandle);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create effect handle failed !");
            
            ret = bef_effect_ai_check_license(m_renderMangerHandle, mLicensePath.c_str());
            std::string  msg ="EffectHandle::initializeHandle:: check_license failed, path: ";
            msg = msg + mLicensePath;
            CHECK_BEF_AI_RET_SUCCESS(ret, msg.c_str());
            
            ret = bef_effect_ai_init(m_renderMangerHandle, 0, 0, mStickerPath.c_str(), "");
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: init effect handle failed !");
            
            mNamaInited = true;
        }
        
        bef_effect_ai_set_width_height(m_renderMangerHandle, videoFrame->width, videoFrame->height);
        
        if(mNeedUpdateBundles) {
            ret = bef_effect_ai_set_beauty(m_renderMangerHandle, mBeautyPath.c_str());
            LOG_F(INFO, "set beauty: %s, %d", mBeautyPath.c_str(), ret);
            for (auto const& x : mBeautyOptions)
            {
                ret = bef_effect_ai_set_intensity(m_renderMangerHandle, x.first, x.second);
                LOG_F(INFO, "set beauty intensity:%d,%f, %d", x.first, x.second, ret);
                
            }
            mNeedUpdateBundles = false;
        }

        // 4. make it beautiful
        CGLLockContext(_glContext);
        
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        unsigned char *in_ptr = yuvData(videoFrame);
        unsigned char* out_ptr = (uint8_t*)malloc(640 * 480 * sizeof(uint8_t) * 4);
        unsigned char* out_ptr2 = (uint8_t*)malloc(640 * 480 * sizeof(uint8_t) * 4);
        cvt_yuv2rgba(in_ptr, out_ptr, BEF_AI_PIX_FMT_YUV420P, videoFrame->yStride, videoFrame->height, videoFrame->yStride, videoFrame->height, BEF_AI_CLOCKWISE_ROTATE_0, false);
        
        
        ret = bef_effect_ai_algorithm_buffer(m_renderMangerHandle, out_ptr,
                                       BEF_AI_PIX_FMT_RGBA8888, videoFrame->width,
                                       videoFrame->height, videoFrame->yStride,
                                       timestamp);
        ret = bef_effect_ai_process_buffer(m_renderMangerHandle, out_ptr,
                                        BEF_AI_PIX_FMT_RGBA8888, videoFrame->width,
                                        videoFrame->height, videoFrame->yStride,
                                        out_ptr2, BEF_AI_PIX_FMT_RGBA8888,
                                        timestamp);
        
        CGLUnlockContext(_glContext);
        
        CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::buffer:: buffer image failed !");
        cvt_rgba2yuv(out_ptr2, in_ptr, BEF_AI_PIX_FMT_YUV420P, videoFrame->width, videoFrame->height);
        
        videoFrameData(videoFrame, in_ptr);
        delete in_ptr;
    } while(false);
    
    return true;
}

bool ByteDancePlugin::load(const char *path)
{
    if(mLoaded) {
        return false;
    }
    
    
    loguru::add_file("pluginfu.log", loguru::Append, loguru::Verbosity_MAX);
    
    std::string sPath(path);
    folderPath = sPath;
    
    
    mLoaded = true;
    mReleased = false;
    return true;
}

bool ByteDancePlugin::unLoad()
{
    if(!mLoaded) {
        return false;
    }
    
    mLoaded = false;
    return true;
}


bool ByteDancePlugin::enable()
{
    do {
        
    } while (false);
    return true;
}


bool ByteDancePlugin::disable()
{
    return true;
}


bool ByteDancePlugin::setParameter(const char *param)
{
    Document d;
    d.Parse(param);
    
    if(d.HasParseError()) {
        return false;
    }
    
    
    if(d.HasMember("plugin.bytedance.licensePath")) {
        Value& licencePath = d["plugin.bytedance.licensePath"];
        if(!licencePath.IsString()) {
            return false;
        }
        mLicensePath = std::string(licencePath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.stickerPath")) {
        Value& stickerPath = d["plugin.bytedance.stickerPath"];
        if(!stickerPath.IsString()) {
            return false;
        }
        mStickerPath = std::string(stickerPath.GetString());
    }

    if(d.HasMember("plugin.bytedance.beauty.resourcepath")) {
        Value& resourcePath = d["plugin.bytedance.beauty.resourcepath"];
        if(!resourcePath.IsString()) {
            return false;
        }
        mBeautyPath = resourcePath.GetString();
    }
    
    if(d.HasMember("plugin.bytedance.beauty.intensity")) {
        Value& options = d["plugin.bytedance.beauty.intensity"];
        if(!options.IsObject()) {
            return false;
        }
        mBeautyOptions.clear();
        for (Value::ConstMemberIterator itr = options.MemberBegin();
             itr != options.MemberEnd(); ++itr)
        {
            int intensityKey = std::stoi(itr->name.GetString());
            const Value& intensityValue = itr->value;
            
            mBeautyOptions.insert(std::map<int, double>::value_type(intensityKey, intensityValue.GetDouble()));
        }
        mNeedUpdateBundles = true;
    }
    
    
    return true;
}

void ByteDancePlugin::release()
{
    mReleased = true;
    folderPath = "";
}

IVideoFramePlugin* createVideoFramePlugin()
{
    return new ByteDancePlugin();
}
