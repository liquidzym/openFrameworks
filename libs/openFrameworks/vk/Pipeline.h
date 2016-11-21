#pragma once

#include "vulkan/vulkan.hpp"
#include <string>
#include <array>

#include "ofFileUtils.h"
#include "ofLog.h"

/*

A pipeline is a monolithic compiled object that represents 
all the programmable, and non-dynamic state affecting a 
draw call. 

You can look at it as a GPU program combining shader machine 
code with gpu-hardware-specific machine code dealing with 
blending, primitive assembly, etc. 

The Pipeline has a layout, that's the "function signature" so
to say, for the uniform parameters. You feed these parameters 
when you bind descriptor sets to the command buffer which you
are currently recording. A pipeline bound to the same command
buffer will then use these inputs.

Note that you *don't* bind to the pipeline directly, 
but you bind both pipeline layout and descriptor sets 
TO THE CURRENT COMMAND BUFFER. 

Imagine the Command Buffer as the plugboard, and the Pipeline 
Layout plugging wires in on one side, and the descriptor sets 
plugging wires in on the other side. 

A pipeline can have some dynamic state, that is state which is
controlled by the command buffer. State which may be dynamic is
pretty limited, and has to be defined when you create a pipeline.

When a pipeline is created it is effectively compiled into a 
GPU program. Different non-dynamic pipeline state needs a different
pipeline. That's why you potentially need a pipeline for all 
possible combinations of states that you may use.

## Mission Statement

This class helps you create pipelines, and it also wraps pipeline
caching, so that pipelines can be requested based on dynamic state
and will be either created dynamically or created upfront. 

This class shall also help you to create pipeline layouts, based
on how your shaders are defined. It will try to match shader 
information gained through reflection (using spriv-cross) with 
descriptorSetLayouts to see if things are compatible.

The API will return VK handles, so that other libraries can be used
on top or alternatively to this one.


*/

namespace of{
namespace vk{

class Shader;

// ------------------------------------------------------------

class ComputePipelineState
{

private:

	int32_t                                mBasePipelineIndex = -1;
	mutable VkBool32                       mDirty = true;  // whether this pipeline state is dirty.

														   // shader allows us to derive pipeline layout, has public getters and setters.
	std::shared_ptr<of::vk::Shader>        mShader;

public:

	const std::shared_ptr<Shader>          getShader() const;
	void                                   setShader( const std::shared_ptr<Shader> & shader );
	void                                   touchShader() const;

	::vk::Pipeline createPipeline( const ::vk::Device& device, const std::shared_ptr<::vk::PipelineCache>& pipelineCache, ::vk::Pipeline basePipelineHandle = nullptr );

	uint64_t calculateHash() const;

	bool  operator== ( ComputePipelineState const & rhs );
	bool  operator!= ( ComputePipelineState const & rhs ){
		return !operator==( rhs );
	};

};

// ------------------------------------------------------------

class GraphicsPipelineState
{

	// when we build the command buffer, we need to check 
	// if the current context state is matched by an already 
	// available pipeline. 
	// 
	// if it isn't, we have to compile a pipeline for the command
	// 
	// if it is, we bind that pipeline.


public:	// these states can be set upfront

	::vk::PipelineInputAssemblyStateCreateInfo              inputAssemblyState;
	::vk::PipelineTessellationStateCreateInfo               tessellationState;
	::vk::PipelineViewportStateCreateInfo                   viewportState;
	::vk::PipelineRasterizationStateCreateInfo              rasterizationState;
	::vk::PipelineMultisampleStateCreateInfo                multisampleState;
	::vk::PipelineDepthStencilStateCreateInfo               depthStencilState;
	
	std::array<::vk::DynamicState, 2>                       dynamicStates;
	std::array<::vk::PipelineColorBlendAttachmentState,8>   blendAttachmentStates; // 8 == max color attachments.
	
	::vk::PipelineColorBlendStateCreateInfo                 colorBlendState;

	::vk::PipelineDynamicStateCreateInfo                    dynamicState;

private: // these states must be received through context

	// non-owning - note that renderpass may be inherited from a 
	// primary command buffer.
	mutable ::vk::RenderPass  mRenderPass         = 0;
	mutable uint32_t          mSubpass            = 0;

private:
	int32_t                   mBasePipelineIndex  = -1;

	// shader allows us to derive pipeline layout, has public getters and setters.
	std::shared_ptr<of::vk::Shader>        mShader;

public:
	
	GraphicsPipelineState();

	void reset();

	uint64_t calculateHash() const;

	// whether this pipeline state is dirty.
	mutable VkBool32          mDirty              = true;

	const std::shared_ptr<Shader> getShader() const;
	void                          setShader( const std::shared_ptr<Shader> & shader );
	void                          touchShader() const;

	void setRenderPass( const ::vk::RenderPass& renderPass ) const {
		if ( renderPass != mRenderPass ){
			mRenderPass = renderPass;
			mDirty = true;
		}
	}

	const ::vk::RenderPass & getRenderPass() const{
		return mRenderPass;
	}

	void setSubPass( uint32_t subpassId ) const {
		if ( subpassId != mSubpass ){
			mSubpass = subpassId;
			mDirty = true;
		}
	}

	::vk::Pipeline createPipeline( const ::vk::Device& device, const std::shared_ptr<::vk::PipelineCache>& pipelineCache, ::vk::Pipeline basePipelineHandle = nullptr );

	bool  operator== ( GraphicsPipelineState const & rhs );
	bool  operator!= ( GraphicsPipelineState const & rhs ){
		return !operator==( rhs );
	};

};

// ----------------------------------------------------------------------

/// \brief  Create a pipeline cache object
/// \detail Optionally load from disk, if filepath given.
/// \note  	Ownership: passed on.
static std::shared_ptr<::vk::PipelineCache> createPipelineCache( const ::vk::Device& device, std::string filePath = "" ){
	::vk::PipelineCache cache;

	ofBuffer cacheFileBuffer;
	::vk::PipelineCacheCreateInfo info;

	if ( ofFile( filePath ).exists() ){
		cacheFileBuffer = ofBufferFromFile( filePath, true );
		info.setInitialDataSize( cacheFileBuffer.size() );
		info.setPInitialData( cacheFileBuffer.getData() );
	}

	auto result = std::shared_ptr<::vk::PipelineCache>(
		new ::vk::PipelineCache( device.createPipelineCache( info ) ), [d = device]( ::vk::PipelineCache* rhs ){
		if ( rhs ){
			d.destroyPipelineCache( *rhs );
			delete( rhs );
		}
	} );

	return result;
};

} // namespace vk
} // namespace of

// ----------------------------------------------------------------------

inline const std::shared_ptr<of::vk::Shader> of::vk::GraphicsPipelineState::getShader() const{
	return mShader;
}

inline const std::shared_ptr<of::vk::Shader> of::vk::ComputePipelineState::getShader() const{
	return mShader;
}
