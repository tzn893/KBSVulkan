#pragma once
#include "Common.h"
#include "gvk.h"
#include "Platform/Platform.h"
#include "Renderer/Flags.h"

namespace kbs
{
    enum class ShaderType
    {
        Surface,
        CustomVertex,
        MeshShader,
        Compute
    };

    struct ShaderInfo
    {
        std::string vertexShader;
        std::string meshShader;
        std::string taskShader;
        std::string fragmentShader;
        std::string computeShader;
        ShaderType  type;
        RenderPassFlags renderPassFlags;

        GvkGraphicsPipelineCreateInfo::RasterizationStateCreateInfo raster;
        GvkGraphicsPipelineCreateInfo::FrameBufferBlendState        blendState;
        GvkGraphicsPipelineCreateInfo::DepthStencilStateInfo        depthStencilState;

        struct FrameBufferBlendState
        {
            GvkGraphicsPipelineCreateInfo::BlendState blendState;
            uint32_t                                  targetLocation;
        };
        std::vector<FrameBufferBlendState>            blendStateArray;


        std::vector<std::string> includeFileList;
    };

    /*
       custom vertex
       #pragma kbs_shader
       #pragma kbs_vertex_begin

       #pragma kbs_vertex_end

       #pragma kbs_fragment_begin
       
       #pragma kbs_fragment_end
    

       mesh shader
       #pragma kbs_shader
       #pragma kbs_task_begin

       #pragma kbs_task_end

       #pragma kbs_mesh_begin
       #pragma kbs_mesh_end

       #pragma kbs_fragment_begin
       #pragma kbs_fragment_end

       compute shader
       #pragma kbs_shader
       #pragma kbs_compute_begin

       #pragma kbs_compute_end


       #pragma zwrite off
       #pragma zwrite on
       #pragma include "..."
    */

    struct ShaderStandardVertex
    {
        vec3 inPos;
        vec3 inNormal;
        vec2 inUv;
        vec3 inTangent;
    };


    KBS_API class ShaderParser
    {
    public:
        static opt<ShaderInfo> Parse(const std::string& content, std::string* msg);
    };
}

