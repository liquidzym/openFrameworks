#pragma once
#include "ofConstants.h"
#include "ofVec2f.h"
#include "ofVec3f.h"

#if defined (OF_TARGET_API_VULKAN)
#include <vulkan/vulkan.hpp>
#endif

class ofWindowSettings{
public:
	ofWindowSettings()
	:width(1024)
	,height(768)
	,windowMode(OF_WINDOW)
	,position(0,0)
	,positionSet(false){}

	virtual ~ofWindowSettings(){};

	int width;
	int height;
	std::string title;
	ofWindowMode windowMode;

	void setPosition(const glm::vec2 & position) {
		this->position = position;
		this->positionSet = true;
	}

	const glm::vec2 & getPosition() const {
		return position;
	}

	bool isPositionSet() const {
		return positionSet;
	}

protected:
	glm::vec2 position;
	bool positionSet;
};

class ofGLWindowSettings: public ofWindowSettings{
public:
	ofGLWindowSettings()
		:glVersionMajor(2)
		,glVersionMinor(1){}

	ofGLWindowSettings(const ofWindowSettings & settings)
		:ofWindowSettings(settings)
		,glVersionMajor(2)
		,glVersionMinor(1){
		const ofGLWindowSettings * glSettings = dynamic_cast<const ofGLWindowSettings*>(&settings);
		if(glSettings){
			glVersionMajor = glSettings->glVersionMajor;
			glVersionMinor = glSettings->glVersionMinor;
		}
	}

	virtual ~ofGLWindowSettings(){};

	void setGLVersion(int major, int minor){
		glVersionMajor = major;
		glVersionMinor = minor;
	}

	int glVersionMajor;
	int glVersionMinor;
};

class ofGLESWindowSettings: public ofWindowSettings{
public:
	ofGLESWindowSettings()
	:glesVersion(1){}

	ofGLESWindowSettings(const ofWindowSettings & settings)
	:ofWindowSettings(settings), glesVersion(1) {
        const ofGLESWindowSettings * glesSettings = dynamic_cast<const ofGLESWindowSettings*>(&settings);
        if(glesSettings){
            glesVersion = glesSettings->glesVersion;
        }
    }

	virtual ~ofGLESWindowSettings(){};

	void setGLESVersion(int version){
		glesVersion = version;
	}

	int glesVersion;
};

#if defined (OF_TARGET_API_VULKAN)

class ofVkWindowSettings : public ofWindowSettings
{
public:
	ofVkWindowSettings()
	{};

	ofVkWindowSettings( const ofWindowSettings & settings )
		: ofWindowSettings( settings ){
	};

	void setVkVersion( int major, int minor, int patch ){
		vkVersion = ( major << 22 ) | ( minor << 12 ) | patch;
	}

	int getVkVersionMajor(){
		return ( ( vkVersion >> 22 ) & ( 0x3ff ) ); // 10 bit
	}

	int getVersionMinor(){
		return ( ( vkVersion >> 12 ) & ( 0x3ff ) ); // 10 bit
	}

	int getVersionPatch(){
		return ( ( vkVersion >> 0 ) & ( 0xfff ) );
	}

	virtual ~ofVkWindowSettings(){
	}

	uint32_t vkVersion             = 1 << 22;
	uint32_t numSwapchainImages    = 2;                               // number of images the swapchain should try to create
	uint32_t numVirtualFrames      = 3;                               // number of virtual frames to produce to feed swapchain
	::vk::PresentModeKHR swapchainType = ::vk::PresentModeKHR::eFifo; // api only guarantees FIFO to be present.
	bool useDebugLayers            = true;                            // whether to use debug layers or not (default == true)
};

#endif