#pragma once
#include "vulkan/vulkan.hpp"
#include "vk/Shader.h"
#include "vk/Pipeline.h"
#include "vk/HelperTypes.h"
#include "vk/Texture.h"
#include "ofMesh.h"


namespace of{
namespace vk{

class DrawCommand;	   // ffdecl.
class RenderBatch;	   // ffdecl.
class BufferAllocator;	   // ffdecl.

// ----------------------------------------------------------------------

class DrawCommand
{
	friend class RenderBatch;
	

private:

	// a draw command has everything needed to draw an object
	GraphicsPipelineState mPipelineState;

private:      /* transient data */

	uint64_t mPipelineHash = 0;

	// Bindings data for descriptorSets, (vector index == set number) - retrieved from shader on setup
	std::vector<DescriptorSetData_t>   mDescriptorSetData;
	// Lookup table for uniform name-> desciptorSetData - retrieved from shader on setup
	std::map<std::string, UniformId_t> mUniformDictionary;

	// vector of buffers holding vertex attribute data
	std::vector<::vk::Buffer> mVertexBuffers;

	// offsets into buffer for vertex attribute data
	std::vector<::vk::DeviceSize> mVertexOffsets;

	// 1-or-0 element buffer of indices for this draw command
	::vk::Buffer mIndexBuffer = nullptr;

	// offsets into buffer for index data - this is optional
	::vk::DeviceSize mIndexOffsets = 0;

	uint32_t mNumIndices = 0;
	uint32_t mNumVertices = 0;

	std::shared_ptr<ofMesh> mMsh; /* optional */

	template <typename T>
	bool allocAndSetAttribute( const std::string& attrName_, const std::vector<T> & vec, const std::unique_ptr<BufferAllocator>& alloc );

public:

	void setup(const GraphicsPipelineState& pipelineState);

	const GraphicsPipelineState&         getPipelineState() const;
	
	const DescriptorSetData_t&           getDescriptorSetData( size_t setId_ ) const;

	const std::vector<::vk::DeviceSize>& getVertexOffsets();
	const std::vector<::vk::Buffer>&     getVertexBuffers();
	
	const ::vk::DeviceSize&              getIndexOffsets();
	const ::vk::Buffer&                  getIndexBuffer();
	
	uint32_t                             getNumIndices();
	of::vk::DrawCommand &                setNumVertices( uint32_t numVertices );
	
	uint32_t                             getNumVertices();
	of::vk::DrawCommand &                setNumIndices( uint32_t numIndices );

	// set data for upload to ubo - data is stored locally 
	// until draw command is submitted
	void commitUniforms( const std::unique_ptr<BufferAllocator>& alloc_ );

	void commitMeshAttributes( const std::unique_ptr<BufferAllocator>& alloc_ );

	void setMesh( const shared_ptr<ofMesh>& msh_ );

	of::vk::DrawCommand & setAttribute( const std::string& name_, ::vk::Buffer buffer_, ::vk::DeviceSize offset_ );
	of::vk::DrawCommand & setAttribute( const size_t attribLocation_, ::vk::Buffer buffer, ::vk::DeviceSize offset );
	of::vk::DrawCommand & setAttribute( const size_t attribLocation_, const of::vk::BufferRegion & bufferRegion_ );
	
	of::vk::DrawCommand & setIndices( ::vk::Buffer buffer, ::vk::DeviceSize offset );
	of::vk::DrawCommand & setIndices( const of::vk::BufferRegion& bufferRegion_ );

	// store uniform values to staging cpu memory
	template <class T>
	of::vk::DrawCommand & setUniform( const std::string& uniformName, const T& uniformValue_ );

	of::vk::DrawCommand & setTexture( const std::string& name, const of::vk::Texture& tex_ );
	of::vk::DrawCommand & setStorageBuffer( const std::string& name, const of::vk::BufferRegion& buf_ );

};

// ------------------------------------------------------------

template<class T>
inline DrawCommand& DrawCommand::setUniform( const std::string & uniformName, const T & uniformValue_ ){

	auto uniformInfoIt = mUniformDictionary.find( uniformName );
	
	if ( uniformInfoIt == mUniformDictionary.end() ){
		ofLogWarning() << "Could not set Uniform '" << uniformName << "': Uniform name not found in shader";
		return *this;
	}

	// --------| invariant: uniform found

	const auto & uniformInfo = uniformInfoIt->second;
	
	if ( uniformInfo.dataRange < sizeof( T ) ){
		ofLogWarning() << "Could not set uniform '" << uniformName << "': Uniform data size does not match: "
			<< " Expected: " << uniformInfo.dataRange << ", received: " << sizeof( T ) << ".";
		return *this;
	}

	// --------| invariant: size match, we can copy data into our vector.

	auto & dataVec = mDescriptorSetData[uniformInfo.setIndex].dynamicUboData[uniformInfo.auxDataIndex];

	if ( uniformInfo.dataOffset + uniformInfo.dataRange <= dataVec.size() ){
		memcpy( dataVec.data() + uniformInfo.dataOffset, &uniformValue_, uniformInfo.dataRange );
	} else{
		ofLogError() << "Not enough space in local uniform storage. Has this drawCommand been properly initialised?";
	}

	return *this;
}

// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setTexture( const std::string & uniformName, const of::vk::Texture& tex_ ){
	
	auto uniformInfoIt = mUniformDictionary.find( uniformName );

	if ( uniformInfoIt == mUniformDictionary.end() ){
		ofLogWarning() << "Could not set Texture '" << uniformName << "': Uniform name not found in shader";
		return *this;
	}

	// --------| invariant: uniform found
	
	const auto & uniformInfo = uniformInfoIt->second;

	auto & imageAttachment = mDescriptorSetData[uniformInfo.setIndex].imageAttachment[uniformInfo.auxDataIndex];

	imageAttachment.sampler     = tex_.getSampler();
	imageAttachment.imageView   = tex_.getImageView();
	imageAttachment.imageLayout = tex_.getImageLayout();
	
	return *this;
}

// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setStorageBuffer( const std::string & uniformName, const of::vk::BufferRegion& buf_ ){

	auto uniformInfoIt = mUniformDictionary.find( uniformName );

	if ( uniformInfoIt == mUniformDictionary.end() ){
		ofLogWarning() << "Could not set Storage Buffer '" << uniformName << "': Uniform name not found in shader";
		return *this;
	}

	// --------| invariant: uniform found

	const auto & uniformInfo = uniformInfoIt->second;

	auto & bufferAttachment = mDescriptorSetData[uniformInfo.setIndex].bufferAttachment[uniformInfo.auxDataIndex];

	bufferAttachment = buf_;

	return *this;
}

} // namespace 
} // end namespace of

// ------------------------------------------------------------
// Inline getters and setters

inline const of::vk::GraphicsPipelineState & of::vk::DrawCommand::getPipelineState() const{
	return mPipelineState;
}

inline const of::vk::DescriptorSetData_t & of::vk::DrawCommand::getDescriptorSetData( size_t setId_ ) const{
	return mDescriptorSetData[setId_];
}

inline const std::vector<::vk::DeviceSize>& of::vk::DrawCommand::getVertexOffsets(){
	return mVertexOffsets;
}

inline const::vk::DeviceSize & of::vk::DrawCommand::getIndexOffsets(){
	return mIndexOffsets;
}

inline const std::vector<::vk::Buffer>& of::vk::DrawCommand::getVertexBuffers(){
	return mVertexBuffers;
}

inline const::vk::Buffer & of::vk::DrawCommand::getIndexBuffer(){
	return mIndexBuffer;
}

inline uint32_t of::vk::DrawCommand::getNumIndices(){
	return mNumIndices;
}

inline uint32_t of::vk::DrawCommand::getNumVertices(){
	return mNumVertices;
}

inline of::vk::DrawCommand & of::vk::DrawCommand::setNumVertices( uint32_t numVertices ){
	mNumVertices = numVertices;
	return *this;
}

inline of::vk::DrawCommand & of::vk::DrawCommand::setNumIndices( uint32_t numIndices ){
	mNumIndices = numIndices;
	return *this;
}

// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setAttribute( const size_t attribLocation_, const of::vk::BufferRegion & bufferRegion_ ){
	return setAttribute( attribLocation_, bufferRegion_.buffer, bufferRegion_.offset );
}

// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setAttribute( const std::string& name_, ::vk::Buffer buffer_, ::vk::DeviceSize offset_ ){
	size_t index = 0;
	if ( mPipelineState.getShader()->getAttributeIndex( name_, index ) ){
		setAttribute( index, buffer_, offset_ );
		return *this;
	}

	// --------| invariant: name was not resolved successfully.

	ofLogWarning() 
		<< "Attribute '" << name_ << "' could not be found in shader: " 
		<< mPipelineState.getShader()->mSettings.sources.at( ::vk::ShaderStageFlagBits::eVertex );
	return *this;
}

// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setAttribute( const size_t attribLocation_, ::vk::Buffer buffer_, ::vk::DeviceSize offset_ ){

	if ( attribLocation_ >= mVertexBuffers.size() ){
		ofLogError() << "Attribute location not available: " << attribLocation_;
		return *this;
	}

	// ---------| invariant: attribLocation is valid

	mVertexBuffers[attribLocation_] = buffer_;
	mVertexOffsets[attribLocation_] = offset_;
	
	return *this;
}


// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setIndices( const of::vk::BufferRegion& bufferRegion_ ){
	return setIndices( bufferRegion_.buffer, bufferRegion_.offset );
}



// ------------------------------------------------------------

inline of::vk::DrawCommand & of::vk::DrawCommand::setIndices( ::vk::Buffer buffer_, ::vk::DeviceSize offset_ ){
	mIndexBuffer = buffer_;
	mIndexOffsets = offset_;
	return *this;
}

// ------------------------------------------------------------
