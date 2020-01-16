#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

#include "chunk.h"

std::map<uint32_t, Chunk*> Chunk::chunkList = std::map<uint32_t, Chunk*>();

void Chunk::getChunkPosition(int globalX, int globalZ, int& chunkX, int& chunkZ) {
	// two separate cases for positive and negative
	if (globalX >= 0) {
		chunkX = globalX - (globalX % CHUNK_SIZE);
	}
	else {
		chunkX = globalX + (-globalX % CHUNK_SIZE) - CHUNK_SIZE;
	}

	if (globalZ >= 0) {
		chunkZ = globalZ - (globalZ % CHUNK_SIZE);
	}
	else {
		chunkZ = globalZ + (-globalZ % CHUNK_SIZE) - CHUNK_SIZE;
	}
}

void Chunk::addBlock(int x, int y, int z, std::string textureName) {
	// make sure block is in bounds vertically
	if (y < 0 || y >= WORLD_HEIGHT) {
		std::cerr << "Attempted to add block out of bounds (y = " << y << ")." << std::endl;
		return;
	}

	// calculate correct chunk position
	int chunkX;
	int chunkZ;
	getChunkPosition(x, z, chunkX, chunkZ);

	// calculate chunk index for map
	uint32_t chunkIndex = getChunkIndex(chunkX, chunkZ);

	// check if a chunk exists at the given position, if not create it
	if (chunkList.find(chunkIndex) == chunkList.end()) {
		chunkList[chunkIndex] = new Chunk(glm::ivec2(chunkX, chunkZ));
	}

	// add block to the right chunk
	Chunk* chunk = chunkList[chunkIndex];
	chunk->blocks[x - chunkX][y][z - chunkZ] = new Block(x - chunkX, y, z - chunkZ, textureName);

	// set update flag
	chunk->updated = false;
}

void Chunk::removeBlock(int x, int y, int z) {
	// calculate correct chunk position
	int chunkX;
	int chunkZ;
	getChunkPosition(x, z, chunkX, chunkZ);

	// calculate chunk index for map
	uint32_t chunkIndex = getChunkIndex(chunkX, chunkZ);

	// check if a chunk exists at the given position
	if (chunkList.find(chunkIndex) == chunkList.end()) {
		std::cerr << "Chunk for block position (x: " << x << ", y: " << y << ", z: " << z << ") does not exist!" << std::endl;
		return;
	}

	// get block if it exists
	Chunk* chunk = chunkList[chunkIndex];
	Block* block = chunk->blocks[x - chunkX][y][z - chunkZ];

	if (block == nullptr) {
		std::cerr << "No block found at position (x: " << x << ", y: " << y << ", z: " << z << ") does not exist!" << std::endl;
		return;
	}

	// remove block from array and free its memory
	chunk->blocks[x - chunkX][y][z - chunkZ] = nullptr;
	delete block;

	// set update flag
	chunk->updated = false;
}

uint32_t Chunk::getChunkIndex(int x, int z) {
	return (x << 16) + z;
}

Chunk::Chunk(glm::ivec2 pos) : blocks(), neighborChunks(), verts(std::vector<Vertex>()) {
	// check position
	if (pos.x % CHUNK_SIZE != 0 || pos.y % CHUNK_SIZE != 0) {
		std::cerr << "Invalid chunk position (x: " << pos.x << ", z: " << pos.y << ") given!" << std::endl;
		std::cerr << "This chunk will not work as expected!" << std::endl;

		return;
	}

	// set position
	this->pos = glm::ivec3(pos.x, 0, pos.y);

	// set model matrix
	model = glm::translate(glm::mat4(1.0f), glm::vec3(this->pos));

	// add to chunkList
	chunkList[getChunkIndex(pos.x, pos.y)] = this;

	// generate vao and set attributes
	glGenVertexArrays(1, &vaoId);
	glGenBuffers(1, &bufferId);

	// bind buffer to vao
	glBindVertexArray(vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, bufferId);

	// set vertex attribs
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) (3 * sizeof(float)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

Chunk::~Chunk() {
	// remove this chunk from the list
	chunkList.erase(getChunkIndex(this->pos.x, this->pos.z));
}

void Chunk::updateBlockFaces() {
	// loop through all chunk blocks
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				// set face status of neighbor blocks depending on this one
				// if this block exists, neighbor blocks have hidden faces
				// if it doesn't exist, neighbor blocks have exposed faces
				bool exposed = (blocks[x][y][z] == nullptr);

				// set status of neighbor blocks, if they exist
				Block* neighbor;
				if (x + 1 < CHUNK_SIZE && (neighbor = blocks[x + 1][y][z]) != nullptr) {
					neighbor->boolFace(BIT_FACE_LEFT, exposed);
				}
				if (x - 1 >= 0 && (neighbor = blocks[x - 1][y][z]) != nullptr) {
					neighbor->boolFace(BIT_FACE_RIGHT, exposed);
				}
				if (y + 1 < WORLD_HEIGHT && (neighbor = blocks[x][y + 1][z]) != nullptr) {
					neighbor->boolFace(BIT_FACE_BOTTOM, exposed);
				}
				if (y - 1 >= 0 && (neighbor = blocks[x][y - 1][z]) != nullptr) {
					neighbor->boolFace(BIT_FACE_TOP, exposed);
				}
				if (z + 1 < CHUNK_SIZE && (neighbor = blocks[x][y][z + 1]) != nullptr) {
					neighbor->boolFace(BIT_FACE_FRONT, exposed);
				}
				if (z - 1 >= 0 && (neighbor = blocks[x][y][z - 1]) != nullptr) {
					neighbor->boolFace(BIT_FACE_BACK, exposed);
				}
			}
		}
	}
}

void Chunk::addFace(const Vertex* face, int x, int y, int z) {
	// loop through all 6 verts of this face
	for (const Vertex* vertPtr = face; vertPtr < face + 6; vertPtr++) {
		// new vertex which will be added to the verts list
		Vertex outVert = *vertPtr;

		// shift position to be at right spot
		outVert.pos[0] += 0.5 + x;
		outVert.pos[1] += 0.5 + y;
		outVert.pos[2] += 0.5 + z;

		// add to verts
		verts.push_back(outVert);
	}
}

void Chunk::updateVerts() {
	verts.clear();

	// loop through all block positions
	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				Block* block;
				if ((block = blocks[x][y][z]) == nullptr) {
					continue;
				}

				// add exposed faces
				if (block->getFace(BIT_FACE_TOP)) {
					addFace(Block::TOP_FACE, x, y, z);
				}
				if (block->getFace(BIT_FACE_BOTTOM)) {
					addFace(Block::BOTTOM_FACE, x, y, z);
				}
				if (block->getFace(BIT_FACE_LEFT)) {
					addFace(Block::LEFT_FACE, x, y, z);
				}
				if (block->getFace(BIT_FACE_RIGHT)) {
					addFace(Block::RIGHT_FACE, x, y, z);
				}
				if (block->getFace(BIT_FACE_FRONT)) {
					addFace(Block::FRONT_FACE, x, y, z);
				}
				if (block->getFace(BIT_FACE_BACK)) {
					addFace(Block::BACK_FACE, x, y, z);
				}
			}
		}
	}
}

void Chunk::updateBuffer() {
	// update buffer with verts
	glNamedBufferData(bufferId, verts.size() * sizeof(Vertex), &verts[0], GL_DYNAMIC_DRAW);
}


void Chunk::addNeighbor(Chunk* chunk) {
	// get the position of the neighbor chunk
	glm::vec3 chunkPos = chunk->getPosition();

	// make sure the neighbor is actually next to this chunk
	// must be either same x/diff z or same z/diff x
	if ((chunkPos.x == pos.x && chunkPos.z == pos.z) || (chunkPos.x != pos.x && chunkPos.z != pos.z)) {
		std::cerr << "Incorrect neighbor chunk given!" << std::endl;
		return;
	}

	// figure our which chunk this is and add it to the right spot
	if (chunkPos.z < pos.z) {
		// front
		neighborChunks[0] = chunk;
	}
	else if (chunkPos.x > pos.x) {
		// right
		neighborChunks[1] = chunk;
	}
	else if (chunkPos.z > pos.z) {
		// back
		neighborChunks[2] = chunk;
	}
	else {
		// left
		neighborChunks[3] = chunk;
	}
}

void Chunk::update() {
	// don't do anything if update isn't needed
	if (updated) {
		return;
	}

	// call the update functions
	updateBlockFaces();
	updateVerts();
	updateBuffer();

	// set update flag
	updated = true;
}

bool Chunk::isUpdated() {
	return updated;
}

glm::ivec3 Chunk::getPosition() {
	return pos;
}

unsigned int Chunk::getVaoId() {
	// warn user if data is not up to date
	if (!updated) {
		std::cout << "Warning: vertex data of this chunk is not up to date" << std::endl;
	}

	return vaoId;
}

int Chunk::getVertexCount() {
	return verts.size();
}

glm::mat4 Chunk::getModelMatrix() {
	return model;
}