#include "vk/ImageAllocator.h"
#include "ofLog.h"

using namespace of::vk;

// ----------------------------------------------------------------------

void ImageAllocator::setup(){
	
	const_cast<::vk::DeviceSize&>( mImageGranularity ) = mSettings.physicalDeviceProperties.limits.bufferImageGranularity;

	// make sure reserved memory is multiple of alignment (= ImageGranularity)
	const_cast<::vk::DeviceSize&>( mSettings.size ) = mImageGranularity * ( ( mSettings.size  + mImageGranularity - 1 ) / mImageGranularity );

	::vk::MemoryRequirements memReqs;
	{
		::vk::ImageCreateInfo imageCreateInfo;
		::vk::ImageFormatProperties imageFormatProperties;
		::vk::Format format = ::vk::Format::eR8G8B8A8Unorm;

		imageCreateInfo
			.setImageType( ::vk::ImageType::e2D )
			.setFormat( format )
			.setExtent( { 1, 1, 1 } )
			.setMipLevels( 1 )
			.setArrayLayers( 1 )
			.setSamples( ::vk::SampleCountFlagBits::e1 )
			.setTiling( mSettings.imageTiling )
			.setUsage( mSettings.imageUsageFlags )
			.setSharingMode( ::vk::SharingMode::eExclusive )
			.setQueueFamilyIndexCount( 0 )
			.setPQueueFamilyIndices( nullptr )
			.setInitialLayout( ::vk::ImageLayout::eUndefined )
			;

		::vk::Image tmpImage = mSettings.device.createImage( imageCreateInfo );

		// Get memory requirements - we're really just interested in memory type bits
		memReqs = mSettings.device.getImageMemoryRequirements( tmpImage );
		
		mSettings.device.destroyImage( tmpImage );
	}

	memReqs.size      = mSettings.size;
	memReqs.alignment = mImageGranularity;
	
	::vk::MemoryAllocateInfo allocateInfo;
	
	bool result = getMemoryAllocationInfo(
		mSettings.physicalDeviceMemoryProperties,
		memReqs,
		mSettings.memFlags,
		allocateInfo
	);

	mDeviceMemory = mSettings.device.allocateMemory( allocateInfo );

	mOffsetEnd = 0;
}

// ----------------------------------------------------------------------

void of::vk::ImageAllocator::reset(){
	if ( mDeviceMemory ){
		mSettings.device.freeMemory( mDeviceMemory );
		mDeviceMemory = nullptr;
	}
	
	mOffsetEnd = 0;
}

// ----------------------------------------------------------------------
// brief   linear allocator
// param   byteCount number of bytes to allocate
// param   frameIdx current virtual frame index
bool ImageAllocator::allocate( ::vk::DeviceSize byteCount_, ::vk::DeviceSize& offset ){
	uint32_t alignedByteCount = mImageGranularity * ( ( byteCount_ + mImageGranularity - 1 ) / mImageGranularity );

	if ( mOffsetEnd + alignedByteCount <= (mSettings.size) ){
		// write out offset 
		offset = mOffsetEnd;
		mOffsetEnd += alignedByteCount;
		return true;
	} else{
		ofLogError() << "Image Allocator: out of memory";
	}
	return false;
}

// ----------------------------------------------------------------------

void ImageAllocator::free(){
	mOffsetEnd = 0;
}

// ----------------------------------------------------------------------

void ImageAllocator::swap(){
}
