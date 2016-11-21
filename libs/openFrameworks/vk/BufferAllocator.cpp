#include "vk/BufferAllocator.h"
#include "ofLog.h"

using namespace of::vk;

// ----------------------------------------------------------------------

void BufferAllocator::setup(){
	
	if ( mSettings.frameCount < 1 ){
		ofLogWarning() << "Allocator: Must have a minimum of 1 frame. Setting frames to 1.";
		const_cast<uint32_t&>( mSettings.frameCount ) = 1;
	}

	// we need to find out the min buffer uniform alignment from the 
	// physical device.

	// make this dependent on the type of buffer this allocator stands for 
	const_cast<::vk::DeviceSize&>( mAlignment )     = mSettings.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

	// make sure reserved memory is multiple of alignment, and that we can fit in the number of requested frames.	
	const_cast<::vk::DeviceSize&>( mSettings.size ) = mSettings.frameCount * mAlignment * ( ( mSettings.size / mSettings.frameCount + mAlignment - 1 ) / mAlignment );

	::vk::BufferCreateInfo bufferCreateInfo;
	
	bufferCreateInfo
		.setSize( mSettings.size)
		.setUsage( mSettings.bufferUsageFlags )
		.setSharingMode( ::vk::SharingMode::eExclusive )
		;

	// allocate physical memory from device
	
	// 1. create a buffer
	// 2. add backing memory to buffer
	
	mBuffer = mSettings.device.createBuffer( bufferCreateInfo );
	
	// 2.1 Get memory requirements including size, alignment and memory type 
	::vk::MemoryRequirements memReqs = mSettings.device.getBufferMemoryRequirements( mBuffer );
	
	// 2.2 Get the appropriate memory type for this type of buffer allocation
	// Only memory types that are visible to the host and coherent (coherent means they
	// appear to the GPU without the need of explicit range flushes)
	// Vulkan 1.0 guarantees the presence of at least one host-visible+coherent memory heap.
	::vk::MemoryAllocateInfo allocateInfo;
	
	bool result = getMemoryAllocationInfo(
		mSettings.physicalDeviceMemoryProperties,
		memReqs,
		mSettings.memFlags,
		allocateInfo
	);

	// 2.3 Finally, to the allocation
	// todo: check for and recover from allocation errors
	mDeviceMemory = mSettings.device.allocateMemory( allocateInfo );

	// 2.4 Attach memory to buffer (buffer must not be already backed by memory)
	mSettings.device.bindBufferMemory( mBuffer, mDeviceMemory, 0 );
	

	mOffsetEnd.clear();
	mOffsetEnd.resize( mSettings.frameCount, 0 );

	mBaseAddress.clear();
	mBaseAddress.resize( mSettings.frameCount, 0 );

	if ( mSettings.memFlags & ::vk::MemoryPropertyFlagBits::eHostVisible ){
		// Map full memory range for CPU write access
		mBaseAddress[0] = (uint8_t*)mSettings.device.mapMemory( mDeviceMemory, 0, VK_WHOLE_SIZE );
	} else{
		mBaseAddress[0] = 0;
	}

	for ( uint32_t i = 1; i != mBaseAddress.size(); ++i ){
		// offset the pointer by full frame sizes
		// for base addresses above frame 0
		mBaseAddress[i] = mBaseAddress[0] + i * ( mSettings.size / mSettings.frameCount );
	}

	mCurrentMappedAddress = nullptr;
	mCurrentVirtualFrameIdx = 0;
}

// ----------------------------------------------------------------------

void of::vk::BufferAllocator::reset(){
	if ( mDeviceMemory ){
		if ( mSettings.memFlags & ::vk::MemoryPropertyFlagBits::eHostVisible ){
			mSettings.device.unmapMemory( mDeviceMemory );
		}
		mSettings.device.freeMemory( mDeviceMemory );
		mDeviceMemory = nullptr;
	}

	if ( mBuffer ){
		mSettings.device.destroyBuffer( mBuffer );
		mBuffer = nullptr;
	}

	mOffsetEnd.clear();
	mBaseAddress.clear();
	mCurrentMappedAddress = nullptr;
	mCurrentVirtualFrameIdx = 0;
}

// ----------------------------------------------------------------------
// brief   linear allocator
// param   byteCount number of bytes to allocate
// param   frameIdx current virtual frame index
// returns offset memory offset in bytes relative to start of buffer to reach address
bool BufferAllocator::allocate( ::vk::DeviceSize byteCount_, ::vk::DeviceSize& offset ){
	uint32_t alignedByteCount = mAlignment * ( ( byteCount_ + mAlignment - 1 ) / mAlignment );

	if ( mOffsetEnd[mCurrentVirtualFrameIdx] + alignedByteCount <= (mSettings.size / mSettings.frameCount) ){
		// write out memory address
		mCurrentMappedAddress = mBaseAddress[mCurrentVirtualFrameIdx] + mOffsetEnd[mCurrentVirtualFrameIdx];
		// write out offset 
		offset = mOffsetEnd[mCurrentVirtualFrameIdx] + mCurrentVirtualFrameIdx * ( mSettings.size / mSettings.frameCount );
		mOffsetEnd[mCurrentVirtualFrameIdx] += alignedByteCount;
		// TODO: if you use non-coherent memory you need to invalidate the 
		// cache for the memory that has been written to.
		// What we will realistically do is to flush the full memory range occpuied by a frame
		// instead of the list of sub-allocations.
		return true;
	} else {
		ofLogError() << "Buffer Allocator: out of memory";
		// TODO: try to recover by re-allocating or something.
		// Whatever we do here, it will be very costly and should 
		// be avoided.
	}
	return false;
}

// ----------------------------------------------------------------------

void BufferAllocator::free(){
	mOffsetEnd[mCurrentVirtualFrameIdx] = 0;
	mCurrentMappedAddress = nullptr;
}

// ----------------------------------------------------------------------

void BufferAllocator::swap(){
	mCurrentVirtualFrameIdx = ( mCurrentVirtualFrameIdx + 1 ) % mSettings.frameCount;
}
