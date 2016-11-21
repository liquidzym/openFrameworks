#version 420 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout (set = 0, binding = 1) uniform sampler2D tex_0;

layout (location = 0) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

void main() 
{

  vec4 sampledColor = texture(tex_0, inTexCoord);
  outFragColor = sampledColor;

  // outFragColor = vec4(inTexCoord, 0.f, 1.f);

}