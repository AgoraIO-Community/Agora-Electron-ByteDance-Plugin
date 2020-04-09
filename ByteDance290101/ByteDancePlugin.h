//
//  ByteDancePlugin.h
//  ByteDancePlugin
//
//  Created by 张乾泽 on 2019/8/6.
//  Copyright © 2019 Agora Corp. All rights reserved.
//

#ifndef ByteDancePlugin_h
#define ByteDancePlugin_h

#include "IAVFramePlugin.h"
#include "bef_effect_ai_api.h"
#include "bef_effect_ai_face_detect.h"
#include "bef_effect_ai_face_attribute.h"
#include "bef_effect_ai_hand.h"
#include <string>
#include <thread>
#include <vector>
#include <map>
#include "common/rapidjson/document.h"
#include "common/rapidjson/writer.h"
#include "common/rapidjson/stringbuffer.h"
#include "common/loguru.hpp"


#ifdef _WIN32
#include "windows.h"
#include "glew.h"
#pragma comment(lib, "effect.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glew32.lib")
#else
#include <dlfcn.h>
#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <unistd.h>
#endif // WIN32

using namespace rapidjson;

enum ByteDancePluginStatus
{
    ByteDance_PLUGIN_STATUS_STOPPED = 0,
    ByteDance_PLUGIN_STATUS_STOPPING = 1,
    ByteDance_PLUGIN_STATUS_STARTED = 2
};

enum VIDEO_FRAME_TYPE
{
    I420 = 0,
    RGBA32 = 1
};

struct ByteDanceBundle {
    std::string bundleName;
    std::string options;
    bool updated;
};

class ByteDancePlugin : public IAVFramePlugin
{
public:
    ByteDancePlugin();
    ~ByteDancePlugin();
    virtual bool onPluginCaptureVideoFrame(VideoPluginFrame* videoFrame) override;
    virtual bool onPluginRenderVideoFrame(unsigned int uid, VideoPluginFrame* videoFrame) override;
    virtual bool onPluginRecordAudioFrame(AudioPluginFrame* audioFrame) override;
    virtual bool onPluginPlaybackAudioFrame(AudioPluginFrame* audioFrame) override;
    virtual bool onPluginMixedAudioFrame(AudioPluginFrame* audioFrame) override;
    virtual bool onPluginPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioPluginFrame* audioFrame) override;
    
    virtual int load(const char* path) override;
    virtual int unLoad() override;
    virtual int enable() override;
    virtual int disable() override;
    virtual int setParameter(const char* param) override;
    virtual const char* getParameter(const char* key) override;
    virtual int release() override;
protected:
    bool initOpenGL();
    void videoFrameData(VideoPluginFrame* videoFrame, unsigned char *yuvData);
    void yuvData(VideoPluginFrame* srcVideoFrame, VideoPluginFrame* dstVideoFrame);
    int yuvSize(VideoPluginFrame* videoFrame);
    int rgbaSize(VideoPluginFrame* videoFrame);
    void checkCreateVideoFrame(VideoPluginFrame* videoFrame);
    void initCacheVideoFrame(VideoPluginFrame* dstVideoFrame, VideoPluginFrame* srcVideoFrame, VIDEO_FRAME_TYPE type);
    void memsetCacheBuffer(VideoPluginFrame* videoFrame);
    void releaseCacheBuffer(VideoPluginFrame* videoFrame);
    std::string folderPath;
    rapidjson::StringBuffer strBuf;
    bool switching = false;
#if defined(_WIN32)
    int previousThreadId;
#else
    uint64_t previousThreadId;
#endif
    bool mLoaded = false;
    bool mNeedLoadBundles = true;
    bool mNeedUpdateBundles = true;
    bool mReleased = false;
    bool mFaceAttributeLoaded = false;
    bool mFaceAttributeEnabled = false;
    bool mHandDetectLoaded = false;
    bool mHandDetectEnabled = false;
    bool mProcessEnabled = false;
    std::string mLicensePath = "";
    std::string mStickerPath = "";
    std::string mBeautyPath  ="";
    std::string mHandDetectPath = "";
    std::string mHandBoxPath = "";
    std::string mHandGesturePath = "";
    std::string mHandKPPath = "";
    std::string mFaceDetectPath = "";
    std::string mFaceDetectExtraPath = "";
    std::string mFaceAttributePath = "";
    bef_ai_face_info mFaceInfo;
    bef_ai_face_attribute_info mFaceAttributeInfo;
    bef_ai_hand_info mHandInfo;
    bef_effect_handle_t m_renderMangerHandle = 0;
    bef_effect_handle_t m_handDetectHandle = 0;
    bef_effect_handle_t m_faceDetectHandle = 0;
    bef_effect_handle_t m_faceAttributesHandle = 0;
    std::vector<ByteDanceBundle> bundles;
    std::map<int, double> mBeautyOptions;
    std::unique_ptr<int> items;
#ifndef _WIN32
    CGLContextObj _glContext;
#endif
    VideoPluginFrame* cacheYuvVideoFramePtr;
    VideoPluginFrame* cacheRGBAVideoFramePtr;
};

#define READ_DOUBLE_VALUE_PARAM(d, name, newvalue) \
if(d.HasMember(name)) { \
    Value& value = d[name]; \
    if(!value.IsNumber()) { \
        return false; \
    } \
    newvalue = value.GetDouble(); \
}

#define CHECK_BEF_AI_RET_SUCCESS(ret, mes) \
if(ret != 0){\
    LOG_F(ERROR, "ByteDancePlugin: error id = %d, %s", ret, mes);\
}

#if defined(_WIN32)
/*
 * '_cups_strlcpy()' - Safely copy two strings.
 */

size_t					/* O - Length of string */
strlcpy(char *dst,		/* O - Destination string */
	const char *src,		/* I - Source string */
	size_t size)		/* I - Size of destination string buffer */
{
	size_t	srclen;			/* Length of source string */


	/*
	* Figure out how much room is needed...
	*/
	size--;

	srclen = strlen(src);

	/*
	* Copy the appropriate amount...
	*/

	if (srclen > size)
		srclen = size;

	memcpy(dst, src, srclen);
	dst[srclen] = '\0';

	return (srclen);
}

#endif

#endif /* ByteDancePlugin_h */
