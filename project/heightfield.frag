#version 420
// required by GLSL spec Sect 4.5.3 (though nvidia does not, amd does)
precision highp float;

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////
//uniform float material_reflectivity;
//uniform float material_metalness;
//uniform float material_fresnel;
//uniform float material_shininess;
//uniform float material_emission;


uniform int has_emission_texture;
layout(binding = 5) uniform sampler2D emissiveMap;

///////////////////////////////////////////////////////////////////////////////
// Environment
///////////////////////////////////////////////////////////////////////////////
layout(binding = 6) uniform sampler2D environmentMap;
layout(binding = 7) uniform sampler2D irradianceMap;
layout(binding = 8) uniform sampler2D reflectionMap;
layout(binding = 10) uniform sampler2DShadow shadowMapTex;
//layout(binding = 10) uniform sampler2D shadowMapTex;
uniform float environment_multiplier;

///////////////////////////////////////////////////////////////////////////////
// Light source
///////////////////////////////////////////////////////////////////////////////
uniform vec3 point_light_color = vec3(1.0, 1.0, 1.0);
uniform float point_light_intensity_multiplier = 50.0;
uniform vec3 viewSpaceLightDir;
uniform float spotOuterAngle;
uniform float spotInnerAngle;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////
#define M_PI 3.1415926535897932384626433832795


///////////////////////////////////////////////////////////////////////////////
// Input varyings from vertex shader
///////////////////////////////////////////////////////////////////////////////
in vec2 texCoord;
in vec3 viewSpacePosition;
in vec3 viewSpaceNormal;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 viewInverse;
uniform vec3 viewSpaceLightPosition;
layout(location = 0) out vec4 fragmentColor;
layout(binding = 9) uniform sampler2D colorHF;
layout(binding = 11) uniform sampler2D colorTexture;


vec3 calculateDirectIllumiunation(vec3 w_o, vec3 n, vec3 material_color)

{

	float material_shininess = 10;
	float material_fresnel = 0.04;
	float material_metalness = 0;
	float material_reflectivity = 1;

	vec3 w_i = normalize(viewSpaceLightPosition - viewSpacePosition);
	vec3 w_distance = viewSpaceLightPosition - viewSpacePosition;
	float dSquare = dot(w_distance, w_distance);
	vec3 l_i = point_light_intensity_multiplier * point_light_color * 1 / dSquare;

	// debug
	//n.x = -n.x;
	//n.z = -n.z;
	//n.y = -n.y;
	//return vec3(dot(n, w_i));
	if (dot(n, w_i) <= 0.0f) {
		return vec3(0.0f);
	}

	vec3 diffuse_term = material_color * 1.0f / M_PI * abs(dot(n, w_i)) * l_i;

	vec3 w_h = normalize(w_i + w_o);
	float s = material_shininess;
	float d_w_h = ((s + 2) / (2 * M_PI)) * pow(dot(n, w_h), s);

	float r_0 = material_fresnel;
	float f_w_i = r_0 + (1 - r_0) * pow((1 - dot(w_h, w_i)), 5);

	float g_w_i_w_o = min(1, min(2 * (dot(n, w_h)*dot(n, w_o)) / dot(w_o, w_h), 2 * (dot(n, w_h)*dot(n, w_i)) / dot(w_o, w_h)));

	float brdf = (f_w_i * d_w_h * g_w_i_w_o) / (4 * dot(n, w_o)*dot(n, w_i));

	float m = material_metalness;
	float r = material_reflectivity;

	vec3 dielectric_term = brdf * dot(n, w_i) * l_i + (1 - f_w_i) * diffuse_term;
	vec3 metal_term = brdf * material_color * dot(n, w_i) * l_i;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;

	return r * microfacet_term + (1 - r) * diffuse_term;
}

vec3 calculateIndirectIllumination(vec3 w_o, vec3 n, vec3 material_color)
{

	float material_shininess = 10;
	float material_fresnel = 0.04;
	float material_metalness = 0;
	float material_reflectivity = 1;

	vec3 n_ws = mat3(viewInverse) * n;

	float theta = acos(max(-1.0f, min(1.0f, n_ws.y)));
	float phi = atan(n_ws.z, n_ws.x);
	if (phi < 0.0f) phi = phi + 2.0f * M_PI;

	vec2 lookup = vec2(phi / (2.0 * M_PI), theta / M_PI);
	vec4 irradiance = environment_multiplier * texture(irradianceMap, lookup);

	vec3 diffuse_term = material_color * (1.0 / M_PI) * vec3(irradiance);

	vec3 w_i = normalize(reflect(-w_o, n));
	vec3 w_i_ws = normalize(mat3(viewInverse) * w_i);
	theta = acos(max(-1.0f, min(1.0f, w_i_ws.y)));
	phi = atan(w_i_ws.z, w_i_ws.x);
	if (phi < 0.0f) phi = phi + 2.0f * M_PI;
	lookup = vec2(phi / (2.0 * M_PI), theta / M_PI);

	vec3 w_h = normalize(w_i + w_o);
	float r_0 = material_fresnel;
	float f_w_i = r_0 + ((1 - r_0) * (pow((1 - dot(w_h, w_i)), 5)));

	float s = material_shininess;
	float roughness = sqrt(sqrt(2 / (s + 2)));
	vec3 l_i = environment_multiplier * textureLod(reflectionMap, lookup, roughness * 7.0f).xyz;
	vec3 dielectric_term = f_w_i * l_i + (1 - f_w_i) * diffuse_term;
	vec3 metal_term = f_w_i * material_color * l_i;

	float m = material_metalness;
	float r = material_reflectivity;
	vec3 microfacet_term = m * metal_term + (1 - m) * dielectric_term;
	
	//return w_i;
	//return vec3(pow((1 - dot(w_h, w_i)), 5));
	//return vec3(r_0 + ((1 - r_0) * (pow((1 - dot(w_h, w_i)), 5))));
	return r * microfacet_term + (1 - r) * diffuse_term;
}



void main() 
{
	vec3 material_color = texture2D(colorTexture, texCoord.xy).xyz;

	float visibility = 1.0;


	vec3 wo = -normalize(viewSpacePosition);
	vec3 n = normalize(viewSpaceNormal);

	vec3 direct_illumination_term = visibility * calculateDirectIllumiunation(wo, n, material_color);
	vec3 indirect_illumination_term = calculateIndirectIllumination(wo, n, material_color);


	vec3 shading = direct_illumination_term +
	indirect_illumination_term;
	//vec3 shading = emission_term;

	//fragmentColor = vec4(irradiance, 1f);
	//fragmentColor = vec4(vec3(n.x, -n.y, n.z), 1f);
	//fragmentColor = vec4(material_color, 1.0);
	fragmentColor = vec4(shading, 1f);
	//fragmentColor = vec4(material_color, 1f); 
	//fragmentColor = vec4(n * 0.5 + 0.5, 1.0);
	return;
}
