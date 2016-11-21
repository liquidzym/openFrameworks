#include "ofVkRenderer.h"
#include "ofMesh.h"
#include "ofPath.h"
#include "ofBitmapFont.h"
#include "ofImage.h"
#include "of3dPrimitives.h"
#include "ofLight.h"
#include "ofMaterial.h"
#include "ofCamera.h"
#include "ofTrueTypeFont.h"
#include "ofNode.h"
#include "GLFW/glfw3.h"

#include <cstdint>

const string ofVkRenderer::TYPE = "Vulkan";

using InstanceP = shared_ptr<VkInstance>;

// ----------------------------------------------------------------------

// fetched function pointers for debug layer callback creation / destruction
// functions. As these are not directly exposed within the sdk, we have to 
// first query the sdk for the function pointers to these.
PFN_vkCreateDebugReportCallbackEXT  fVkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT fVkDestroyDebugReportCallbackEXT = nullptr;

// ----------------------------------------------------------------------

ofVkRenderer::ofVkRenderer(const ofAppBaseWindow * _window, Settings settings )
	: m3dGraphics(this)
	, mSettings(settings)
{
	bBackgroundAuto = true;
	wrongUseLoggedOnce = false;

	currentMaterial = nullptr;

	window = _window;

	mPath.setMode(ofPath::POLYLINES);
	mPath.setUseShapeColor(false);

	if ( mSettings.useDebugLayers ){
		requestDebugLayers();
	}

#ifdef TARGET_LINUX
	mInstanceExtensions.push_back( "VK_KHR_xcb_surface" );
#endif
#ifdef TARGET_WIN32
	mInstanceExtensions.push_back( "VK_KHR_win32_surface" );
#endif
	mInstanceExtensions.push_back( "VK_KHR_surface" );
	mDeviceExtensions.push_back( "VK_KHR_swapchain" );

	createInstance();

	// important to call createDebugLayers() after createInstance, 
	// since otherwise the debug layer create function pointers will not be 
	// correctly resolved, since the main library would not yet have been loaded.
	if ( mSettings.useDebugLayers ){
		createDebugLayers();
	}
	// createDevice also initialises the device queue, mQueue
	createDevice();

	// up next: create window surface (this happens within glfw)
}

// ----------------------------------------------------------------------


const vk::Instance& ofVkRenderer::getInstance() {
	return mInstance;
}

// ----------------------------------------------------------------------

vk::SurfaceKHR& ofVkRenderer::getWindowSurface() {
	return mWindowSurface;
}


// ----------------------------------------------------------------------

ofVkRenderer::~ofVkRenderer()
{
	// Tell GPU to finish whatever it is doing 
	// and to catch up with the CPU waiting right here.
	//
	// This is a sync method so harsh, it should
	// only ever be used for teardown. 
	//
	// Which is what this method is doing.
	mDevice.waitIdle();

	mDefaultContext.reset();

	for ( auto & depthStencilResource : mDepthStencil ){
		mDevice.destroyImageView( depthStencilResource.view );
		mDevice.destroyImage( depthStencilResource.image );
		mDevice.freeMemory( depthStencilResource.mem );
	}
	mDepthStencil.clear();

	mSwapchain.reset();
	mPipelineCache.reset();

	// reset command pool and all associated command buffers.
	mDevice.resetCommandPool( mSetupCommandPool, ::vk::CommandPoolResetFlagBits::eReleaseResources );
	mDevice.destroyCommandPool( mSetupCommandPool );

	destroyDevice();
	destroySurface();
	destroyDebugLayers();
	destroyInstance();
}

// ----------------------------------------------------------------------

void ofVkRenderer::destroySurface(){
	vkDestroySurfaceKHR( mInstance, mWindowSurface, nullptr );
	mWindowSurface = VK_NULL_HANDLE;
}

// ----------------------------------------------------------------------

 void ofVkRenderer::createInstance()
{
	ofLog() << "Creating instance.";
	
	std::string appName = "openFrameworks" + ofGetVersionInfo();

	vk::ApplicationInfo applicationInfo;
	applicationInfo
		.setApiVersion( mSettings.vkVersion )
		.setApplicationVersion( VK_MAKE_VERSION( 0, 1, 0 ) )
		.setPApplicationName( appName.c_str() )
		.setPEngineName( "openFrameworks Vulkan Renderer" )
		.setEngineVersion( VK_MAKE_VERSION( 0, 0, 0 ) )
		;

	vk::InstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo
		.setPApplicationInfo        ( &applicationInfo )
		.setEnabledLayerCount       ( mInstanceLayers.size() )
		.setPpEnabledLayerNames     ( mInstanceLayers.data() )
		.setEnabledExtensionCount   ( mInstanceExtensions.size() )
		.setPpEnabledExtensionNames ( mInstanceExtensions.data() )
		.setPNext                   ( &mDebugCallbackCreateInfo ) /* <-- enables debugging the instance creation. */
		;

	mInstance = vk::createInstance( instanceCreateInfo );
	ofLog() << "Successfully created instance.";
}

// ----------------------------------------------------------------------

void ofVkRenderer::destroyInstance()
{
	vkDestroyInstance(mInstance, VK_NULL_HANDLE);
	mInstance = VK_NULL_HANDLE;
}
// ----------------------------------------------------------------------

void ofVkRenderer::createDevice()
{
	// enumerate physical devices list to find 
	// first available device
	auto deviceList = mInstance.enumeratePhysicalDevices();

	// CONSIDER: find the best appropriate GPU
	// Select a physical device (GPU) from the above queried list of options.
	// For now, we assume the first one to be the best one.
	mPhysicalDevice = deviceList.front();

	// query the gpu for more info about itself
	mPhysicalDeviceProperties = mPhysicalDevice.getProperties();

	ofLog() << "GPU Type: " << mPhysicalDeviceProperties.deviceName;

	{
		ofVkWindowSettings tmpSettings;
		tmpSettings.vkVersion = mPhysicalDeviceProperties.apiVersion;
		ofLog() << "GPU API Version: " << tmpSettings.getVkVersionMajor() << "."
			<< tmpSettings.getVersionMinor() << "." << tmpSettings.getVersionPatch();

		uint32_t driverVersion = mPhysicalDeviceProperties.driverVersion;
		ofLog() << "GPU Driver Version: " << std::hex << driverVersion;
	}

	// let's find out the devices' memory properties
	mPhysicalDeviceMemoryProperties = mPhysicalDevice.getMemoryProperties();

	// query queue families for the first queue supporting graphics
	{
		// query number of queue family properties
		std::vector<vk::QueueFamilyProperties> queueFamilyPropertyList = mPhysicalDevice.getQueueFamilyProperties();

		bool foundGraphics = false;
		for (uint32_t i = 0; i < queueFamilyPropertyList.size(); ++i) {
			// test queue family against flag bitfields
			if (queueFamilyPropertyList[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				foundGraphics = true;
				mVkGraphicsFamilyIndex = i;
				break;
			}
		}
		if (!foundGraphics) {
			ofLogError() << "Vulkan error: did not find queue family that supports graphics";
			ofExit(-1);
		}
	}


	// query debug layers available for instance
	{
		std::ostringstream console;

		vector<vk::LayerProperties> instanceLayerPropertyList = vk::enumerateInstanceLayerProperties();

		console << "Available Instance Layers:" << std::endl << std::endl;
		for (auto &l : instanceLayerPropertyList ) {
			console << std::right << std::setw( 40 ) << l.layerName << " : " << l.description << std::endl;
		}
		ofLog() << console.str();
	}

	// query debug layers available for physical device
	{
		std::ostringstream console;

		vector<vk::LayerProperties> deviceLayerPropertylist = mPhysicalDevice.enumerateDeviceLayerProperties();

		console << "Available Device Layers:" << std::endl << std::endl;
		for (auto &l : deviceLayerPropertylist ) {
			console << std::right << std::setw( 40 ) << l.layerName << " : " << l.description << std::endl;
		}
		ofLog() << console.str();
	}

	float queuePriority[] = { 1.f };

	vk::DeviceQueueCreateInfo queueCreateInfo;
	queueCreateInfo
		.setQueueFamilyIndex ( mVkGraphicsFamilyIndex) /* <-- vkGraphicsFamilyIndex was queried earlier, when we went through all available queues, and selected the first graphcis capable queue. */
		.setQueueCount       ( 1 )
		.setPQueuePriorities ( queuePriority )
		;

	// TODO: check which features must be switched on for 
	//       default openFrameworks operations.
	vk::PhysicalDeviceFeatures deviceFeatures = mPhysicalDevice.getFeatures();
	deviceFeatures
		.setFillModeNonSolid(VK_TRUE); // allow line drawing
	

	vk::DeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo
		.setQueueCreateInfoCount      ( 1 )
		.setPQueueCreateInfos         ( &queueCreateInfo )
		.setEnabledLayerCount         ( mDeviceLayers.size() )
		.setPpEnabledLayerNames       ( mDeviceLayers.data() )
		.setEnabledExtensionCount     ( mDeviceExtensions.size() )
		.setPpEnabledExtensionNames   ( mDeviceExtensions.data() )
		.setPEnabledFeatures          ( &deviceFeatures )
		;

	// create device	
	mDevice = mPhysicalDevice.createDevice( deviceCreateInfo );

	ofLogNotice() << "Successfully created Vulkan device";

	// fetch queue handle into mQueue
	mQueue = mDevice.getQueue( mVkGraphicsFamilyIndex, 0 );

	// query possible depth formats, find the 
	// first format that supports attachment as a depth stencil 
	//
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	std::vector<vk::Format> depthFormats = {
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD32Sfloat,
		vk::Format::eD24UnormS8Uint,
		vk::Format::eD16Unorm,
		vk::Format::eD16UnormS8Uint
	};

	for ( auto& format : depthFormats ){
		vk::FormatProperties formatProps = mPhysicalDevice.getFormatProperties( format );
		// Format must support depth stencil attachment for optimal tiling
		if ( formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment){
			mDepthFormat = format;
			break;
		}
	}

}

// ----------------------------------------------------------------------

void ofVkRenderer::destroyDevice()
{
	vkDestroyDevice(mDevice, VK_NULL_HANDLE);
	mDevice = VK_NULL_HANDLE;
}

// ----------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL
VulkanDebugCallback(
	VkDebugReportFlagsEXT		flags,					// what kind of error are we handling
	VkDebugReportObjectTypeEXT 	objType,				// type of object that caused the error
	uint64_t                    srcObj,					// pointer to the object that caused the error
	size_t                      location,				// ? could be source code line ?
	int32_t                     msgCode,				// ? how important this callback is ?
	const char*                 layer_prefix,			// which layer called this callback
	const char*                 msg,					// user readable string
	void *                      userData
) {
	
#ifdef WIN32
	static HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ){
		SetConsoleTextAttribute( hConsole, 12 + 0 * 16 );
	}
#endif // WIN32
	
	bool shouldBailout = false;
	std::string logLevel = "";

	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		logLevel = "INFO";
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		logLevel = "WARN";
	} else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
		logLevel = "PERF";
	} else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		logLevel = "ERROR";
		shouldBailout |= true;
	} else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
		logLevel = "DEBUG";
	}

	std::ostringstream os; 
	os << std::right << std::setw( 8 ) << logLevel << "{" << std::setw( 10 ) << layer_prefix << "}: " << msg << std::endl;

	ofLogNotice() << (os.str().substr(0,os.str().length()-1));
#ifdef WIN32
	SetConsoleTextAttribute( hConsole, 7 + 0 * 16 );
#endif
	// if error returns true, this layer will try to bail out and not forward the command
	return shouldBailout; 
}

// ----------------------------------------------------------------------

void ofVkRenderer::requestDebugLayers() {

	mInstanceLayers.push_back( "VK_LAYER_LUNARG_standard_validation" );
	mInstanceLayers.push_back("VK_LAYER_LUNARG_object_tracker");
	mInstanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	mDeviceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
	
}
// ----------------------------------------------------------------------

void ofVkRenderer::createDebugLayers()
{
	mDebugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	mDebugCallbackCreateInfo.pfnCallback = &VulkanDebugCallback;
	mDebugCallbackCreateInfo.flags =
		//VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT
		| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
		| VK_DEBUG_REPORT_ERROR_BIT_EXT
		| VK_DEBUG_REPORT_DEBUG_BIT_EXT
		| 0;		  // this should enable all flags.

	// first get (find) function pointers from sdk for callback [create / destroy] function addresses
	fVkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugReportCallbackEXT");
	fVkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugReportCallbackEXT");

	// we can't check against nullptr here, since 0x0 is not the same as nullptr and 
	// we would falsely get a positive feedback even if the sdk returns 0x0 as the address
	// for the function pointers.
	if (VK_NULL_HANDLE == fVkCreateDebugReportCallbackEXT || VK_NULL_HANDLE == fVkDestroyDebugReportCallbackEXT) {
		ofLogError() << "error fetching pointers for debug layer callbacks";
		ofExit(-1);
		return;
	}

	// this method is not available by default.
	{
		// note that we execute the function pointers we searched for earlier, 
		// since "vkCreateDebugReportCallbackEXT" is not directly exposed by vulkan-1.lib
		// fVkCreateDebugReportCallbackEXT is the function we want to call.
		fVkCreateDebugReportCallbackEXT(mInstance, &mDebugCallbackCreateInfo , VK_NULL_HANDLE, &mDebugReportCallback);
	}
}

// ----------------------------------------------------------------------

void ofVkRenderer::destroyDebugLayers()
{
	if ( mDebugReportCallback != VK_NULL_HANDLE ){
		fVkDestroyDebugReportCallbackEXT( mInstance, mDebugReportCallback, VK_NULL_HANDLE );
		// let's set our own callback address to 0 just to be on the safe side.
		mDebugReportCallback = VK_NULL_HANDLE;
	}
}
// ----------------------------------------------------------------------

ofRectangle ofVkRenderer::getCurrentViewport() const
{
	return mViewport;
}

// ----------------------------------------------------------------------

ofRectangle ofVkRenderer::getNativeViewport() const
{
	return mViewport;
}

// ----------------------------------------------------------------------

int ofVkRenderer::getViewportWidth() const
{
	return mWindowWidth;
}

// ----------------------------------------------------------------------

int ofVkRenderer::getViewportHeight() const
{
	return mWindowHeight;
}

// ----------------------------------------------------------------------

bool ofVkRenderer::isVFlipped() const
{
	return false;
}

// ----------------------------------------------------------------------

ofHandednessType ofVkRenderer::getCoordHandedness() const
{
	return ofHandednessType();
}

inline glm::mat4x4 ofVkRenderer::getCurrentMatrix( ofMatrixMode matrixMode_ ) const{
	return glm::mat4x4();
}

inline glm::mat4x4 ofVkRenderer::getCurrentOrientationMatrix() const{
	return glm::mat4x4();
}

// ----------------------------------------------------------------------


ofRectMode ofVkRenderer::getRectMode()
{
	return ofRectMode();
}

// ----------------------------------------------------------------------

ofFillFlag ofVkRenderer::getFillMode()
{
	return ofFillFlag();
}

// ----------------------------------------------------------------------

inline ofColor ofVkRenderer::getBackgroundColor(){
	return ofColor();
}

bool ofVkRenderer::getBackgroundAuto()
{
	return bBackgroundAuto;
}

// ----------------------------------------------------------------------

ofPath & ofVkRenderer::getPath()
{
	return mPath;
}

inline ofStyle ofVkRenderer::getStyle() const{
	return ofStyle();
}

// ----------------------------------------------------------------------

const of3dGraphics & ofVkRenderer::get3dGraphics() const
{
	return m3dGraphics;
}

// ----------------------------------------------------------------------

of3dGraphics & ofVkRenderer::get3dGraphics()
{
	return m3dGraphics;
}

// ----------------------------------------------------------------------

glm::mat4x4 ofVkRenderer::getCurrentViewMatrix() const{
	return glm::mat4x4();
}

// ----------------------------------------------------------------------
glm::mat4x4 ofVkRenderer::getCurrentNormalMatrix() const{
	return glm::mat4x4();
}

// ----------------------------------------------------------------------

//void ofVkRenderer::bind( const ofCamera & camera, const ofRectangle & viewport ){
//	
//	if ( mDefaultContext ){
//		mDefaultContext->pushMatrix();
//		mDefaultContext->setViewMatrix( camera.getModelViewMatrix() );
//
//		// Clip space transform:
//
//		// Vulkan has inverted y 
//		// and half-width z.
//
//		static const glm::mat4x4 clip( 1.0f, 0.0f, 0.0f, 0.0f,
//			0.0f, -1.0f, 0.0f, 0.0f,
//			0.0f, 0.0f, 0.5f, 0.0f,
//			0.0f, 0.0f, 0.5f, 1.0f );
//
//		mDefaultContext->setProjectionMatrix( clip * camera.getProjectionMatrix( viewport ) );
//	}
//}

// ----------------------------------------------------------------------

//void ofVkRenderer::unbind( const ofCamera& camera ){
//	if ( mDefaultContext )
//		mDefaultContext->popMatrix();
//}


