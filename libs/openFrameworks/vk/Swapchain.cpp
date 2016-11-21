#include "vk/Swapchain.h"
#include <vector>
#include "ofLog.h"
#include "GLFW/glfw3.h"

// CONSIDER: maybe using a settings object could make the setup method 
// less long to call.

void Swapchain::setup(
	const vk::Instance & instance_,
	const vk::Device & device_,
	const vk::PhysicalDevice & physicalDevice_,
	const vk::SurfaceKHR & surface_,
	const vk::SurfaceFormatKHR& surfaceFormat_,
	uint32_t & width_,
	uint32_t & height_,
	uint32_t & numSwapChainFrames_,
	vk::PresentModeKHR& presentMode_ )
{
	vk::Result err = vk::Result::eSuccess;

	mInstance = instance_;
	mDevice = device_;
	mPhysicalDevice = physicalDevice_;
	mWindowSurface = surface_;
	mColorFormat = surfaceFormat_;

	vk::SwapchainKHR oldSwapchain = mSwapchain;

	// Get physical device surface properties and formats
	vk::SurfaceCapabilitiesKHR surfCaps	= mPhysicalDevice.getSurfaceCapabilitiesKHR( mWindowSurface );
	
	// get available present modes for physical device
	std::vector<vk::PresentModeKHR> presentModes = mPhysicalDevice.getSurfacePresentModesKHR( mWindowSurface );

	// either set or get the swapchain surface extents
	vk::Extent2D swapchainExtent = {};

	if ( surfCaps.currentExtent.width == -1 ){
		swapchainExtent.width = width_;
		swapchainExtent.height = height_;
	}
	else{
		swapchainExtent = surfCaps.currentExtent;
		width_ = surfCaps.currentExtent.width;
		height_ = surfCaps.currentExtent.height;
	}

	// Prefer user-selected present mode, 
	// use guaranteed fallback mode (FIFO) if preferred mode couldn't be found.
	vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

	bool presentModeSwitchSuccessful = false;
	for ( auto & p : presentModes ){
		if ( p == presentMode_ ){
			swapchainPresentMode = p;
			presentModeSwitchSuccessful = true;
			break;
		}
	}

	if (!presentModeSwitchSuccessful){
		ofLogWarning() << "Could not switch to selected Swapchain Present Mode. Falling back to FIFO...";
	}

	// write current present mode back to reference from parameter
	// so caller can find out whether chosen present mode has been 
	// applied successfully.
	presentMode_ = swapchainPresentMode;

	uint32_t desiredNumberOfSwapchainImages = std::max<uint32_t>( surfCaps.minImageCount, numSwapChainFrames_ );
	if ( ( surfCaps.maxImageCount > 0 ) && ( desiredNumberOfSwapchainImages > surfCaps.maxImageCount ) ){
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// write current value back to parameter reference so caller
	// has a chance to check if values were applied correctly.
	numSwapChainFrames_ = desiredNumberOfSwapchainImages;

	vk::SurfaceTransformFlagBitsKHR preTransform;
	// Note: this will be interesting for mobile devices
	// - if rotation and mirroring for the final output can 
	// be defined here.

	if ( surfCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity){
		preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	} else{
		preTransform = surfCaps.currentTransform;
	}

	vk::SwapchainCreateInfoKHR swapChainCreateInfo;

	swapChainCreateInfo
		.setSurface           ( mWindowSurface )
		.setMinImageCount     ( desiredNumberOfSwapchainImages)
		.setImageFormat       ( mColorFormat.format)
		.setImageColorSpace   ( mColorFormat.colorSpace)
		.setImageExtent       ( swapchainExtent )
		.setImageArrayLayers  ( 1 )
		.setImageUsage        ( vk::ImageUsageFlagBits::eColorAttachment )
		.setImageSharingMode  ( vk::SharingMode::eExclusive )
		.setPreTransform      ( preTransform  )
		.setCompositeAlpha    ( vk::CompositeAlphaFlagBitsKHR::eOpaque )
		.setPresentMode       ( presentMode_ )
		.setClipped           ( VK_TRUE )
		.setOldSwapchain      ( oldSwapchain )
		;

	mSwapchain = mDevice.createSwapchainKHR( swapChainCreateInfo );

	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	if ( oldSwapchain ){
		mDevice.destroySwapchainKHR( oldSwapchain );
		oldSwapchain = nullptr;
	}

	std::vector<vk::Image> swapchainImages = mDevice.getSwapchainImagesKHR( mSwapchain );
	mImageCount = swapchainImages.size();

	// Swapchain re-created because of window resize, most possibly
	// therefore we have to destroy old ImageView object(s).

	if ( !mImages.empty() ){
		for ( auto&b : mImages ){
			mDevice.destroyImageView( b.view );
		}
	}

	mImages.resize( mImageCount );

	for ( uint32_t i = 0; i < mImageCount; i++ ){
		
		mImages[i].imageRef = swapchainImages[i];
		
		vk::ComponentMapping componentMapping { 
			vk::ComponentSwizzle::eR,
			vk::ComponentSwizzle::eG,
			vk::ComponentSwizzle::eB,
			vk::ComponentSwizzle::eA
		};
	
		vk::ImageSubresourceRange subresourceRange;
		subresourceRange
			.setAspectMask       ( vk::ImageAspectFlagBits::eColor )
			.setBaseMipLevel     ( 0 )
			.setLevelCount       ( 1 )
			.setBaseArrayLayer   ( 0 )
			.setLayerCount       ( 1 )
			;

		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo
			.setImage            ( mImages[i].imageRef )
			.setViewType         ( vk::ImageViewType::e2D )
			.setFormat           ( mColorFormat.format )
			.setComponents       ( componentMapping )
			.setSubresourceRange ( subresourceRange )
			;
		// create image view for color image
		mImages[i].view = mDevice.createImageView( imageViewCreateInfo );
		
	}

}

// ----------------------------------------------------------------------

void Swapchain::reset(){

	// it's imperative we clean up.
	
	for ( auto&b : mImages ){
		// note that we only destroy the VkImageView,
		// as the VkImage is owned by the swapchain mSwapchain
		// and will get destroyed when destroying the swapchain
		vkDestroyImageView( mDevice, b.view, nullptr );
	}
	mImages.clear();

	vkDestroySwapchainKHR( mDevice, mSwapchain, nullptr );
}

// ----------------------------------------------------------------------

// Acquires the next image in the swap chain
// blocks cpu until image has been acquired
// signals semaphorePresentComplete once image has been acquired
vk::Result Swapchain::acquireNextImage( vk::Semaphore semaphorePresentComplete, uint32_t *imageIndex ){
	// TODO: research:
	// because we are blocking here, could this affect our frame rate? could it take away time for cpu work?
	// we somehow need to make sure to keep the internal time value increasing in regular intervals,
	// tracking the current frame number!

	//mImageIndex = mDevice.acquireNextImageKHR( mSwapchain, UINT64_MAX, semaphorePresentComplete, nullptr );

	auto err = vkAcquireNextImageKHR( mDevice, mSwapchain, UINT64_MAX, semaphorePresentComplete, ( VkFence )nullptr, imageIndex );
	
	if ( err != VK_SUCCESS ){
		ofLogWarning() << "image acquisition returned: " << err;
	}
	mImageIndex = *imageIndex;

	return vk::Result(err);
}

// ----------------------------------------------------------------------
  
// Present the current image to the queue
vk::Result Swapchain::queuePresent( vk::Queue queue, uint32_t currentBuffer ){
	std::vector<vk::Semaphore> noSemaphores;
	return queuePresent( queue, currentBuffer, noSemaphores);
}

// ----------------------------------------------------------------------

vk::Result Swapchain::queuePresent( vk::Queue queue, uint32_t currentImageIndex, std::vector<vk::Semaphore> waitSemaphores_ ){
	
	vk::PresentInfoKHR presentInfo;
	presentInfo
		.setWaitSemaphoreCount( waitSemaphores_.size() )
		.setPWaitSemaphores( waitSemaphores_.data())
		.setSwapchainCount( 1 )
		.setPSwapchains( &mSwapchain )
		.setPImageIndices( &currentImageIndex )
		;

	// each command wich begins with vkQueue... is appended to the end of the 
	// queue. this includes presenting.
	return queue.presentKHR( presentInfo );
}

