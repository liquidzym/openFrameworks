#include "ofLog.h"
#include "vk/TransferBatch.h"

using namespace of::vk;
/*

We assume a transfer batch is issued *before* the render batch that might use the buffers for the first time,
from dynamic memory.

We can ensure this by requesting a transfer batch from a context.

This means, after the draw batch has been submitted, the draw batch fence being signalled means that the
command buffer used for the batch has completed execution.

Command buffers execute in sequence - so adding a transfer barrier into the copy command buffer means
that the copy command must have finished executing by the time the draw command buffer is executing.

Q: How do we tell BufferObjects that their transfer has concluded?
A: They could be added to a per-virtual frame list of buffers which are in transition.
Q: But what happens if a BufferObject is again changed whilst in transition?

Transfer batch needs to be attached to context.
Context can signal that virtual frame fence has been reached.
Once virtual frame fence was reached, we can dispose of dynamic data.

*/

// ----------------------------------------------------------------------

bool of::vk::TransferBatch::add( std::shared_ptr<BufferObject>& buffer ){

	//// check if buffer can be added

	//if ( !buffer->needsTransfer() ){
	//	ofLogVerbose() << "TransferBatch: Buffer does not need transfer.";
	//	return false;
	//}

	//// --------| invariant: buffer needs transfer.

	//// find the first element in the batch that matches the transient and 
	//// persistent buffer targets of the current buffer - if nothing found,
	//// return last element.
	//auto it = std::find_if( mBatch.begin(), mBatch.end(), [buffer]( std::shared_ptr<BufferObject> lhs ){
	//	return buffer->getTransientAllocator()->getBuffer() == lhs->getTransientAllocator()->getBuffer()
	//		&& buffer->getPersistentAllocator()->getBuffer() == lhs->getPersistentAllocator()->getBuffer();
	//} );

	//mBatch.insert( it, buffer );

	return true;
}

// ----------------------------------------------------------------------

void of::vk::TransferBatch::submit(){

	//if ( mBatch.empty() ){
	//	return;
	//}

	//auto & device  = mRenderContext->getDevice();
	//auto & cmdPool = mRenderContext->getCommandPool();

	//// First, we need a command buffer where we can record a pipeline barrier command into.
	//// This command - the pipeline barrier with an image barrier - will transfer the 
	//// image resource from its original layout to a layout that the gpu can use for 
	//// sampling.
	//::vk::CommandBuffer cmd = nullptr;
	//{
	//	::vk::CommandBufferAllocateInfo cmdBufAllocInfo;
	//	cmdBufAllocInfo
	//		.setCommandPool( cmdPool )
	//		.setLevel( ::vk::CommandBufferLevel::ePrimary )
	//		.setCommandBufferCount( 1 )
	//		;
	//	cmd = device.allocateCommandBuffers( cmdBufAllocInfo ).front();
	//}

	//std::vector<::vk::BufferCopy> bufferCopies;
	//bufferCopies.reserve( mBatch.size() );

	//::vk::Buffer srcBuf = nullptr;
	//::vk::Buffer dstBuf = nullptr;

	//cmd.begin( { ::vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );

	//for ( auto it = mBatch.cbegin(); it != mBatch.cend(); ++it ){
	//	const auto & bufferObject = *it;

	//	::vk::BufferCopy bufferCopy;
	//	bufferCopy
	//		.setSize( bufferObject->mRange )
	//		.setSrcOffset( bufferObject->mOffset )
	//		.setDstOffset( bufferObject->mPersistentOffset )
	//		;

	//	// if src or dst are different from last buffer, flush, and enqueue next one.
	//	if ( bufferObject->getTransientAllocator()->getBuffer() != srcBuf
	//		|| bufferObject->getPersistentAllocator()->getBuffer() != dstBuf ){

	//		if ( bufferCopies.empty() == false ){
	//			// submit buffer copies.
	//			cmd.copyBuffer( srcBuf, dstBuf, bufferCopies );
	//			bufferCopies.clear();
	//			bufferCopies.reserve( mBatch.size() );
	//		}
	//		srcBuf = bufferObject->getTransientAllocator()->getBuffer();
	//		dstBuf = bufferObject->getPersistentAllocator()->getBuffer();
	//	}

	//	bufferCopies.push_back( bufferCopy );

	//	if ( std::next( it ) == mBatch.cend() && !bufferCopies.empty() ){
	//		// submit buffer copies
	//		cmd.copyBuffer( srcBuf, dstBuf, bufferCopies );
	//	}

	//}

	//// CONSIDER: add transfer barrier
	//cmd.end();

	//mRenderContext->submit( std::move( cmd ) );

	//cmd = nullptr;

	//mInflightBatch.insert( mInflightBatch.end(), mBatch.begin(), mBatch.end());
	//mBatch.clear();
}

// ----------------------------------------------------------------------

void TransferBatch::signalTransferComplete(){

	////!TODO: decrease "inflight" count for each buffer
	//// found inside the batch.

	//for ( auto & b : mInflightBatch ){
	//	b->setTransferComplete();
	//}

	//mInflightBatch.clear();

}

// ----------------------------------------------------------------------

