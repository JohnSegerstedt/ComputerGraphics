
#include "heightfield.h"

#include <iostream>
#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <stb_image.h>

#include <labhelper.h>

using namespace glm;
using std::string;

const int maxIndices = 100000000;

// The shaderProgram holds the vertexShader and fragmentShader


GLuint	positionBuffer, indexBuffer, vertexArrayObject, textureBuffer, normalBuffer;

float positions[maxIndices];
float texcoords[maxIndices];
int indices[maxIndices];

int positionsSize;
int indicesSize;

int numberOfVertices;

HeightField::HeightField(void)
	: m_meshResolution(0)
	, m_vao(UINT32_MAX)
	, m_positionBuffer(UINT32_MAX)
	, m_uvBuffer(UINT32_MAX)
	, m_indexBuffer(UINT32_MAX)
	, m_numIndices(0)
	, m_texid_hf(UINT32_MAX)
	, m_texid_diffuse(UINT32_MAX)
	, m_heightFieldPath("")
	, m_diffuseTexturePath("")
{
}

void HeightField::loadHeightField(const std::string &heigtFieldPath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	float * data = stbi_loadf(heigtFieldPath.c_str(), &width, &height, &components, 1);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << heigtFieldPath << ".\n";
		return;
	}

	if (m_texid_hf == UINT32_MAX) {
		glGenTextures(1, &m_texid_hf);
	}
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data); // just one component (float)

	m_heightFieldPath = heigtFieldPath;
	std::cout << "Successfully loaded heigh field texture: " << heigtFieldPath << ".\n";
}

void HeightField::loadDiffuseTexture(const std::string &diffusePath)
{
	int width, height, components;
	stbi_set_flip_vertically_on_load(true);
	uint8_t * data = stbi_load(diffusePath.c_str(), &width, &height, &components, 3);
	if (data == nullptr) {
		std::cout << "Failed to load image: " << diffusePath << ".\n";
		return;
	}

	if (m_texid_diffuse == UINT32_MAX) {
		glGenTextures(1, &m_texid_diffuse);
	}

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // plain RGB
	glGenerateMipmap(GL_TEXTURE_2D);

	std::cout << "Successfully loaded diffuse texture: " << diffusePath << ".\n";
}


void HeightField::generateMesh(int tesselation) {
	// generate a mesh in range -1 to 1 in x and z
	// (y is 0 but will be altered in height field vertex shader)

	int size = tesselation + 1;
	numberOfVertices = size * size;
	positionsSize = 3 * size * size;
	indicesSize = 3 * tesselation * tesselation * 2;

	// Fills "positions"
	float value = 2.0f / tesselation;
	int count = 0;
	for (int i = 0; i <= tesselation; i++) {
		for (int j = 0; j <= tesselation; j++) {
			positions[count] = -1.0f + value * ((float)i);
			positions[count + 1] = 0.0f;
			positions[count + 2] = -1.0f + value * ((float)j);
			count += 3;
		}
	}
	count -= 3;

	// Puts all triangles into Indices
	int count2 = 0;
	for (int index = 0; index < numberOfVertices; index++) {
		if (!(index % size == tesselation) &&
			!(index + size >= numberOfVertices)) {

			indices[count2] = index;
			indices[count2 + 1] = index + 1;
			indices[count2 + 2] = index + size;

			indices[count2 + 3] = index + 1;
			indices[count2 + 4] = index + size + 1;
			indices[count2 + 5] = index + size;

			count2 += 6;
		}
	}

	
	// Fills "texcoords"
	float value2 = 1.0f / tesselation;
	int count3 = 0;
	for (int i = 0; i <= tesselation; i++) {
		for (int j = 0; j <= tesselation; j++) {
			texcoords[count3] = value2 * ((float)i);
			texcoords[count3 + 1] = value2 * ((float)j);
			count3 += 2;
		}
	}



	///////////////////////////////////////////////////////////////////////////
	// Create the vertex array object
	///////////////////////////////////////////////////////////////////////////
	glGenVertexArrays(1, &vertexArrayObject);
	glBindVertexArray(vertexArrayObject);

	glGenBuffers(1, &positionBuffer);												// Create a handle for the vertex position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);									// Set the newly created buffer as the current one
	glBufferData(GL_ARRAY_BUFFER, positionsSize * 4, positions, GL_STATIC_DRAW);	// Send the vetex position data to the current buffer
	glVertexAttribPointer(0, 3, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(0);


	glGenBuffers(1, &indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * 4, indices, GL_STATIC_DRAW);


	//glGenBuffers(1, &normalBuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(numberOfVertices), , GL_STATIC_DRAW);
	//glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	//glEnableVertexAttribArray(1);

	glGenBuffers(1, &textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, false/*normalized*/, 0/*stride*/, 0/*offset*/);
	glEnableVertexAttribArray(2);


}

void HeightField::submitTriangles(void)
{
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glActiveTexture(GL_TEXTURE11);

	glBindTexture(GL_TEXTURE_2D, m_texid_diffuse);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, m_texid_hf);
	glBindVertexArray(vertexArrayObject);

	glDrawElements(GL_TRIANGLES, indicesSize, GL_UNSIGNED_INT, 0);


	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/*
//allocate the array
float** positions = new float*[size];
for (int i = 0; i < size; i++)
positions[i] = new float[size];

//deallocate the array
for (int i = 0; i < size; i++)
delete[] positions[i];
delete[] positions;*/


//float positions[maxIndices];
//int indices[maxIndices];