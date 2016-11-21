#include "vk/DrawCommand.h"
#include "vk/RenderBatch.h"
#include "vk/Shader.h"

using namespace of::vk;

// setup all non-transient state for this draw object

// current ubo values are stored with draw command

// think about it as immutable DATA versus STATE - we want immutable DATA
// not state. DATA is Plain Old Data - and this is how the draw command 
// must store itself.

// ----------------------------------------------------------------------


void DrawCommand::setup(const GraphicsPipelineState& pipelineState){
	
	mPipelineState = pipelineState;
	
	mDescriptorSetData = mPipelineState.getShader()->getDescriptorSetData();
	mUniformDictionary = mPipelineState.getShader()->getUniformDictionary();

	// parse shader info to find out how many buffers to reserve for vertex attributes.

	const auto & vertexInfo = mPipelineState.getShader()->getVertexInfo();

	size_t numAttributes = vertexInfo.attribute.size();

	mVertexBuffers.resize( numAttributes, nullptr );
	mVertexOffsets.resize( numAttributes, 0 );

	for ( size_t i = 0; i != numAttributes; ++i ){
		ofLog() << std::setw( 4 ) << i << ":" << vertexInfo.attributeNames[i];
	}
}

// ------------------------------------------------------------

void DrawCommand::commitUniforms(const std::unique_ptr<BufferAllocator>& alloc ){
	
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

					// allocate memory on gpu
					if ( alloc->allocate( dataRange, offset ) && alloc->map( dataP ) ){

						// copy data from draw command temp storage to gpu
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
			{
				descriptor.buffer = bufferInfoIt->buffer;
				descriptor.range  = bufferInfoIt->range;
				*dynamicOffsetsIt = bufferInfoIt->offset;

				bufferInfoIt++;
				dynamicOffsetsIt++;
			}
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

void DrawCommand::commitMeshAttributes( const std::unique_ptr<BufferAllocator>& alloc ){
	// check if current draw command has a mesh - if yes, upload mesh data to buffer memory.
	if ( mMsh ){
		auto &mesh = *mMsh;

		if ( !mesh.hasVertices() || !allocAndSetAttribute( "inPos", mesh.getVertices(), alloc ) ){
			ofLogError() << "Mesh has no vertices.";
			return;
		} else {
			mNumVertices = uint32_t(mesh.getVertices().size());
		}

		if ( mesh.hasColors() && mesh.usingColors()){
			allocAndSetAttribute( "inColor", mesh.getColors(), alloc );
		}
		if ( mesh.hasNormals() && mesh.usingNormals() ){
			allocAndSetAttribute( "inNormal", mesh.getNormals(), alloc );
		}
		if ( mesh.hasTexCoords() && mesh.usingTextures() ){
			allocAndSetAttribute( "inTexCoords", mesh.getTexCoords(), alloc );
		}

		if ( mesh.hasIndices() && mesh.usingIndices() ){
			const auto & indices = mesh.getIndices();
			const auto byteSize = sizeof( indices[0] ) * indices.size();

			void * dataP = nullptr;
			::vk::DeviceSize offset = 0;

			if ( alloc->allocate( byteSize, offset ) && alloc->map( dataP ) ){
				memcpy( dataP, indices.data(), byteSize );
				setIndices( alloc->getBuffer(), offset );
				mNumIndices = uint32_t(indices.size());
			}
		} else{
			mIndexBuffer = nullptr;
			mIndexOffsets = 0;
		}

	}
}

// ------------------------------------------------------------

void DrawCommand::setMesh(const shared_ptr<ofMesh> & msh_ ){
	mMsh = msh_;
}

// ------------------------------------------------------------

// upload vertex data to gpu memory
template<typename T>
inline bool DrawCommand::allocAndSetAttribute( const std::string & attrName_, const std::vector<T>& vec, const std::unique_ptr<BufferAllocator>& alloc ){
	void * dataP = nullptr;
	::vk::DeviceSize offset = 0;

	const auto byteSize = sizeof( vec[0] ) * vec.size();
	// allocate data on gpu
	if ( alloc->allocate( byteSize, offset ) && alloc->map( dataP ) ){
		alloc->map( dataP );
		memcpy( dataP, vec.data(), byteSize );
		setAttribute( attrName_, alloc->getBuffer(), offset );
		return true;
	}
	return false;
}

// ------------------------------------------------------------
