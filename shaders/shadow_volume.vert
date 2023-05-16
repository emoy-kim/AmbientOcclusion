#version 460

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;

layout (location = 0) in vec3 v_position;

void main()
{
   gl_Position = ViewMatrix * WorldMatrix * vec4(v_position, 1.0f);
}