/*
 * General vertex structure
 * For reference check Geometry/Vertex.h files
 */
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_scalar_block_layout : require

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	//int materialIndex;
	vec3 tangent;
};


layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object

struct Triangle
{
	Vertex v[3];
};


Vertex transformVertex(Vertex vert)
{
	vert.position = gl_ObjectToWorldEXT * vec4(vert.position, 1.0);
	vert.tangent = gl_ObjectToWorldEXT * vec4(vert.tangent, 0.0);
	vert.normal = vec3(vert.normal * gl_WorldToObjectEXT);

	return vert;
}

Triangle unpack(ObjDesc obj)
{
	// Vertex vertex;
	
	// vertex.position = vec3(Vertices[offset + 0], Vertices[offset + 1], Vertices[offset + 2]);
	// vertex.normal = vec3(Vertices[offset + 3], Vertices[offset + 4], Vertices[offset + 5]);
	// vertex.texCoord = vec2(Vertices[offset + 6], Vertices[offset + 7]);
	// vertex.materialIndex = floatBitsToInt(Vertices[offset + 8]);
	Indices    indices     = Indices(obj.indiceBufferAddr);
  	Vertices   vertices    = Vertices(obj.vertexBufferAddr);

	ivec3 ind = indices.i[gl_PrimitiveID + obj.indexPrimitiveOffset];
	Triangle tri;
	tri.v[0] = transformVertex(vertices.v[ind.x]);
	tri.v[1] = transformVertex(vertices.v[ind.y]);
	tri.v[2] = transformVertex(vertices.v[ind.z]);

	return tri;
}
