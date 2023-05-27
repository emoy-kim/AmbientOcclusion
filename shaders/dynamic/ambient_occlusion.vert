#version 460

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 ModelViewProjectionMatrix;
uniform int UseBentNormal;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_bent_normal;
layout (location = 3) in float v_accessibility;

out vec3 position_in_ec;
out vec3 normal_in_ec;
out float accessibility;

void main()
{   
   vec4 e_position = ViewMatrix * WorldMatrix * vec4(v_position, 1.0f);
   vec3 n = bool(UseBentNormal) ? v_bent_normal : v_normal;
   // As ViewMatrix * WorldMatrix is rigid body transformation,
   // transpose( inverse( ViewMatrix * WorldMatrix ) ) is equal to ViewMatrix * WorldMatrix.
   // So it is possible to avoid the costly operation to calculate the tranformation for normals.
   vec4 e_normal = ViewMatrix * WorldMatrix * vec4(n, 0.0f);
   position_in_ec = e_position.xyz;
   normal_in_ec = normalize( e_normal.xyz );

   accessibility = v_accessibility;

   gl_Position = ModelViewProjectionMatrix * vec4(v_position, 1.0f);
}