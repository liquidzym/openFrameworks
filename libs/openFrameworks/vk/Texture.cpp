#include "vk/Texture.h"
#include "ofVkRenderer.h"

using namespace of::vk;

// ----------------------------------------------------------------------

Texture::Texture( const ::vk::Device & device_, const ::vk::Image & image_ )
	: mDevice(device_)
{
	
	::vk::ImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo
		.setImage( image_ )
		.setViewType( ::vk::ImageViewType::e2D )
		.setFormat( ::vk::Format::eR8G8B8A8Unorm )
		.setComponents( {::vk::ComponentSwizzle::eR, ::vk::ComponentSwizzle::eG, ::vk::ComponentSwizzle::eB, ::vk::ComponentSwizzle::eA} )
		.setSubresourceRange( { ::vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } )
		;

	const_cast<::vk::ImageView&>(mImageView) = mDevice.createImageView( imageViewCreateInfo );

	::vk::SamplerCreateInfo samplerInfo;
	samplerInfo
		.setMagFilter( ::vk::Filter::eLinear )
		.setMinFilter( ::vk::Filter::eLinear )
		.setMipmapMode( ::vk::SamplerMipmapMode::eLinear )
		.setAddressModeU( ::vk::SamplerAddressMode::eRepeat )
		.setAddressModeV( ::vk::SamplerAddressMode::eRepeat )
		.setAddressModeW( ::vk::SamplerAddressMode::eRepeat )
		.setMipLodBias( 0.f )
		.setAnisotropyEnable( VK_FALSE )
		.setMaxAnisotropy( 0.f )
		.setCompareEnable( VK_FALSE )
		.setCompareOp( ::vk::CompareOp::eLess )
		.setMinLod( 0.f )
		.setMaxLod( 1.f )
		.setBorderColor( ::vk::BorderColor::eFloatTransparentBlack)
		.setUnnormalizedCoordinates( VK_FALSE )
		;

	const_cast<::vk::Sampler&>(mSampler) = mDevice.createSampler( samplerInfo );

}

// ----------------------------------------------------------------------

Texture::~Texture(){

	if ( mDevice ){
		// TODO: find other way to make sure elements are ready 
		// for destruction (none of them must be in use or in flight.)
		mDevice.waitIdle();
		if ( mSampler ){
			mDevice.destroySampler( mSampler );
		}
		if ( mImageView ){
			mDevice.destroyImageView( mImageView );
		}
	}

}


