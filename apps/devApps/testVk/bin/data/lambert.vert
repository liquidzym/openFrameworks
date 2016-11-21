#version 420 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// uniforms (resources)
layout (set = 0, binding = 0) uniform DefaultMatrices 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
}; // note: if you don't specify a variable name for the block its elements will live in the global namespace.

// inputs (vertex attributes)
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inNormal;

// outputs 
layout (location = 0) flat out vec3 outNormal;
layout (location = 1) out vec4 outPosition;

// we override the built-in fixed function outputs
// to have more control over the SPIR-V code created.
out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	outPosition = viewMatrix * modelMatrix * vec4(inPos, 1);
	outNormal   = (inverse(transpose( viewMatrix * modelMatrix)) * vec4(inNormal, 0.0)).xyz;
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPos.xyz, 1.0);
}
