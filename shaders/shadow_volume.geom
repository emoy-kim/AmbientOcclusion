#version 460

uniform mat4 ProjectionMatrix;
uniform vec4 LightPosition;
uniform int Robust;
uniform int IsZFailAlgorithm;

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 18) out;

const float zero = 0.0f;
const float one = 1.0f;
const float epsilon = 1e-1f;

void main()
{
   // six vertices of triangles_adjacency
   // 5 . . . . . . 4 . . . . . . . . 3
   //   .          / \              .
   //    .        /   \           .
   //     .     e1    e2        .
   //      .    /       \     .
   //       .  /         \  .
   //        0 --- e0 --- 2
   //         .         .
   //          .      .
   //            .  .
   //             1
   vec3 e0 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
   vec3 e1 = gl_in[4].gl_Position.xyz - gl_in[0].gl_Position.xyz;
   vec3 e2 = gl_in[4].gl_Position.xyz - gl_in[2].gl_Position.xyz;
   vec3 normals[3] = { cross( e0, e1 ), cross( e2, -e0 ), cross( -e1, -e2 ) };
   vec4 vertices[3] = { gl_in[0].gl_Position, gl_in[2].gl_Position, gl_in[4].gl_Position };
   vec3 light_directions[3] = {
      LightPosition.xyz - LightPosition.w * vertices[0].xyz,
      LightPosition.xyz - LightPosition.w * vertices[1].xyz,
      LightPosition.xyz - LightPosition.w * vertices[2].xyz
   };
   bool faces_light = true;
   if (dot( normals[0], light_directions[0] ) < epsilon &&
       dot( normals[1], light_directions[1] ) < epsilon &&
       dot( normals[2], light_directions[2] ) < epsilon) {
      if (!bool(Robust)) return;

      faces_light = false;
   }

   if (bool(IsZFailAlgorithm)) {
      light_directions[0] = -normalize( light_directions[0] );
      light_directions[1] = -normalize( light_directions[1] );
      light_directions[2] = -normalize( light_directions[2] );
      if (faces_light) {
         gl_Position = ProjectionMatrix * vec4(vertices[0].xyz + light_directions[0] * epsilon, vertices[0].w);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(vertices[1].xyz + light_directions[1] * epsilon, vertices[1].w);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(vertices[2].xyz + light_directions[2] * epsilon, vertices[2].w);
         EmitVertex();
         EndPrimitive();

         gl_Position = ProjectionMatrix * vec4(light_directions[0], zero);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(light_directions[2], zero);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(light_directions[1], zero);
         EmitVertex();
         EndPrimitive();
      }
      else {
         gl_Position = ProjectionMatrix * vec4(vertices[0].xyz + light_directions[0] * epsilon, vertices[0].w);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(vertices[2].xyz + light_directions[2] * epsilon, vertices[2].w);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(vertices[1].xyz + light_directions[1] * epsilon, vertices[1].w);
         EmitVertex();
         EndPrimitive();

         gl_Position = ProjectionMatrix * vec4(light_directions[0], zero);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(light_directions[1], zero);
         EmitVertex();
         gl_Position = ProjectionMatrix * vec4(light_directions[2], zero);
         EmitVertex();
         EndPrimitive();
      }
   }

   for (int i = 0; i < 3; ++i) {
      int v0 = i * 2;
      int v1 = (v0 + 2) % 6;
      int adjacent_v0 = v0 + 1;
      normals[0] = cross(
         gl_in[adjacent_v0].gl_Position.xyz - gl_in[v0].gl_Position.xyz,
         gl_in[v1].gl_Position.xyz - gl_in[v0].gl_Position.xyz
      );
      normals[1] = cross(
         gl_in[v1].gl_Position.xyz - gl_in[adjacent_v0].gl_Position.xyz,
         gl_in[v0].gl_Position.xyz - gl_in[adjacent_v0].gl_Position.xyz
      );
      normals[2] = cross(
         gl_in[v0].gl_Position.xyz - gl_in[v1].gl_Position.xyz,
         gl_in[adjacent_v0].gl_Position.xyz - gl_in[v1].gl_Position.xyz
      );
      light_directions[0] = LightPosition.xyz - LightPosition.w * gl_in[v0].gl_Position.xyz;
      light_directions[1] = LightPosition.xyz - LightPosition.w * gl_in[adjacent_v0].gl_Position.xyz;
      light_directions[2] = LightPosition.xyz - LightPosition.w * gl_in[v1].gl_Position.xyz;
      if (gl_in[adjacent_v0].gl_Position.w < 1e-3f ||
          (faces_light == (dot( normals[0], light_directions[0] ) < epsilon &&
                           dot( normals[1], light_directions[1] ) < epsilon &&
                           dot( normals[2], light_directions[2] ) < epsilon))) {
         light_directions[0] = -normalize( light_directions[0] );
         light_directions[2] = -normalize( light_directions[2] );
         if (faces_light) {
            gl_Position = ProjectionMatrix * vec4(gl_in[v0].gl_Position.xyz + light_directions[0] * epsilon, gl_in[v0].gl_Position.w);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(light_directions[0], zero);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(gl_in[v1].gl_Position.xyz + light_directions[2] * epsilon, gl_in[v1].gl_Position.w);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(light_directions[2], zero);
            EmitVertex();
            EndPrimitive();
         }
         else {
            gl_Position = ProjectionMatrix * vec4(gl_in[v1].gl_Position.xyz + light_directions[2] * epsilon, gl_in[v1].gl_Position.w);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(light_directions[2], zero);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(gl_in[v0].gl_Position.xyz + light_directions[0] * epsilon, gl_in[v0].gl_Position.w);
            EmitVertex();
            gl_Position = ProjectionMatrix * vec4(light_directions[0], zero);
            EmitVertex();
            EndPrimitive();
         }
      }
   }
}