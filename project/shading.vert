#version 420
///////////////////////////////////////////////////////////////////////////////
// Input vertex attributes
///////////////////////////////////////////////////////////////////////////////
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 texCoordIn;

///////////////////////////////////////////////////////////////////////////////
// Input uniform variables
///////////////////////////////////////////////////////////////////////////////
uniform mat4 normalMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpaceNormal;
out vec3 viewSpacePosition;


void main()
{
	gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
	texCoord = texCoordIn;
	viewSpaceNormal = (normalMatrix * vec4(normalIn, 0.0)).xyz;
	viewSpacePosition = (modelViewMatrix * vec4(position, 1.0)).xyz;

}








/*#version 420
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

///////////////////////////////////////////////////////////////////////////////
// Output to fragment shader
///////////////////////////////////////////////////////////////////////////////
out vec2 texCoord;
out vec3 viewSpaceNormal;
out vec3 viewSpacePosition;


void main() 
{
	gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
	
	vec2 texCoord1;
	vec2 texCoord2;

	texCoord = texCoordIn;
	texCoord1.x = texCoord.x;
	texCoord1.y = texCoord.y + 0.000001;
	texCoord2.x = texCoord.x + 0.000001;
	texCoord2.y = texCoord.y;

	float height0 = texture2D(colorHF, texCoord.xy).r * 0.1f;
	float height1 = texture2D(colorHF, texCoord1.xy).r * 0.1f;
	float height2 = texture2D(colorHF, texCoord2.xy).r * 0.1f;

	vec3 point0 = new vec3(texCoord.x, texCoord.y, height0);
	vec3 point1 = new vec3(texCoord1.x, texCoord1.y, height1);
	vec3 point2 = new vec3(texCoord2.x, texCoord2.y, height2);

	vec3 vec01 = point1 - point0;
	vec3 vec02 = point2 - point0;

	//viewSpaceNormal = new vec3(1f, 1f, 0f);
	//viewSpaceNormal = cross(vec01, vec02);
	viewSpaceNormal = (normalMatrix * vec4(normalIn, 0.0)).xyz;
	
	viewSpacePosition = (modelViewMatrix * vec4(position, 1.0)).xyz;

}


/*
texCoord0.x = texCoord.x;
texCoord0.y = texCoord.y + 1 / tesselation;
texCoord1.x = texCoord.x + 1 / tesselation;
texCoord1.y = texCoord.y;
texCoord2.x = texCoord.x;
texCoord2.y = texCoord.y - 1 / tesselation;
texCoord3.x = texCoord.x - 1 / tesselation;
texCoord3.y = texCoord.y;

float height0 = texture2D(colorHF, texCoord0.xy).r * 0.1f;
float height1 = texture2D(colorHF, texCoord1.xy).r * 0.1f;
float height2 = texture2D(colorHF, texCoord2.xy).r * 0.1f;
float height3 = texture2D(colorHF, texCoord3.xy).r * 0.1f;

float heightC = texture2D(colorHF, texCoord.xy).r * 0.1f;

vec3 point0 = new vec3(texCoord0.x, texCoord0.y, height0);

vec3 normal01 = cross(new vec3(new vec3() - new vec3()), new vec3(new vec3() - new vec3()));
*/