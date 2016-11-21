#pragma once

#include "vk/Allocator.h"

namespace of{
namespace vk{

// ----------------------------------------------------------------------


/*
	BufferAllocator is a simple linear allocator.

	Allocator may have more then one virtual frame,
	and only allocations from the current virutal 
	frame are performed until swap(). 

	Allocator may be for transient memory or for 
	static memory.

	If allocated from Host memory, the allocator 
	maps a buffer to CPU visible memory for its 
	whole lifetime. 

*/


class ImageAllocator : public AbstractAllocator
{

public:

	struct Settings : public AbstractAllocator::Settings
	{
		::vk::ImageUsageFlags imageUsageFlags = (
			::vk::ImageUsageFlagBits::eTransferDst
			| ::vk::ImageUsageFlagBits::eSampled
			);

		::vk::ImageTiling imageTiling = ::vk::ImageTiling::eOptimal;
	};

	ImageAllocator( const ImageAllocator::Settings& settings )
		: mSettings( settings ){};

	~ImageAllocator(){
		mSettings.device.waitIdle();
		reset();
	};

	/// @detail set up allocator based on Settings and pre-allocate 
	///         a chunk of GPU memory, and attach a buffer to it 
	void setup() override;

	/// @brief  free GPU memory and de-initialise allocator
	void reset() override;

	/// @brief  sub-allocate a chunk of memory from GPU
	/// 
	bool allocate( ::vk::DeviceSize byteCount_, ::vk::DeviceSize& offset ) override;

	void swap() override;

	const ::vk::DeviceMemory& getDeviceMemory() const override;

	/// @brief  remove all sub-allocations within the given frame
	/// @note   this does not free GPU memory, it just marks it as unused
	void free();

	// jump to use next segment assigned to next virtual frame


	const AbstractAllocator::Settings& getSettings() const override{
		return mSettings;
	}

private:
	const ImageAllocator::Settings     mSettings;
	const ::vk::DeviceSize             mImageGranularity = (10 << 1UL);  // granularity is calculated on setup. must be power of two.

	::vk::DeviceSize                   mOffsetEnd;                // next free location for allocations
	::vk::DeviceMemory                 mDeviceMemory = nullptr;	  // owning

};

// ----------------------------------------------------------------------

inline const ::vk::DeviceMemory & of::vk::ImageAllocator::getDeviceMemory() const{
	return mDeviceMemory;
}

// ----------------------------------------------------------------------


} // namespace of::vk
} // namespace of