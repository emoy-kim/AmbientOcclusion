#version 460

#define MAX_LIGHTS 32

struct LightInfo
{
   int LightSwitch;
   vec4 Position;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   vec3 SpotlightDirection;
   float SpotlightExponent;
   float SpotlightCutoffAngle;
   vec3 AttenuationFactors;
};
uniform LightInfo Lights[MAX_LIGHTS];

struct MateralInfo {
   vec4 EmissionColor;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   float SpecularExponent;
};
uniform MateralInfo Material;

struct Disk
{
   int ParentIndex;
   int NextIndex;
   int LeftChildIndex;
   int RightChildIndex;
   float AreaOverPi;
   float Accessibility;
   vec3 Centroid;
   vec3 Normal;
   vec3 BentNormal;
};

layout (binding = 0, std430) buffer InDisks { Disk in_disks[]; };
layout (binding = 1, std430) buffer Indices { int indices[]; };
layout (binding = 2, std430) buffer Vertices { vec3 vertices[]; };

uniform int Robust;
uniform int UseBentNormal;
uniform int RootIndex;
uniform float ProximityTolerance;
uniform float DistanceAttenuation;
uniform float TriangleAttenuation;

uniform int UseLight;
uniform int LightIndex;
uniform int LightNum;
uniform vec4 GlobalAmbient;

uniform mat4 WorldMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

in vec3 receiver_position;
in vec3 receiver_normal;
in vec3 position_in_ec;
in vec3 normal_in_ec;

layout (location = 0) out vec4 final_color;

const float zero = 0.0f;
const float one = 1.0f;
const float epsilon = 1e-16f;

bool IsPointLight(in vec4 light_position)
{
   return light_position.w != zero;
}

float getAttenuation(in vec3 light_vector, in int light_index)
{
   vec3 distance_scale;
   distance_scale.x = one;
   distance_scale.z = dot( light_vector, light_vector );
   distance_scale.y = sqrt( distance_scale.z );
   return one / dot( distance_scale, Lights[light_index].AttenuationFactors );
}

float getSpotlightFactor(in vec3 normalized_light_vector, in int light_index)
{
   if (Lights[light_index].SpotlightCutoffAngle >= 180.0f) return one;

   vec4 direction_in_ec = 
      transpose( inverse( ViewMatrix * WorldMatrix ) ) * 
      vec4(Lights[light_index].SpotlightDirection, one);
   vec3 normalized_direction = normalize( direction_in_ec.xyz );
   float cutoff_angle = clamp( Lights[light_index].SpotlightCutoffAngle, zero, 90.0f );
   float factor = dot( -normalized_light_vector, normalized_direction );
   return factor >= cos( radians( cutoff_angle ) ) ? pow( factor, Lights[light_index].SpotlightExponent ) : zero;
}

vec4 calculateLightingEquation(in vec3 normal)
{
   vec4 color = Material.EmissionColor + GlobalAmbient * Material.AmbientColor;

   if (Lights[LightIndex].LightSwitch == 0) return color;
      
   vec4 light_position_in_ec = ViewMatrix * Lights[LightIndex].Position;
      
   float final_effect_factor = one;
   vec3 light_vector = light_position_in_ec.xyz - position_in_ec;
   if (IsPointLight( light_position_in_ec )) {
      float attenuation = getAttenuation( light_vector, LightIndex );

      light_vector = normalize( light_vector );
      float spotlight_factor = getSpotlightFactor( light_vector, LightIndex );
      final_effect_factor = attenuation * spotlight_factor;
   }
   else light_vector = normalize( light_position_in_ec.xyz );
   
   if (final_effect_factor <= zero) return color;

   vec4 local_color = Lights[LightIndex].AmbientColor * Material.AmbientColor;

   float diffuse_intensity = max( dot( normal, light_vector ), zero );
   local_color += diffuse_intensity * Lights[LightIndex].DiffuseColor * Material.DiffuseColor;

   vec3 halfway_vector = normalize( light_vector - normalize( position_in_ec ) );
   float specular_intensity = max( dot( normal, halfway_vector ), zero );
   local_color += 
      pow( specular_intensity, Material.SpecularExponent ) * 
      Lights[LightIndex].SpecularColor * Material.SpecularColor;

   color += local_color;
   return color;
}

float getShadowApproximation(
   in vec3 v,
   in float squared_distance,
   in vec3 receiver_normal,
   in vec3 emitter_normal,
   in float emitter_area
)
{
   return
      emitter_area / ( emitter_area + squared_distance ) *
      clamp( dot( emitter_normal, -v ), zero, one ) *
      clamp( dot( receiver_normal, v ), zero, one );
}

vec3 getPointOnPlane(in vec3 p0, in vec3 p1, in float signed_distance0, in float signed_distance1)
{
   //              p0
   //             / .
   //            /  d0 (= signed_distance0)
   //           /   .
   //  --------x------ plane         x = p0 + (d0 / (d0 + d1) * (p1 - p0)
   //         /     .
   //        /      d1 (= -signed_distance1)
   //       /       .
   //     p1
   return p0 + (signed_distance0 / (signed_distance0 - signed_distance1)) * (p1 - p0);
}

void getVisiblePoints(
   out vec3 q0,
   out vec3 q1,
   out vec3 q2,
   out vec3 q3,
   in vec3 receiver_position,
   in vec3 receiver_normal,
   in vec3 emitter_v0,
   in vec3 emitter_v1,
   in vec3 emitter_v2
)
{
   // plane equation ax + by + cz + d = 0 can be represented as n dot (p' - p) = 0.
   // (p: point on the plane, n: normal from p, p': point on the plane, not p)
   float d = dot( receiver_normal, receiver_position );
   float signed_distances[3] = {
      dot( receiver_normal, emitter_v0 ) - d,
      dot( receiver_normal, emitter_v1 ) - d,
      dot( receiver_normal, emitter_v2 ) - d,
   };
   if (abs( signed_distances[0] ) <= 1e-6f) signed_distances[0] = zero;
   if (abs( signed_distances[1] ) <= 1e-6f) signed_distances[0] = zero;
   if (abs( signed_distances[2] ) <= 1e-6f) signed_distances[0] = zero;

   if (signed_distances[0] > zero) {
      if (signed_distances[1] > zero) {
         if (signed_distances[2] < zero) {
            // ++-
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = getPointOnPlane( emitter_v1, emitter_v2, signed_distances[1], signed_distances[2] );
            q3 = getPointOnPlane( emitter_v0, emitter_v2, signed_distances[0], signed_distances[2] );
         }
         else {
            // +++ and ++0
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = emitter_v2;
         }
      }
      else if (signed_distances[1] < zero) {
         if (signed_distances[2] > zero) {
            // +-+
            q0 = emitter_v0;
            q1 = getPointOnPlane( emitter_v0, emitter_v1, signed_distances[0], signed_distances[1] );
            q2 = getPointOnPlane( emitter_v2, emitter_v1, signed_distances[2], signed_distances[1] );
            q3 = emitter_v2;
         }
         else if (signed_distances[2] < zero) {
            // +--
            q0 = emitter_v0;
            q1 = getPointOnPlane( emitter_v0, emitter_v1, signed_distances[0], signed_distances[1] );
            q2 = getPointOnPlane( emitter_v0, emitter_v2, signed_distances[0], signed_distances[2] );
            q3 = q2;
         }
         else {
            // +-0
            q0 = emitter_v0;
            q1 = getPointOnPlane( emitter_v0, emitter_v1, signed_distances[0], signed_distances[1] );
            q2 = emitter_v2;
            q3 = q2;
         }
      }
      else {
         if (signed_distances[2] < zero) {
            // +0-
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = getPointOnPlane( emitter_v0, emitter_v2, signed_distances[0], signed_distances[2] );
            q3 = q2;
         }
         else {
            // +0+ and +00
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = q2;
         }
      }
   }
   else if (signed_distances[0] < zero) {
      if (signed_distances[1] > zero) {
         if (signed_distances[2] > zero) {
            // -++
            q0 = getPointOnPlane( emitter_v1, emitter_v0, signed_distances[1], signed_distances[0] );
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = getPointOnPlane( emitter_v2, emitter_v0, signed_distances[2], signed_distances[0] );
         }
         else if (signed_distances[2] < zero) {
            // -+-
            q0 = getPointOnPlane( emitter_v1, emitter_v0, signed_distances[1], signed_distances[0] );
            q1 = emitter_v1;
            q2 = getPointOnPlane( emitter_v1, emitter_v2, signed_distances[1], signed_distances[2] );
            q3 = q2;
         }
         else {
            // -+0
            q0 = getPointOnPlane( emitter_v1, emitter_v0, signed_distances[1], signed_distances[0] );
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = q2;
         }
      }
      else if (signed_distances[1] < zero) {
         if (signed_distances[2] > zero) {
            // --+
            q0 = getPointOnPlane( emitter_v2, emitter_v0, signed_distances[2], signed_distances[0] );
            q1 = getPointOnPlane( emitter_v2, emitter_v1, signed_distances[2], signed_distances[1] );
            q2 = emitter_v2;
            q3 = q2;
         }
         else {
            // --- and --0
            q0 = q1 = q2 = q3 = receiver_position;
         }
      }
      else {
         if (signed_distances[2] > zero) {
            // -0+
            q0 = getPointOnPlane( emitter_v2, emitter_v0, signed_distances[2], signed_distances[0] );
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = q2;
         }
         else {
            // -0- and -00
            q0 = q1 = q2 = q3 = receiver_position;
         }
      }
   }
   else {
      if (signed_distances[1] > zero) {
         if (signed_distances[2] < zero) {
            // 0+-
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = getPointOnPlane( emitter_v1, emitter_v2, signed_distances[1], signed_distances[2] );
            q3 = q2;
         }
         else {
            // 0+0 and 0++
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = q2;
         }
      }
      else if (signed_distances[1] < zero) {
         if (signed_distances[2] > zero) {
            // 0-+
            q0 = emitter_v0;
            q1 = getPointOnPlane( emitter_v2, emitter_v1, signed_distances[2], signed_distances[1] );
            q2 = emitter_v2;
            q3 = q2;
         }
         else {
            // 0-- and 0-0
            q0 = q1 = q2 = q3 = receiver_position;
         }
      }
      else {
         if (signed_distances[2] > zero) {
            // 00+
            q0 = emitter_v0;
            q1 = emitter_v1;
            q2 = emitter_v2;
            q3 = q2;
         }
         else {
            // 00- and 000
            q0 = q1 = q2 = q3 = receiver_position;
         }
      }
   }
}

float calculateFormFactor(
   in vec3 q0,
   in vec3 q1,
   in vec3 q2,
   in vec3 q3,
   in vec3 receiver_position,
   in vec3 receiver_normal
)
{
   vec3 r0 = normalize( q0 - receiver_position );
   vec3 r1 = normalize( q1 - receiver_position );
   vec3 r2 = normalize( q2 - receiver_position );
   vec3 r3 = normalize( q3 - receiver_position );
   vec3 g0 = normalize( cross( r1, r0 ) );
   vec3 g1 = normalize( cross( r2, r1 ) );
   vec3 g2 = normalize( cross( r3, r2 ) );
   vec3 g3 = normalize( cross( r0, r3 ) );
   float a = acos( clamp( dot( r0, r1 ), -one, one ) );
}

float getFormFactor(in vec3 receiver_position, in vec3 receiver_normal, in int index)
{
   int face_index = 3 * index;
   int i0 = indices[face_index];
   int i1 = indices[face_index + 1];
   int i2 = indices[face_index + 2];
   vec3 v0 = vertices[i0];
   vec3 v1 = vertices[i1];
   vec3 v2 = vertices[i2];
   vec3 q0, q1, q2, q3;
   getVisiblePoints( q0, q1, q2, q3, receiver_position, receiver_normal, v0, v1, v2 );
   return calculateFormFactor( q0, q1, q2, q3, receiver_position, receiver_normal );
}

float calculateOcclusion(out vec3 bent_normal)
{
   bent_normal = receiver_normal;
   int emitter_index = RootIndex;
   float total_shadow = zero;
   while (emitter_index >= 0) {
      vec3 emitter_position = in_disks[emitter_index].Centroid;
      vec3 emitter_normal = in_disks[emitter_index].Normal;
      float emitter_area = in_disks[emitter_index].AreaOverPi;
      vec3 v = emitter_position - receiver_position;
      float squared_distance = dot( v, v ) + epsilon;
      if (in_disks[emitter_index].LeftChildIndex >= 0 && squared_distance < emitter_area * ProximityTolerance) {
         emitter_index = in_disks[emitter_index].LeftChildIndex;
         continue;
      }
      v *= inversesqrt( squared_distance );
      float shadow = getShadowApproximation( v, squared_distance, receiver_normal, emitter_normal, emitter_area );
      shadow *= in_disks[emitter_index].Accessibility;
      shadow /= (one + DistanceAttenuation * sqrt( squared_distance ));

      total_shadow += shadow;
      bent_normal -= shadow * v;
      emitter_index = in_disks[emitter_index].NextIndex;
   }
   bent_normal = normalize( bent_normal );
   return clamp( one - total_shadow, zero, one );
}

float calculateRobustOcclusion(out vec3 bent_normal)
{
   bent_normal = receiver_normal;
   int parent_next = 0;
   int emitter_index = RootIndex;
   float total_shadow = zero;
   float parent_area = one;
   float parent_shadow = zero;
   float parent_weight = zero;
   const float zone_radius = 0.1f;
   while (emitter_index >= 0) {
      vec3 emitter_position = in_disks[emitter_index].Centroid;
      vec3 emitter_normal = in_disks[emitter_index].Normal;
      float emitter_area = in_disks[emitter_index].AreaOverPi;
      vec3 v = emitter_position - receiver_position;
      float squared_distance = dot( v, v ) + epsilon;
      v *= inversesqrt( squared_distance );
      float close = ProximityTolerance * emitter_area;
      if (in_disks[emitter_index].LeftChildIndex >= 0 && squared_distance < close * (one + zone_radius)) {
         parent_next = in_disks[emitter_index].NextIndex;
         emitter_index = in_disks[emitter_index].LeftChildIndex;
         float shadow = getShadowApproximation( v, squared_distance, receiver_normal, emitter_normal, emitter_area );
         shadow *= in_disks[emitter_index].Accessibility;
         shadow /= (one + DistanceAttenuation * sqrt( squared_distance ));
         parent_shadow = shadow;
         parent_area = emitter_area;
         parent_weight =
            clamp( (squared_distance - (one - zone_radius) * close) / (2.0f * zone_radius * close), zero, one );
      }
      else {
         float shadow = zero;
         if (in_disks[emitter_index].LeftChildIndex < 0) {
            if (dot( emitter_normal, -v ) >= zero) {
               shadow = getFormFactor( receiver_position, receiver_normal, 0 );

               // with low TriangleAttenuation, small features like creases and cracks are emphasized.
               // with high TriangleAttenuation, the influence of far away (probably invisible) triangles is lessened.
               shadow *= pow( in_disks[emitter_index].Accessibility, TriangleAttenuation );
            }
         }
         else {
            shadow = getShadowApproximation( v, squared_distance, receiver_normal, emitter_normal, emitter_area );
            shadow *= in_disks[emitter_index].Accessibility;
            shadow /= (one + DistanceAttenuation * sqrt( squared_distance ));
         }

         total_shadow += shadow;
         bent_normal -= shadow * v;
         emitter_index = in_disks[emitter_index].NextIndex;
      }
   }
   bent_normal = normalize( bent_normal );
   return clamp( one - total_shadow, zero, one );
}

void main()
{
   final_color = vec4(one);

   vec3 bent_normal;
   float accessibility = bool(Robust) ? calculateRobustOcclusion( bent_normal ) : calculateOcclusion( bent_normal );
   if (bool(UseLight)) {
      vec3 normal = bool(UseBentNormal) ? vec3(ViewMatrix * WorldMatrix * vec4(bent_normal, zero)) : normal_in_ec;
      final_color *= calculateLightingEquation( normal );
   }
   else final_color *= Material.DiffuseColor;

   final_color *= accessibility;
}