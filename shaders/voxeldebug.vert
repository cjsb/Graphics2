#version 430
#extension GL_ARB_bindless_texture : require

out vec3 posColor;

uniform ivec3 gridDim;

layout(binding = 0, std430) buffer voxelGridBuffer
{
    layout(rgba32f) image3D voxelGrid;
};

void main()
{
    int z = gl_VertexID / (gridDim.x * gridDim.y);
    int y = (gl_VertexID - z * gridDim.x * gridDim.y) / gridDim.x;
    int x = gl_VertexID - gridDim.x * (y + gridDim.y * z);
    ivec3 gridPos3D = ivec3(x, y, z);
    vec4 voxelDataContent = imageLoad(voxelGrid, gridPos3D);
	gl_Position = vec4(voxelDataContent.xyz, 1.0f); // place at position from voxel grid
    //gl_Position = vec4(gridPos3D, 1.0f); // place at vertex position
    posColor = vec3(voxelDataContent.w);//posColor = voxelDataContent.xyz;
    //posColor = vec3(gridPos3D) / gridDim;
}