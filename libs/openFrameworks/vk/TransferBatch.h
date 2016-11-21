#pragma once

#include <vulkan/vulkan.hpp>
#include "vk/RenderContext.h"

namespace of{
namespace vk{

/*

TransferBatch is owned by a rendercontext - there is one transferbatch per virtual frame 
in each rendercontext. This is necessary so that buffers may be marked as "transferred" 
once the virtual frame has made the round-trip across the virtual frame fence, meaning
that all command buffers within the virtual frame have completed execution.

Transfer command buffers will be sent to the queue before draw, in the queue submission 
triggered by the rendercontext. This submission is bounded by a fence. Once that virtual 
frame fence has been waited upon, we can assume safely that all draw commands, and all 
transfers have completed execution.

*/

class BufferObject;

class TransferBatch 
{
	friend RenderContext;

	// These methods may only be called by RenderContext
	void signalTransferComplete();


private:

	RenderContext *                    mRenderContext;
	
	// batch which accumulates all submitted batches whilst the frame is in flight
	std::list<std::shared_ptr<BufferObject>> mInflightBatch;
	// batch which accumulates transfers until submit
	std::list<std::shared_ptr<BufferObject>> mBatch;

	TransferBatch() = delete;

public:
	
	TransferBatch( RenderContext* context_ )
	: mRenderContext(context_)
	{
		
	};

	~TransferBatch(){
		// as we don't own anything, there is nothing to destroy.
	};

	bool add( std::shared_ptr<BufferObject>& buffer );

	void submit();

};


} // end namespace of::vk
} // end namespace of
