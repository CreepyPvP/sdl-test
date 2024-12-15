all: assets/default.frag.spv assets/default.vert.spv

assets/default.frag.spv: assets/default.frag
	glslc assets/default.frag -o assets/default.frag.spv

assets/default.vert.spv: assets/default.vert
	glslc assets/default.vert -o assets/default.vert.spv
