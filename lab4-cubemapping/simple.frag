#version 420
#define M_PI 3.1415926535897932384626433832795

// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
uniform vec3 material_color;
uniform float material_reflectivity;
uniform float material_metalness;
uniform float material_fresnel;
uniform float material_shininess;
uniform float material_emission;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0f, 1.0f, 1.0f);
uniform float point_light_intensity_multiplier = 50.0f;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define PI 3.14159265359

///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpaceNormal; 
in vec3 viewSpacePosition; 

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse; 
uniform vec3 viewSpaceLightPosition;

///////////////////////////////////////////////////////////////////////////////
// Output color
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) out vec4 fragmentColor;

/*
vec3 calculateDirectIllumination(vec3 w_o, vec3 n)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.2 - Calculate the radiance Li from the light, and the direction
	//            to the light. If the light is backfacing the triangle, 
	//            return vec3(0); 
	///////////////////////////////////////////////////////////////////////////
	vec3 w_i = normalize(viewSpaceLightPosition - viewSpacePosition);
	vec3 w_distance = viewSpaceLightPosition - viewSpacePosition;
	float dSquare = dot(w_distance, w_distance);
	vec3 l_i = point_light_intensity_multiplier * point_light_color * 1/dSquare;

	if(dot(n, w_i) <= 0.0f){
		 return vec3(0.0f);
	}

	///////////////////////////////////////////////////////////////////////////
	// Task 1.3 - Calculate the diffuse term and return that as the result
	///////////////////////////////////////////////////////////////////////////
	vec3 diffuse_term = material_color * 1.0f/M_PI * length(dot(n, w_i)) * l_i;
	//return diffuse_term;

	///////////////////////////////////////////////////////////////////////////
	// Task 2 - Calculate the Torrance Sparrow BRDF and return the light 
	//          reflected from that instead
	///////////////////////////////////////////////////////////////////////////
	vec3 w_h = normalize(w_i + w_o);
	float s = material_shininess;
	float d_w_h = (s+2)/(2*M_PI) * pow(dot(n, w_h), s);

	float r_0 = material_fresnel;
	float f_w_i = r_0 + ((1 - r_0) * (pow((1 - dot(w_h, w_i)), 5)));

	float g_w_i_w_o = min(1, min(2*(dot(n, w_h)*dot(n, w_o))/dot(w_o, w_h), 2*(dot(n, w_h)*dot(n, w_i))/dot(w_o, w_h)));

	float brdf = (f_w_i * d_w_h * g_w_i_w_o)/(4*dot(n, w_o)*dot(n, w_i));

	//return brdf * dot(n, w_i) * l_i;

	///////////////////////////////////////////////////////////////////////////
	// Task 3 - Make your shader respect the parameters of our material model.
	///////////////////////////////////////////////////////////////////////////
	//return vec3(material_color);

	float m = material_metalness;
	float r = material_reflectivity;

	vec3 dielectric_term = brdf * dot(n, w_i) * l_i + (1 - f_w_i) * diffuse_term;
	vec3 metal_term = brdf * material_color * dot(n, w_i) * l_i;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;
	
	return r * microfacet_term + (1 - r) * diffuse_term;

}*/

vec3 calculateDirectIllumination(vec3 w_o, vec3 n)
{

	vec3 w_i = normalize(viewSpaceLightPosition - viewSpacePosition);
	vec3 w_distance = viewSpaceLightPosition - viewSpacePosition;
	float dSquare = dot(w_distance, w_distance);
	vec3 l_i = point_light_intensity_multiplier * point_light_color * 1 / dSquare;

	if(dot(n, w_i) <= 0.0f){
		 return vec3(0.0f);
	}

	vec3 diffuse_term = material_color * 1.0f / M_PI * abs(dot(n, w_i)) * l_i;

	vec3 w_h = normalize(w_i + w_o);
	float s = material_shininess;
	float d_w_h = ((s+2)/(2*M_PI)) * pow(dot(n, w_h), s);

	float r_0 = material_fresnel;
	float f_w_i = r_0 + (1 - r_0) * pow((1 - dot(w_h, w_i)), 5);

	float g_w_i_w_o = min(1, min(2*(dot(n, w_h)*dot(n, w_o))/dot(w_o, w_h), 2*(dot(n, w_h)*dot(n, w_i))/dot(w_o, w_h)));

	float brdf = (f_w_i * d_w_h * g_w_i_w_o)/(4*dot(n, w_o)*dot(n, w_i));

	
	float m = material_metalness;
	float r = material_reflectivity;

	vec3 dielectric_term = brdf * dot(n, w_i) * l_i + (1 - f_w_i) * diffuse_term;
	vec3 metal_term = brdf * material_color * dot(n, w_i) * l_i;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;
	
	return r * microfacet_term + (1 - r) * diffuse_term;
}

vec3 calculateIndirectIllumination(vec3 w_o, vec3 n)
{
	///////////////////////////////////////////////////////////////////////////
	// Task 5 - Lookup the irradiance from the irradiance map and calculate
	//          the diffuse reflection
	///////////////////////////////////////////////////////////////////////////
	
	vec3 n_ws = mat3(viewInverse) * n;

	float theta = acos(max(-1.0f, min(1.0f, n_ws.y)));
	float phi = atan(n_ws.z, n_ws.x);
	if (phi < 0.0f) phi = phi + 2.0f * PI;

	vec2 lookup = vec2(phi / (2.0 * PI), theta / PI);
	vec4 irradiance = environment_multiplier * texture(irradianceMap, lookup);

	vec3 diffuse_term = material_color * (1.0 / M_PI) * vec3(irradiance);

	///////////////////////////////////////////////////////////////////////////
	// Task 6 - Look up in the reflection map from the perfect specular 
	//          direction and calculate the dielectric and metal terms. 
	///////////////////////////////////////////////////////////////////////////
	
	vec3 w_i = normalize(reflect(normalize(viewSpaceLightPosition - viewSpacePosition), n));
	vec3 w_h = normalize(w_i + w_o);
	float r_0 = material_fresnel;
	float f_w_i = r_0 + ((1 - r_0) * (pow((1 - dot(w_h, w_i)), 5)));

	float s = material_shininess;
	float roughness = sqrt(sqrt(2/(s+2)));
	vec3 l_i = environment_multiplier * textureLod(reflectionMap, lookup, roughness * 7.0f).xyz;
	vec3 dielectric_term = f_w_i * l_i + (1 - f_w_i) * diffuse_term;
	vec3 metal_term = f_w_i * material_color * l_i;

	float m = material_metalness;
	float r = material_reflectivity;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;
	return r * microfacet_term + (1 - r) * diffuse_term;
}


void main()
{
	///////////////////////////////////////////////////////////////////////////
	// Task 1.1 - Fill in the outgoing direction, wo, and the normal, n. Both
	//            shall be normalized vectors in view-space. 
	///////////////////////////////////////////////////////////////////////////
	vec3 wo = normalize(-1 * viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	vec3 direct_illumination_term = vec3(0.0);
	{ // Direct illumination
		direct_illumination_term = calculateDirectIllumination(wo, n);
	}

	vec3 indirect_illumination_term = vec3(0.0);
	{ // Indirect illumination
		indirect_illumination_term = calculateIndirectIllumination(wo, n);
	}

	///////////////////////////////////////////////////////////////////////////
	// Task 7 - Make glowy things glow!
	///////////////////////////////////////////////////////////////////////////
	vec3 emission_term = material_emission * material_color;

	fragmentColor.xyz =
		direct_illumination_term +
		indirect_illumination_term +
		emission_term;
}
