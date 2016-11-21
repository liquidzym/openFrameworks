#pragma once

#include "vulkan/vulkan.hpp"
#include <vector>

// TODO: rename
typedef struct
{
	::vk::Image imageRef;	   // owned by SwapchainKHR, only referenced here
	::vk::ImageView view;
} SwapchainImage;

class Swapchain {

	::vk::SwapchainKHR       mSwapchain      = nullptr;
	::vk::Instance           mInstance       = nullptr;
	::vk::Device             mDevice         = nullptr;
	::vk::PhysicalDevice     mPhysicalDevice = nullptr;

	::vk::SurfaceKHR         mWindowSurface  = nullptr;
	::vk::SurfaceFormatKHR   mColorFormat    = {};

	uint32_t             mImageCount = 0;
	uint32_t             mImageIndex = 0;

	std::vector<SwapchainImage> mImages;  // owning, clients may only borrow!

public:

	const std::vector<SwapchainImage> & getImages() const {
		return mImages;
	};

	const SwapchainImage& getImage( size_t i ) const{
		return mImages[i];
	};

	void setup( 
		const ::vk::Instance& instance_, 
		const ::vk::Device& device_, 
		const ::vk::PhysicalDevice& physicalDevice_, 
		const ::vk::SurfaceKHR& surface_,
		const ::vk::SurfaceFormatKHR& surfaceFormat_,
		uint32_t & width,
		uint32_t & height,
		uint32_t & numSwapChainFrames,
		::vk::PresentModeKHR & presentMode_
	);

	// resets all vk objects owned by this swapchain 
	void reset();

	// request an image from the swapchain, so that we might render to it
	// the image must be returned to the swapchain when done using 
	// queuePresent
	// \note this might cause waiting.
	vk::Result acquireNextImage( ::vk::Semaphore presentCompleteSemaphore, uint32_t *imageIndex );

	// mark the image ready to present by the swapchain.
	// this returns the image to the swapchain and tells the 
	// swapchain that we're done rendering to it and that 
	// it may show the image on screen.
	::vk::Result queuePresent( ::vk::Queue queue, uint32_t imageIndex );
	// Present the current image to the queue
	// Waits with execution until all waitSemaphores have been signalled
	::vk::Result queuePresent( ::vk::Queue queue, uint32_t imageIndex, std::vector<::vk::Semaphore> waitSemaphores_ );

	// return number of swapchain images
	inline const uint32_t & getImageCount() const { return mImageCount; };
	
	// return last acquired buffer id
	inline const uint32_t & getCurrentImageIndex() const { return mImageIndex; };

};
