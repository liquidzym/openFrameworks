#version 420 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// uniforms (resources)
layout (set = 0, binding = 0) uniform DefaultMatrices 
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
}; 

// inputs (vertex attributes)
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inTexCoord;

// outputs 
layout (location = 0) out vec2 outTexCoord;

// we override the built-in fixed function outputs
// to have more control over the SPIR-V code created.
out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	outTexCoord = vec2(inTexCoord.x,1.0 -inTexCoord.y);
	gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(inPos.xyz, 1.0);
}
