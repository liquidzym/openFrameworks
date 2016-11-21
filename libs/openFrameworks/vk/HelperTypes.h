#pragma once

#include <vulkan/vulkan.hpp>

namespace of{
namespace vk{

struct TransferSrcData
{
	void * pData ;
	::vk::DeviceSize numElements;
	::vk::DeviceSize numBytesPerElement;
};

struct ImageTransferSrcData 
{
	void * pData;
	::vk::DeviceSize          numBytes;
	::vk::ImageType           imageType   { ::vk::ImageType::e2D };
	::vk::Format              format      { ::vk::Format::eR8G8B8A8Unorm };
	::vk::Extent3D            extent      { 0, 0, 1 };
	uint32_t                  mipLevels   { 1 };
	uint32_t                  arrayLayers { 1 };
	::vk::SampleCountFlagBits samples     { ::vk::SampleCountFlagBits::e1 };
};

struct BufferRegion
{
	::vk::Buffer buffer = nullptr;
	::vk::DeviceSize offset = 0;
	::vk::DeviceSize range = VK_WHOLE_SIZE;
	uint64_t numElements = 0;
};

} // end namespace of::vk
} // end namespace of