#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GL/glew.h>

#include "texture.h"
#include "block.h"

std::map<std::string, unsigned int>& getTextureMap() {
	// done like this instead of global var to make sure it exists at first use
	static std::map<std::string, unsigned int> textureMap;
	return textureMap;
}

std::map<std::string, BlockTexture>& getBlockTextures() {
	static std::map<std::string, BlockTexture> blockTextures;
	return blockTextures;
}

void addBlockTexture(std::string blockName, BlockTexture texture) {
	getBlockTextures().insert(std::pair<std::string, BlockTexture>(blockName, texture));
}

void loadTextures() {
	// block textures
	// sprite sheet offset (should have one entry for each block in the spritesheet)
	Block::addBlockTextureOffset("dirt", 0, 0);
	Block::addBlockTextureOffset("grass_side", 1, 0);
	Block::addBlockTextureOffset("grass", 0, 1);
	Block::addBlockTextureOffset("stone", 1, 1);

	// textures for each block
	addBlockTexture("dirt", BlockTexture("dirt"));
	addBlockTexture("stone", BlockTexture("stone"));
	addBlockTexture("grass", BlockTexture("grass", "dirt", "grass_side", "grass_side", "grass_side", "grass_side"));

	Block::loadSpritesheet();
}

unsigned int getTextureId(std::string name) {
	if (getTextureMap().find(name) == getTextureMap().end()) {
		std::cerr << "Texture not found." << std::endl;
		return 0;
	}

	return getTextureMap()[name];
}

void bindTexture(std::string name, unsigned int shaderId) {
	// get the texture id
	unsigned int textureId = getTextureId(name);

	// activate texture unit and bind texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureId);

	// get uniform
	glUniform1i(glGetUniformLocation(shaderId, "texture"), 0);
}

unsigned int loadTexture(std::string path, std::string name) {
	// load image
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);		// makes sure the image is the right way up
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

	// generate texture
	unsigned int textureId;
	glGenTextures(1, &textureId);

	// add id to map
	getTextureMap()[name] = textureId;

	// bind texture and set parameters
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// fill with data
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	stbi_image_free(data);

	return textureId;
}