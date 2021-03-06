#pragma once

#include "Terrain.h"

Terrain::Terrain() :
	cellSize(.03f)
{
	GLint numBytes;

	GLint index = 0;
	GLfloat posOX = 0.f;
	GLfloat posOZ = 0.f;

	/* Read the heightmap */
	heights = stbi_load(
		"Source/Teme/Tema3/Textures/heightmap.png",
		&terrainWidth,
		&terrainHeight,
		&numChannels,
		STBI_grey
	);

	numBytes = terrainHeight * terrainWidth * numChannels;

	std::vector<VertexFormat> vertices;
	std::vector<GLushort> indices;

	for (GLushort i = 0; i != terrainHeight; ++i, posOZ += cellSize)
	{
		posOX = 0.f;
		for (GLushort j = 0; j != terrainWidth; ++j, ++index, posOX += cellSize)
		{
			/* Set the vertices for this row */
			vertices.emplace_back(
				glm::vec3(posOX, 1.f, posOZ),
				glm::vec3(0.f),
				glm::vec3(0.f, 1.f, 0.f),
				glm::vec2((GLfloat)i / terrainHeight, (GLfloat)j / terrainWidth)
			);

			/* Set the indices */
			if (i != 0 && (j + 1) % terrainWidth)
			{
				/* Upper triangle */
				indices.push_back(index);
				indices.push_back(index - terrainWidth + 1);
				indices.push_back(index - terrainWidth);

				/* Lower triangle */
				indices.push_back(index);
				indices.push_back(index + 1);
				indices.push_back(index - terrainWidth + 1);
			}
		}
	}

	/* Create the mesh used for the terrain */
	mesh = WormsUtils::CreateMesh("TerrainMesh", vertices, indices);

	/* Read and create the shader used for the terrain */
	shader = new Shader("TerrainShader");
	shader->AddShader(
		"Source/Teme/Tema3/Shaders/TerrainVS.glsl",
		GL_VERTEX_SHADER
	);
	shader->AddShader(
		"Source/Teme/Tema3/Shaders/TerrainFS.glsl",
		GL_FRAGMENT_SHADER
	);
	shader->CreateAndLink();

	CreateHeightMap();

	/* Load the terrain texture */
	texture = new Texture2D();
	texture->Load2D("Source/Teme/Tema3/Textures/terrain_texture.jpg", GL_REPEAT);
}

GLvoid Terrain::CreateHeightMap()
{
	GLuint heightTextureID;

	/* Generate and bind the new texture ID */
	glGenTextures(1, &heightTextureID);
	glBindTexture(GL_TEXTURE_2D, heightTextureID);

	/**
	* Set the texture parameters (MIN_FILTER, MAG_FILTER and WRAPPING MODE)
	* using glTexParameteri
	*/
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	/* Use glTextImage2D to set the texture data */
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_R8,
		terrainWidth,
		terrainHeight,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		(GLvoid*)heights
	);

	/* Generate texture mip-maps */
	glGenerateMipmap(GL_TEXTURE_2D);

	/**
	* Save the texture into a wrapper Texture2D class for using easier later
	* during rendering phase
	*/
	heightTexture = new Texture2D();
	heightTexture->Init(
		heightTextureID,
		terrainWidth,
		terrainHeight,
		numChannels
	);
}

GLboolean Terrain::CheckCollision(
	GLfloat posX,
	GLfloat posY,
	GLfloat posZ,
	GLfloat radius
)
{
	GLshort mapRadius	= (GLshort)ceil(radius / cellSize);
	GLfloat sqRadius	= radius * radius;
	GLint proj			= (GLint)floor(posX / cellSize) * terrainWidth
						  + (GLint)floor(posZ / cellSize);
	
	GLint pos;
	GLfloat mapY;

	/* Check if the projectile touches the terrain */
	for (GLint i = -mapRadius; i != mapRadius; ++i)
	{
		pos = proj + i * terrainWidth;

		for (GLint j = -mapRadius; j != mapRadius; ++j)
		{
			mapY = (GLfloat)heights[pos + j] / 255.f;

			if ((j * j + i * i) * cellSize * cellSize
				+ (mapY - posY) * (mapY - posY) <= sqRadius)
			{
				PlaySound(TEXT("fart.wav"), NULL, SND_FILENAME | SND_ASYNC);
				return GLFW_TRUE;
			}
		}
	}

	return GLFW_FALSE;
}

GLvoid Terrain::DeformTerrain(
	GLfloat posX,
	GLfloat posY,
	GLfloat posZ,
	GLfloat blastRadius
)
{
	GLshort mapRadius	= (GLshort)ceil(blastRadius / cellSize);
	GLint sqRadius		= 1L * mapRadius * mapRadius;
	GLint proj			= (GLint)floor(posX / cellSize) * terrainWidth
		+ (GLint)floor(posZ / cellSize);

	GLint pos;

	/* Make a circular hole */
	for (GLint i = -mapRadius; i != mapRadius; ++i)
	{
		pos = proj + i * terrainWidth;

		for (GLint j = -mapRadius; j != mapRadius; ++j)
		{
			if (i * i + j * j <= sqRadius)
			{
				if (heights[pos + j] < 50)
				{
					heights[pos + j] = 0;
				} else
				{
					heights[pos + j] -= 50;
				}
			}
		}
	}

	delete heightTexture;
	CreateHeightMap();
}

Terrain::~Terrain()
{
	delete shader;
	delete mesh;
	delete texture;
	delete heightTexture;
	delete[] heights;
}

Texture2D* Terrain::GetHeightTexture(GLfloat posX, GLfloat posY, GLfloat posZ)
{
	return heightTexture;
}

Texture2D* Terrain::GetTexture()
{
	return texture;
}

Mesh* Terrain::GetMesh()
{
	return mesh;
}

Shader* Terrain::GetShader()
{
	return shader;
}

GLint Terrain::GetHeightMapHeight()
{
	return terrainHeight;
}

GLint Terrain::GetHeightMapWidth()
{
	return terrainWidth;
}
