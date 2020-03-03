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
    glewInit();
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

bool ByteDancePlugin::onPluginRecordAudioFrame(AudioPluginFrame* audioFrame)
{
    return true;
}

bool ByteDancePlugin::onPluginPlaybackAudioFrame(AudioPluginFrame* audioFrame)
{
    return true;
}

bool ByteDancePlugin::onPluginMixedAudioFrame(AudioPluginFrame* audioFrame)
{
    return true;
}

bool ByteDancePlugin::onPluginPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioPluginFrame* audioFrame)
{
    return true;
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
            
            //face detect
            ret = bef_effect_ai_face_detect_create(BEF_DETECT_SMALL_MODEL | BEF_DETECT_FULL | BEF_DETECT_MODE_VIDEO, mFaceDetectPath.c_str(), &m_faceDetectHandle);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create face detect handle failed !");
            
            ret = bef_effect_ai_face_check_license(m_faceDetectHandle, mLicensePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: check_license face detect failed");
            
            ret = bef_effect_ai_face_detect_setparam(m_faceDetectHandle, BEF_FACE_PARAM_FACE_DETECT_INTERVAL, 15);
            
            ret = bef_effect_ai_face_detect_setparam(m_faceDetectHandle, BEF_FACE_PARAM_MAX_FACE_NUM, BEF_MAX_FACE_NUM);
            
//            ret = bef_effect_ai_face_detect_add_extra_model(m_faceDetectHandle, TT_MOBILE_FACE_280_DETECT , mFaceDetectExtraPath.c_str());
            
            //face attributes
            ret = bef_effect_ai_face_attribute_create(0, mFaceAttributePath.c_str(), &m_faceAttributesHandle);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create face attribute handle failed !");
            
            ret = bef_effect_ai_face_attribute_check_license(m_faceAttributesHandle, mLicensePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: check_license face attribute failed");
            
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
#ifndef _WIN32
        CGLLockContext(_glContext);
#endif
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        unsigned char *in_ptr = yuvData(videoFrame);
        unsigned char* out_ptr = (unsigned char*)malloc(videoFrame->yStride * videoFrame->height * 4);
        cvt_yuv2rgba(in_ptr, out_ptr, BEF_AI_PIX_FMT_YUV420P, videoFrame->yStride, videoFrame->height, videoFrame->yStride, videoFrame->height, BEF_AI_CLOCKWISE_ROTATE_0, false);
        
        bef_ai_face_info faceInfo;
        memset(&faceInfo, 0, sizeof(bef_ai_face_info));
        ret = bef_effect_ai_face_detect(m_faceDetectHandle, out_ptr, BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride, videoFrame->height, videoFrame->yStride * 4, BEF_AI_CLOCKWISE_ROTATE_0, BEF_DETECT_MODE_VIDEO | BEF_DETECT_FULL, &faceInfo);
        mFaceInfo = faceInfo;
        
        if(faceInfo.face_count == 0) {
            LOG_F(INFO, "no face detected");
        } else {
            unsigned long long attriConfig = BEF_FACE_ATTRIBUTE_AGE | BEF_FACE_ATTRIBUTE_HAPPINESS                                |BEF_FACE_ATTRIBUTE_EXPRESSION|BEF_FACE_ATTRIBUTE_GENDER
            |BEF_FACE_ATTRIBUTE_RACIAL|BEF_FACE_ATTRIBUTE_ATTRACTIVE;
            bef_ai_face_attribute_info attrInfo;
            memset(&attrInfo, 0, sizeof(bef_ai_face_attribute_info));
            ret = bef_effect_ai_face_attribute_detect(m_faceAttributesHandle, out_ptr, BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride, videoFrame->height, videoFrame->yStride * 4, faceInfo.base_infos, attriConfig, &attrInfo);
            mFaceAttributeInfo = attrInfo;
        }
        
        
        
        ret = bef_effect_ai_algorithm_buffer(m_renderMangerHandle, out_ptr,
                                       BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride,
                                       videoFrame->height, videoFrame->yStride * 4,
                                       timestamp);
        ret = bef_effect_ai_process_buffer(m_renderMangerHandle, out_ptr,
                                        BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride,
                                        videoFrame->height, videoFrame->yStride * 4,
                                        out_ptr, BEF_AI_PIX_FMT_RGBA8888,
                                        timestamp);
#ifndef _WIN32        
        CGLUnlockContext(_glContext);
#endif
        
        CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::buffer:: buffer image failed !");
        cvt_rgba2yuv(out_ptr, in_ptr, BEF_AI_PIX_FMT_YUV420P, videoFrame->width, videoFrame->height);
        
        videoFrameData(videoFrame, in_ptr);
        delete in_ptr;
        delete out_ptr;
    } while(false);
    
    return true;
}

int ByteDancePlugin::load(const char *path)
{
    if(mLoaded) {
        return -101;
    }
    
    
    loguru::add_file("pluginfu.log", loguru::Append, loguru::Verbosity_MAX);
    
    std::string sPath(path);
    folderPath = sPath;
    
    
    mLoaded = true;
    mReleased = false;
    return 0;
}

int ByteDancePlugin::unLoad()
{
    if(!mLoaded) {
        return -101;
    }
    
    mLoaded = false;
    return 0;
}


int ByteDancePlugin::enable()
{
    return 0;
}


int ByteDancePlugin::disable()
{
    return 0;
}


int ByteDancePlugin::setParameter(const char *param)
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
    
    if(d.HasMember("plugin.bytedance.faceDetectModelPath")) {
        Value& faceDetectModelPath = d["plugin.bytedance.faceDetectModelPath"];
        if(!faceDetectModelPath.IsString()) {
            return false;
        }
        mFaceDetectPath = std::string(faceDetectModelPath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.faceDetectExtraModelPath")) {
        Value& faceDetectExtraModelPath = d["plugin.bytedance.faceDetectExtraModelPath"];
        if(!faceDetectExtraModelPath.IsString()) {
            return -101;
        }
        mFaceDetectExtraPath = std::string(faceDetectExtraModelPath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.faceAttributeModelPath")) {
        Value& attributeModelPath = d["plugin.bytedance.faceAttributeModelPath"];
        if(!attributeModelPath.IsString()) {
            return -101;
        }
        mFaceAttributePath = std::string(attributeModelPath.GetString());
    }

    if(d.HasMember("plugin.bytedance.beauty.resourcepath")) {
        Value& resourcePath = d["plugin.bytedance.beauty.resourcepath"];
        if(!resourcePath.IsString()) {
            return -101;
        }
        mBeautyPath = resourcePath.GetString();
    }
    
    if(d.HasMember("plugin.bytedance.beauty.intensity")) {
        Value& options = d["plugin.bytedance.beauty.intensity"];
        if(!options.IsObject()) {
            return -101;
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
    
    
    return 0;
}

const char* ByteDancePlugin::getParameter(const char* key)
{
    strBuf.Clear();
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
    writer.SetMaxDecimalPlaces(3);
    if (strncmp(key, "plugin.bytedance.face.info", strlen(key)) == 0) {
    } else if(strncmp(key, "plugin.bytedance.face.attribute", strlen(key)) == 0) {
        writer.StartObject();
        writer.Key("age");
        writer.Double(mFaceAttributeInfo.age);
            
        writer.Key("boy_prob");
        writer.Double(mFaceAttributeInfo.boy_prob);
            
        writer.Key("attractive");
        writer.Double(mFaceAttributeInfo.attractive);
            
        writer.Key("happy_score");
        writer.Double(mFaceAttributeInfo.happy_score);
            
        writer.Key("exp_type");
        writer.Double(mFaceAttributeInfo.exp_type);
            
        writer.Key("racial_type");
        writer.Double(mFaceAttributeInfo.racial_type);
            
        writer.EndObject();
        return strBuf.GetString();
    }
    return "";
}

int ByteDancePlugin::release()
{
    mReleased = true;
    folderPath = "";
    return 0;
}

IAVFramePlugin* createAVFramePlugin()
{
    return new ByteDancePlugin();
}
