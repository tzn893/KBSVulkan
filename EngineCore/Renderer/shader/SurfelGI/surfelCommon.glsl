#ifndef SURFEL_COMMON_GLSL
#define SURFEL_COMMON_GLSL


#ifndef MAX_SURFEL_BUFFER_COUNT
#define MAX_SURFEL_BUFFER_COUNT 65535
#endif

#ifndef SURFEL_GRID_COUNT
#define SURFEL_GRID_COUNT 64
#endif

#ifndef SURFEL_CELL_SURFEL_COUNT
#define SURFEL_CELL_SURFEL_COUNT 31
#endif

#ifndef SURFEL_CELL_INDEX_POOL_COUNT
#define SURFEL_CELL_INDEX_POOL_COUNT 4194304
#endif

#include "frustrum.glsl"

struct SurfelGlobalUniform
{
    mat4        viewInverse;
    mat4        projInverse;
    vec4        resolution;

    vec3        surfelCellExtent;
    uint        currentFrame;    
    vec3        accelerationStructurePosition;
    float       surfelRadius;

    Frustrum    frustrum;    
    vec3        cameraPosition;
};

int GetGridCellIdx(SurfelGlobalUniform uni, vec3 position)
{
    ivec3 gridIdx = ivec3((position - uni.accelerationStructurePosition) / uni.surfelCellExtent);
    return gridIdx.x + gridIdx.y * SURFEL_GRID_COUNT + gridIdx.z * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT;
}

int GetNeighborCellIdx(int cellIdx, int xOffset, int yOffset, int zOffset)
{
    return cellIdx  + xOffset + yOffset * SURFEL_GRID_COUNT + zOffset * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT;
}

vec3 GetGridCellPosition(SurfelGlobalUniform uni, int idx)
{
    int gridIdxX = idx % SURFEL_GRID_COUNT;
    int gridIdxY = (idx / SURFEL_GRID_COUNT) % SURFEL_GRID_COUNT;
    int gridIdxZ = idx / (SURFEL_GRID_COUNT * SURFEL_GRID_COUNT);

    return uni.surfelCellExtent * ivec3(gridIdxX, gridIdxY, gridIdxZ)
        + uni.accelerationStructurePosition;
}

struct SurfelInfo
{
    vec4		radianceWeight;
    vec3		position;
    float		radius;
    //			the last frame surfel has contribution to main view
    vec3        normal;
    uint	    lastContributionFrame;
};

int GridSurfelCellIntersection(SurfelGlobalUniform uni, int idx, SurfelInfo surfel)
{
    vec3 cellPosition = GetGridCellPosition(uni, idx);
    vec3 cellHalfExt = uni.surfelCellExtent / 2;
    vec3 cellCenter = cellPosition + cellHalfExt;

    vec3 distanceFromSurfelToCell =  abs(surfel.position - cellCenter);

    if(distanceFromSurfelToCell.x < surfel.radius + cellHalfExt.x &&
        distanceFromSurfelToCell.y < surfel.radius + cellHalfExt.y &&
        distanceFromSurfelToCell.z < surfel.radius + cellHalfExt.z)
    {
        return 1;
    }
    
    return 0;
}

float coverRatio(SurfelInfo surfel, vec3 position, vec3 normal)
{
    float surfelDistance = distance(surfel.position, position);
    
    float s = surfel.radius / 4;
    const float e = 2.71828;
    float x_div_sigma = surfelDistance / s;
    return max(dot(surfel.normal, normal), 0.0) * pow(e, -x_div_sigma * x_div_sigma * 0.5);
}

/*
struct SurfelAccelerationStructureCell
{
    int surfels[SURFEL_CELL_SURFEL_COUNT];
    int surfelCount;
};
*/

struct SurfelAccelerationStructure
{
    //SurfelAccelerationStructureCell cells[SURFEL_GRID_COUNT * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT];
    int   asIndexPool[SURFEL_CELL_INDEX_POOL_COUNT];
    ivec2 cellIndexStartCount[SURFEL_GRID_COUNT * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT];
    int   cellIndexCount;
};

struct SurfelAccelerationStructureV2
{
    int   asIndexPool[SURFEL_CELL_INDEX_POOL_COUNT];
    ivec2 cellIndexStartCount[SURFEL_GRID_COUNT * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT];
    int   cellDirtyFlag[SURFEL_GRID_COUNT * SURFEL_GRID_COUNT * SURFEL_GRID_COUNT];
    int   cellIndexCount;
};

struct SurfelBuffer
{
    SurfelInfo surfels[MAX_SURFEL_BUFFER_COUNT];
};

struct SurfelIndexBuffer
{
    int  liveSurfelCount;
    int  liveSurfelIdx[MAX_SURFEL_BUFFER_COUNT];
    int  avaliableSurfelCount;
    int  avaliableSurfelIdx[MAX_SURFEL_BUFFER_COUNT];
};


#endif