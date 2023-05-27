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
   return one;
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