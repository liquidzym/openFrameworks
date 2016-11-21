#pragma once
#include "vulkan\vulkan.hpp"

struct RenderPassData
{
	std::vector<::vk::AttachmentDescription> attachments;
	::vk::AttachmentReference depthStencilAttachment;

	struct SubpassDescription
	{
		// subpass description, indexed by subpass 
		std::vector<::vk::AttachmentReference> colorReferences;
		std::vector<::vk::AttachmentReference> depthReferences;	 // only first used, if any.
	};

	std::vector<SubpassDescription> subpasses;

	std::vector<::vk::SubpassDependency> subpassDependencies;

};

// ----------------------------------------------------------------------

static ::vk::RenderPass createRenderPass( const ::vk::Device device_, const RenderPassData& rpd_ ){
	::vk::RenderPassCreateInfo renderPassCreateInfo;

	std::vector<::vk::SubpassDescription> subpassDescriptions;
	subpassDescriptions.reserve( rpd_.subpasses.size() );

	for ( const auto & subpass : rpd_.subpasses ){
		::vk::SubpassDescription subpassDescription;
		subpassDescription
			.setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
			.setColorAttachmentCount( subpass.colorReferences.size() )
			.setPColorAttachments( subpass.colorReferences.data() )
			;
		if ( !subpass.depthReferences.empty() ){
			subpassDescription
				.setPDepthStencilAttachment( subpass.depthReferences.data() );
		}
		subpassDescriptions.emplace_back( subpassDescription );
	}

	renderPassCreateInfo
		.setAttachmentCount( rpd_.attachments.size() )
		.setPAttachments( rpd_.attachments.data() )
		.setSubpassCount( subpassDescriptions.size() )
		.setPSubpasses( subpassDescriptions.data() )
		.setDependencyCount( rpd_.subpassDependencies.size() )
		.setPDependencies( rpd_.subpassDependencies.data() );

	return device_.createRenderPass( renderPassCreateInfo );
};