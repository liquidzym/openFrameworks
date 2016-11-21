#pragma once
#include "vulkan/vulkan.hpp"

namespace of{
namespace vk{

// TODO: Texture issues commands to the queue, and creates command buffers to do so.
//       There should be a way to make these operations more transparent. 
//       Also, transfer from host memory to device memory should be handled more transparently.

class Texture
{
private:
	
	const ::vk::Sampler     mSampler     = nullptr;
	const ::vk::ImageView   mImageView   = nullptr;
	const ::vk::ImageLayout mImageLayout = ::vk::ImageLayout::eShaderReadOnlyOptimal;

	const ::vk::Device      mDevice;

	Texture() = delete;

public:

	Texture(const ::vk::Device& device_, const ::vk::Image & image_);
	
	const ::vk::Sampler& getSampler() const{
		return mSampler;
	}

	const ::vk::ImageView& getImageView() const {
		return mImageView;
	}

	const ::vk::ImageLayout& getImageLayout() const{
		return mImageLayout;
	}

	~Texture();;
};

} /* namespace vk */
} /* namespace of */
