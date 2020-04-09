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

ByteDancePlugin::ByteDancePlugin():cacheYuvVideoFramePtr(NULL),cacheRGBAVideoFramePtr(NULL)
{

}

ByteDancePlugin::~ByteDancePlugin()
{
    releaseCacheBuffer(cacheYuvVideoFramePtr);
    releaseCacheBuffer(cacheRGBAVideoFramePtr);
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

void ByteDancePlugin::yuvData(VideoPluginFrame* videoFrame, VideoPluginFrame* dstVideoFrame)
{
    int ysize = videoFrame->yStride * videoFrame->height;
    int usize = videoFrame->uStride * videoFrame->height / 2;
    int vsize = videoFrame->vStride * videoFrame->height / 2;
    memcpy(static_cast<unsigned char*>(dstVideoFrame->buffer), videoFrame->yBuffer, ysize);
    memcpy(static_cast<unsigned char*>(dstVideoFrame->buffer) + ysize, videoFrame->uBuffer, usize);
    memcpy(static_cast<unsigned char*>(dstVideoFrame->buffer) + ysize + usize, videoFrame->vBuffer, vsize);
}

int ByteDancePlugin::yuvSize(VideoPluginFrame* videoFrame)
{
    int ysize = videoFrame->yStride * videoFrame->height;
    int usize = videoFrame->uStride * videoFrame->height / 2;
    int vsize = videoFrame->vStride * videoFrame->height / 2;
    return ysize + usize + vsize;
}

int ByteDancePlugin::rgbaSize(VideoPluginFrame* videoFrame)
{
    return videoFrame->yStride * videoFrame->height * 4;
}

void ByteDancePlugin::initCacheVideoFrame(VideoPluginFrame* dstVideoFrame, VideoPluginFrame* srcVideoFrame, VIDEO_FRAME_TYPE type)
{
    dstVideoFrame->width = srcVideoFrame->width;
    dstVideoFrame->height = srcVideoFrame->height;
    dstVideoFrame->yStride = srcVideoFrame->yStride;
    dstVideoFrame->uStride = srcVideoFrame->uStride;
    dstVideoFrame->vStride = srcVideoFrame->vStride;

    switch (type)
    {
        case VIDEO_FRAME_TYPE::I420:
            dstVideoFrame->buffer = malloc(yuvSize(dstVideoFrame));
            break;

        case VIDEO_FRAME_TYPE::RGBA32:
            dstVideoFrame->buffer = malloc(rgbaSize(dstVideoFrame));
            break;
        default:
            break;
    }
}

void ByteDancePlugin::checkCreateVideoFrame(VideoPluginFrame* videoFrame)
{
    if (!cacheYuvVideoFramePtr && !cacheRGBAVideoFramePtr)
    {
        cacheYuvVideoFramePtr = new VideoPluginFrame();
        initCacheVideoFrame(cacheYuvVideoFramePtr, videoFrame, VIDEO_FRAME_TYPE::I420);

        cacheRGBAVideoFramePtr = new VideoPluginFrame();
        initCacheVideoFrame(cacheRGBAVideoFramePtr, videoFrame, VIDEO_FRAME_TYPE::RGBA32);
        return;
    }

    // if video resolution change, we need to resize videoPtr
    if (cacheYuvVideoFramePtr->width != videoFrame->width
    || cacheYuvVideoFramePtr->height != videoFrame->height
    || cacheYuvVideoFramePtr->yStride != videoFrame->yStride
    || cacheYuvVideoFramePtr->uStride != videoFrame->uStride
    || cacheYuvVideoFramePtr->vStride != videoFrame->vStride)
    {
        releaseCacheBuffer(cacheYuvVideoFramePtr);
        releaseCacheBuffer(cacheRGBAVideoFramePtr);

        cacheYuvVideoFramePtr = new VideoPluginFrame();
        initCacheVideoFrame(cacheYuvVideoFramePtr, videoFrame, VIDEO_FRAME_TYPE::I420);

        cacheRGBAVideoFramePtr = new VideoPluginFrame();
        initCacheVideoFrame(cacheRGBAVideoFramePtr, videoFrame, VIDEO_FRAME_TYPE::RGBA32);
    }
}

void ByteDancePlugin::memsetCacheBuffer(VideoPluginFrame* videoFrame)
{
    memset(videoFrame->buffer, 0, sizeof(videoFrame->buffer));
}

void ByteDancePlugin::releaseCacheBuffer(VideoPluginFrame* videoFrame)
{
    if (videoFrame)
    {
        if (videoFrame->buffer)
        {
            delete videoFrame->buffer;
            videoFrame->buffer = NULL;
        }
        delete videoFrame;
        videoFrame = NULL;
    }
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
            mFaceAttributeLoaded = false;
            mHandDetectLoaded = false;
        }
        previousThreadId = tid;

        
        // 2. initialize if not yet done
        if (!mNamaInited) {
            //load bytedance and initialize
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
        
        if (mNamaInited && mFaceAttributeEnabled && !mFaceAttributeLoaded) {
            //face detect
            ret = bef_effect_ai_face_detect_create(BEF_DETECT_SMALL_MODEL | BEF_DETECT_FULL | BEF_DETECT_MODE_VIDEO, mFaceDetectPath.c_str(), &m_faceDetectHandle);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create face detect handle failed !");
            
            ret = bef_effect_ai_face_check_license(m_faceDetectHandle, mLicensePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: check_license face detect failed");
            
            ret = bef_effect_ai_face_detect_setparam(m_faceDetectHandle, BEF_FACE_PARAM_FACE_DETECT_INTERVAL, 15);
            
            ret = bef_effect_ai_face_detect_setparam(m_faceDetectHandle, BEF_FACE_PARAM_MAX_FACE_NUM, BEF_MAX_FACE_NUM);
            
            //face attributes
            ret = bef_effect_ai_face_attribute_create(0, mFaceAttributePath.c_str(), &m_faceAttributesHandle);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create face attribute handle failed !");
            
            ret = bef_effect_ai_face_attribute_check_license(m_faceAttributesHandle, mLicensePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: check_license face attribute failed");
            mFaceAttributeLoaded = true;
        }
        
        if (mNamaInited && mHandDetectEnabled && !mHandDetectLoaded) {
            //hand detect
            ret = bef_effect_ai_hand_detect_create(&m_handDetectHandle, 0);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: create hand detect handle failed !");
            
            ret = bef_effect_ai_hand_check_license(m_handDetectHandle, mLicensePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: check_license hand detect failed");
            
            ret = bef_effect_ai_hand_detect_setmodel(m_handDetectHandle, BEF_HAND_MODEL_DETECT, mHandDetectPath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set hand detect model failed !");
            
            ret = bef_effect_ai_hand_detect_setmodel(m_handDetectHandle, BEF_HAND_MODEL_BOX_REG, mHandBoxPath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set hand box model failed !");
            
            ret = bef_effect_ai_hand_detect_setmodel(m_handDetectHandle, BEF_HAND_MODEL_GESTURE_CLS, mHandGesturePath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set hand gesture model failed !");
            
            ret = bef_effect_ai_hand_detect_setmodel(m_handDetectHandle, BEF_HAND_MODEL_KEY_POINT, mHandKPPath.c_str());
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set hand key points model failed !");
            
            ret = bef_effect_ai_hand_detect_setparam(m_handDetectHandle, BEF_HAND_MAX_HAND_NUM, 1);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set max hand num failed !");
            ret = bef_effect_ai_hand_detect_setparam(m_handDetectHandle, BEF_HNAD_ENLARGE_FACTOR_REG, 2.0);
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::initializeHandle:: set hand enlarge factor failed !");
            mHandDetectLoaded = true;
        }
        
        if(mNamaInited) {
            bef_effect_ai_set_width_height(m_renderMangerHandle, videoFrame->width, videoFrame->height);
                    // 4. make it beautiful
    #ifndef _WIN32
            CGLLockContext(_glContext);
    #endif
            checkCreateVideoFrame(videoFrame);
            yuvData(videoFrame, cacheYuvVideoFramePtr);
            uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            cvt_yuv2rgba((unsigned char*)cacheYuvVideoFramePtr->buffer,(unsigned char*)cacheRGBAVideoFramePtr->buffer, BEF_AI_PIX_FMT_YUV420P, videoFrame->yStride, videoFrame->height, videoFrame->yStride, videoFrame->height, BEF_AI_CLOCKWISE_ROTATE_0, false);
            
            if(mFaceAttributeEnabled){
                if(!mFaceAttributeLoaded){
                    LOG_F(ERROR, "face attribute enabled but not initialized");
                } else {
                    bef_ai_face_info faceInfo;
                    memset(&faceInfo, 0, sizeof(bef_ai_face_info));
                    ret = bef_effect_ai_face_detect(m_faceDetectHandle, (unsigned char*)cacheRGBAVideoFramePtr->buffer, BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride, videoFrame->height, videoFrame->yStride * 4, BEF_AI_CLOCKWISE_ROTATE_0, BEF_DETECT_MODE_VIDEO | BEF_DETECT_FULL, &faceInfo);
                    mFaceInfo = faceInfo;
                    CHECK_BEF_AI_RET_SUCCESS(ret, "face info detect failed");
                    if(faceInfo.face_count != 0) {
                        unsigned long long attriConfig = BEF_FACE_ATTRIBUTE_AGE | BEF_FACE_ATTRIBUTE_HAPPINESS                                |BEF_FACE_ATTRIBUTE_EXPRESSION|BEF_FACE_ATTRIBUTE_GENDER
                        |BEF_FACE_ATTRIBUTE_RACIAL|BEF_FACE_ATTRIBUTE_ATTRACTIVE;
                        bef_ai_face_attribute_info attrInfo;
                        memset(&attrInfo, 0, sizeof(bef_ai_face_attribute_info));
                        ret = bef_effect_ai_face_attribute_detect(m_faceAttributesHandle, (unsigned char*)cacheRGBAVideoFramePtr->buffer, BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride, videoFrame->height, videoFrame->yStride * 4, faceInfo.base_infos, attriConfig, &attrInfo);
                        mFaceAttributeInfo = attrInfo;
                        CHECK_BEF_AI_RET_SUCCESS(ret, "face attribute detect failed");
                    }
                }
            }
            
            if(mHandDetectEnabled){
                if(!mHandDetectLoaded){
                    LOG_F(ERROR, "hand detect enabled but not initialized");
                } else {
                    bef_ai_hand_info handInfo;
                    ret = bef_effect_ai_hand_detect(m_handDetectHandle, (unsigned char*)cacheRGBAVideoFramePtr->buffer, BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride, videoFrame->height, videoFrame->yStride * 4, BEF_AI_CLOCKWISE_ROTATE_0,
                                                       BEF_HAND_MODEL_DETECT | BEF_HAND_MODEL_BOX_REG |
                                                       BEF_HAND_MODEL_GESTURE_CLS| BEF_HAND_MODEL_KEY_POINT, &handInfo, 0);
                    mHandInfo = handInfo;
                    CHECK_BEF_AI_RET_SUCCESS(ret, "gesture info collect failed");
                }
            }
            
            if(mProcessEnabled) {
                ret = bef_effect_ai_algorithm_buffer(m_renderMangerHandle, (unsigned char*)cacheRGBAVideoFramePtr->buffer,
                                               BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride,
                                               videoFrame->height, videoFrame->yStride * 4,
                                               timestamp);
                ret = bef_effect_ai_process_buffer(m_renderMangerHandle, (unsigned char*)cacheRGBAVideoFramePtr->buffer,
                                                BEF_AI_PIX_FMT_RGBA8888, videoFrame->yStride,
                                                videoFrame->height, videoFrame->yStride * 4,
                                                (unsigned char*)cacheRGBAVideoFramePtr->buffer, BEF_AI_PIX_FMT_RGBA8888,
                                                timestamp);
            }
    #ifndef _WIN32
            CGLUnlockContext(_glContext);
    #endif
            
            CHECK_BEF_AI_RET_SUCCESS(ret, "EffectHandle::buffer:: buffer image failed !");
            cvt_rgba2yuv((unsigned char*)cacheRGBAVideoFramePtr->buffer, (unsigned char*)cacheYuvVideoFramePtr->buffer, BEF_AI_PIX_FMT_YUV420P, videoFrame->width, videoFrame->height);
            
            videoFrameData(videoFrame, (unsigned char*)cacheYuvVideoFramePtr->buffer);
            memsetCacheBuffer(cacheYuvVideoFramePtr);
            memsetCacheBuffer(cacheRGBAVideoFramePtr);
        }
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
        return -100;
    }
    
    
    if(d.HasMember("plugin.bytedance.licensePath")) {
        Value& licencePath = d["plugin.bytedance.licensePath"];
        if(!licencePath.IsString()) {
            return -101;
        }
        mLicensePath = std::string(licencePath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.stickerPath")) {
        Value& stickerPath = d["plugin.bytedance.stickerPath"];
        if(!stickerPath.IsString()) {
            return -101;
        }
        mStickerPath = std::string(stickerPath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.faceDetectModelPath")) {
        Value& faceDetectModelPath = d["plugin.bytedance.faceDetectModelPath"];
        if(!faceDetectModelPath.IsString()) {
            return -101;
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
    
    if(d.HasMember("plugin.bytedance.faceAttributeEnabled")) {
        Value& enabled = d["plugin.bytedance.faceAttributeEnabled"];
        if(!enabled.IsBool()) {
            return -101;
        }
        mFaceAttributeEnabled = enabled.GetBool();
    }
    
    if(d.HasMember("plugin.bytedance.faceAttributeModelPath")) {
        Value& attributeModelPath = d["plugin.bytedance.faceAttributeModelPath"];
        if(!attributeModelPath.IsString()) {
            return -101;
        }
        mFaceAttributePath = std::string(attributeModelPath.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.handDetectEnabled")) {
        Value& enabled = d["plugin.bytedance.handDetectEnabled"];
        if(!enabled.IsBool()) {
            return -101;
        }
        mHandDetectEnabled = enabled.GetBool();
    }
    
    if(d.HasMember("plugin.bytedance.handDetectModelPath")) {
        Value& path = d["plugin.bytedance.handDetectModelPath"];
        if(!path.IsString()) {
            return -101;
        }
        mHandDetectPath = std::string(path.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.handBoxModelPath")) {
        Value& path = d["plugin.bytedance.handBoxModelPath"];
        if(!path.IsString()) {
            return -101;
        }
        mHandBoxPath = std::string(path.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.handGestureModelPath")) {
        Value& path = d["plugin.bytedance.handGestureModelPath"];
        if(!path.IsString()) {
            return -101;
        }
        mHandGesturePath = std::string(path.GetString());
    }
    
    if(d.HasMember("plugin.bytedance.handKPModelPath")) {
        Value& path = d["plugin.bytedance.handKPModelPath"];
        if(!path.IsString()) {
            return -101;
        }
        mHandKPPath = std::string(path.GetString());
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
    } else if(strncmp(key, "plugin.bytedance.hand.info", strlen(key)) == 0) {
        writer.StartArray();
        for(int i = 0; i < mHandInfo.hand_count; i++) {
            bef_ai_hand hand = mHandInfo.p_hands[i];
            writer.StartObject();
            writer.Key("action");
            writer.Int(hand.action);
                
            writer.Key("seq_action");
            writer.Double(hand.seq_action);
            writer.EndObject();
        }
        
        writer.EndArray();
        return strBuf.GetString();
    }
    return "";
}

int ByteDancePlugin::release()
{
    mReleased = true;
    folderPath = "";
    delete this;
    return 0;
}

IAVFramePlugin* createAVFramePlugin()
{
    return new ByteDancePlugin();
}
