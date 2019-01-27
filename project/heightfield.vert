#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 texCoordIn;

layout(binding = 9) uniform sampler2D colorHF;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////

uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform int tesselation;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpacePosition;
out vec3 viewSpaceNormal;

void main() 
{

	texCoord = texCoordIn;

	float heightFactor = 0.1f;

	
	float offset = 0.001;
	float height0 = texture2D(colorHF, texCoord.xy).r;
	float height1 = texture2D(colorHF, texCoord.xy + vec2(offset, 0.0)).r;
	float height2 = texture2D(colorHF, texCoord.xy + vec2(0.0, offset)).r;

	vec3 p0 = vec3(0, height0, 0);
	vec3 px = vec3(offset, height1, 0);
	vec3 pz = vec3(0, height2, offset);

	vec3 vec0x = normalize(px-p0);
	vec3 vec0z = normalize(pz-p0);

	//vec3 normal = normalize(vec3(-(height1-height0), offset, -(height2-height0)));
	vec3 normal = normalize(-cross(vec0x, vec0z));

	viewSpaceNormal = (normalMatrix * vec4(normal, 0f)).xyz;


	//viewSpaceNormal = (normalMatrix * normalize(vec4(cross(vec01, vec02),0f))).xyz;
	
	/*
	// Sanity check
	float delta = 0.001;
	float height = texture2D(colorHF, texCoord.xy).r;
	float dx = texture2D(colorHF, texCoord.xy + vec2(delta, 0.0)).r - height;
	float dy = texture2D(colorHF, texCoord.xy + vec2(0.0, delta)).r - height;
	vec3 normal_ws = normalize(vec3(-dx, delta, -dy));
	viewSpaceNormal = (normalMatrix * vec4(normal_ws, 0.0)).xyz;
	*/

	viewSpacePosition = (modelViewMatrix * vec4(position.x, height0*heightFactor, position.z, 1.0)).xyz;
	gl_Position = modelViewProjectionMatrix * vec4(position.x, height0*heightFactor, position.z, 1.0);

}
