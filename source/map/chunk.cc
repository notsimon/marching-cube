#include "map.hh"

#include <fstream>

Chunk::Chunk (const vec3i& coord)
	: coord (coord),
	  node (0),
	  geometry (0),
	  mesh (0),
	  material (0) {
};

void Chunk::load (const std::string& filepath) {
	std::ifstream file (filepath.c_str(), std::ios::binary);
	
	file.read((char*)data.data(), data.size());
	std::cout << data.size() << " voxels read" << std::endl;

	file.close();
};

void Chunk::save (const std::string& filepath) {
	std::ofstream file (filepath.c_str(), std::ios::binary);
	
	file.write((char*)data.data(), data.size());
	std::cout << data.size() << " voxels written" << std::endl;	

	file.close();
};