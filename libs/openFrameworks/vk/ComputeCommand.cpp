#include "vk/ComputeCommand.h"
#include "vk/spooky/SpookyV2.h"

using namespace of::vk;
// setup all non-transient state for this draw object

// current ubo values are stored with draw command

// think about it as immutable DATA versus STATE - we want immutable DATA
// not state. DATA is Plain Old Data - and this is how the draw command 
// must store itself.

// ----------------------------------------------------------------------


void ComputeCommand::setup( const ComputePipelineState& pipelineState ){

	mPipelineState = pipelineState;

	mDescriptorSetData = mPipelineState.getShader()->getDescriptorSetData();
	mUniformDictionary = mPipelineState.getShader()->getUniformDictionary();

}

// ------------------------------------------------------------

void ComputeCommand::commitUniforms( const std::unique_ptr<BufferAllocator>& alloc ){

	for ( auto & descriptorSetData : mDescriptorSetData ){

		auto imgInfoIt        = descriptorSetData.imageAttachment.begin();
		auto bufferInfoIt     = descriptorSetData.bufferAttachment.begin();
		auto dynamicOffsetsIt = descriptorSetData.dynamicBindingOffsets.begin();
		auto dataIt           = descriptorSetData.dynamicUboData.begin();

		for ( auto & descriptor : descriptorSetData.descriptors ){

			switch ( descriptor.type ){
			case ::vk::DescriptorType::eSampler:
				break;
			case ::vk::DescriptorType::eCombinedImageSampler:
			{
				descriptor.imageView   = imgInfoIt->imageView;
				descriptor.sampler     = imgInfoIt->sampler;
				descriptor.imageLayout = imgInfoIt->imageLayout;
				imgInfoIt++;
			}
			break;
			case ::vk::DescriptorType::eSampledImage:
				break;
			case ::vk::DescriptorType::eStorageImage:
				break;
			case ::vk::DescriptorType::eUniformTexelBuffer:
				break;
			case ::vk::DescriptorType::eStorageTexelBuffer:
				break;
			case ::vk::DescriptorType::eUniformBuffer:
				break;
			case ::vk::DescriptorType::eUniformBufferDynamic:
			{
				descriptor.buffer = alloc->getBuffer();
				::vk::DeviceSize offset;
				void * dataP = nullptr;

				const auto & dataVec = *dataIt;
				const auto & dataRange = dataVec.size();

				// allocate data on gpu
				if ( alloc->allocate( dataRange, offset ) && alloc->map( dataP ) ){

					// copy data to gpu
					memcpy( dataP, dataVec.data(), dataRange );

					// update dynamic binding offsets for this binding
					*dynamicOffsetsIt = offset;

				} else{
					ofLogError() << "commitUniforms: could not allocate transient memory.";
				}
				dataIt++;
				dynamicOffsetsIt++;
				descriptor.range = dataRange;
			}
			break;
			case ::vk::DescriptorType::eStorageBuffer:
				break;
			case ::vk::DescriptorType::eStorageBufferDynamic:
				descriptor.buffer = bufferInfoIt->buffer;
				descriptor.range  = bufferInfoIt->range;
				*dynamicOffsetsIt = bufferInfoIt->offset;

				bufferInfoIt++;
				dynamicOffsetsIt++;
				break;
			case ::vk::DescriptorType::eInputAttachment:
				break;
			default:
				break;
			} // end switch

		} // end for each descriptor
	} // end foreach mDescriptorSetData
}

// ------------------------------------------------------------

void ComputeCommand::submit( RenderContext & context, const glm::uvec3& dims = {256,256,1} ){

	commitUniforms( context.getTransientAllocator() );

	auto cmd = context.requestPrimaryCommandBuffer();


	// current draw state for building command buffer - this is based on parsing the drawCommand list
	std::unique_ptr<ComputePipelineState> boundPipelineState;

		// find out pipeline state needed for this draw command

	if ( !boundPipelineState || *boundPipelineState != mPipelineState ){
		// look up pipeline in pipeline cache
		// otherwise, create a new pipeline, then bind pipeline.

		boundPipelineState = std::make_unique<ComputePipelineState>( mPipelineState );

		uint64_t pipelineStateHash = boundPipelineState->calculateHash();

		auto & currentPipeline = context.borrowPipeline( pipelineStateHash );

		if ( currentPipeline.get() == nullptr ){
			currentPipeline =
				std::shared_ptr<::vk::Pipeline>( ( new ::vk::Pipeline ),
					[device = context.mDevice]( ::vk::Pipeline*rhs ){
				if ( rhs ){
					device.destroyPipeline( *rhs );
				}
				delete rhs;
			} );

			*currentPipeline = boundPipelineState->createPipeline( context.mDevice, context.mSettings.pipelineCache );
		}

		cmd.bindPipeline( ::vk::PipelineBindPoint::eCompute, *currentPipeline );
	}

	// ----------| invariant: correct pipeline is bound

	// Match currently bound DescriptorSetLayouts against 
	// dc pipeline DescriptorSetLayouts
	std::vector<::vk::DescriptorSet> boundVkDescriptorSets;
	std::vector<uint32_t> dynamicBindingOffsets;

	const std::vector<uint64_t> & setLayoutKeys = mPipelineState.getShader()->getDescriptorSetLayoutKeys();

	for ( size_t setId = 0; setId != setLayoutKeys.size(); ++setId ){

		uint64_t setLayoutKey = setLayoutKeys[setId];
		auto & descriptors = getDescriptorSetData( setId ).descriptors;
		const auto desciptorSet = mPipelineState.getShader()->getDescriptorSetLayout( setId );
		// calculate hash of descriptorset, combined with descriptor set sampler state
		uint64_t descriptorSetHash = SpookyHash::Hash64( descriptors.data(), descriptors.size() * sizeof( DescriptorSetData_t::DescriptorData_t ), setLayoutKey );

		// Receive a descriptorSet from the renderContext's cache.
		// The renderContext will allocate and initialise a DescriptorSet if none has been found.
		const ::vk::DescriptorSet& descriptorSet = context.getDescriptorSet( descriptorSetHash, setId, *desciptorSet, descriptors );

		boundVkDescriptorSets.emplace_back( descriptorSet );

		const auto & offsets = getDescriptorSetData( setId ).dynamicBindingOffsets;

		// now append dynamic binding offsets for this set to vector of dynamic offsets for this draw call
		dynamicBindingOffsets.insert( dynamicBindingOffsets.end(), offsets.begin(), offsets.end() );

	}

	// We always bind the full descriptor set.
	// Bind uniforms (the first set contains the matrices)

	// bind dc descriptorsets to current pipeline descriptor sets
	// make sure dynamic ubos have the correct offsets
	if ( !boundVkDescriptorSets.empty() ){
		cmd.bindDescriptorSets(
			::vk::PipelineBindPoint::eCompute,	                           // use compute, not graphics pipeline
			*mPipelineState.getShader()->getPipelineLayout(),              // VkPipelineLayout object used to program the bindings.
			0,                                                             // firstset: first set index (of the above) to bind to - mDescriptorSet[0] will be bound to pipeline layout [firstset]
			boundVkDescriptorSets.size(),                                  // setCount: how many sets to bind
			boundVkDescriptorSets.data(),                                  // the descriptor sets to match up with our mPipelineLayout (need to be compatible)
			dynamicBindingOffsets.size(),                                  // dynamic offsets count how many dynamic offsets
			dynamicBindingOffsets.data()                                   // dynamic offsets for each descriptor
		);
	}

	cmd.dispatch( dims.x, dims.y, dims.z );

	cmd.end();

	context.submit( std::move( cmd ) );
}

// ------------------------------------------------------------
