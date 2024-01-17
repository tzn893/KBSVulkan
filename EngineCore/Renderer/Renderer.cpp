#include "Renderer.h"
#include "Asset/AssetManager.h"
#include "Scene/Entity.h"
#include "Core/Singleton.h"

namespace kbs
{
    bool kbs::Renderer::Initialize(ptr<kbs::Window> window, RendererCreateInfo& info)
    {
        m_Window = window;
        std::string msg;
        if (auto ctx = gvk::Context::CreateContext(info.appName.c_str(), GVK_VERSION{ 1, 0, 0 }, VK_API_VERSION_1_3, window->GetGvkWindow(), &msg);
            ctx.has_value())
        {
            m_Context = ctx.value();
        }
        else
        {
            KBS_WARN("fail to create context for renderer reason : {}", msg.c_str());
            return false;
        }

        if (!m_Context->InitializeInstance(info.instance, &msg))
        {
            KBS_WARN("fail to initialize instance for renderer reason : {}", msg.c_str());
            return false;
        }

        if (info.device.required_queues.empty()) info.device.RequireQueue(VK_QUEUE_GRAPHICS_BIT, 1);
        info.device.AddDeviceExtension(GVK_DEVICE_EXTENSION_SWAP_CHAIN);

        if (!m_Context->InitializeDevice(info.device, &msg))
        {
            KBS_WARN("fail to initialize device for renderer reason : {}", msg.c_str());
            return false;
        }

        m_BackBufferFormat = m_Context->PickBackbufferFormatByHint({ VK_FORMAT_R8G8B8A8_UNORM,VK_FORMAT_R8G8B8A8_UNORM });
        if (!m_Context->CreateSwapChain(m_BackBufferFormat, &msg))
        {
            KBS_WARN("fail to create swap chain for renderer reason : {}", msg.c_str());
            return false;
        }

        m_Graph = std::make_shared<vkrg::RenderGraph>();

        if (!InitializeObjectDescriptorPool())
        {
            return false;
        }

        m_Graph = std::make_shared<vkrg::RenderGraph>();
        if (!InitRenderGraph(m_Window, m_Graph))
        {
            return false;
        }

        uint32_t backBufferCount = m_Context->GetBackBufferCount();
        for (uint32_t i = 0;i < backBufferCount;i++)
        {
            m_Fences.push_back(m_Context->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT).value());
        }

        m_PrimaryCmdQueue = m_Context->CreateQueue(VK_QUEUE_GRAPHICS_BIT).value();
        m_PrimaryCmdPool = m_Context->CreateCommandPool(m_PrimaryCmdQueue.get()).value();

        for (uint32_t i = 0;i < backBufferCount;i++)
        {
            m_PrimaryCmdBuffer.push_back(m_PrimaryCmdPool->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY).value());
        }
        for (uint32_t i = 0;i < backBufferCount;i++)
        {
            m_ColorOutputFinish.push_back(m_Context->CreateVkSemaphore().value());
        }

        // TODO better way initialize AssetManager::ShaderManager
        Singleton::GetInstance<AssetManager>()->GetShaderManager()->Initialize(m_Context);

        return true;
    }

    void kbs::Renderer::Destroy()
    {
        // TODO destroy all resources
        m_Graph = nullptr;
        m_Window = nullptr;
        uint32_t backBufferCnt = m_Context->GetBackBufferCount();
        m_PrimaryCmdPool = nullptr;
        m_PrimaryCmdQueue = nullptr;

        for (uint32_t i = 0;i < backBufferCnt;i++)
        {
            m_Context->DestroyFence(m_Fences[i]);
        }
        for (uint32_t i = 0; i < backBufferCnt; i++)
        {
            m_Context->DestroyVkSemaphore(m_ColorOutputFinish[i]);
        }
        m_Context = nullptr;
    }

    VkFormat Renderer::GetBackBufferFormat()
    {
        return m_BackBufferFormat;
    }

    RenderAPI Renderer::GetAPI()
    {
        KBS_ASSERT(m_Context != nullptr, "you can get render api only after renderer has been initialized");
        return RenderAPI(m_Context);
    }

    void Renderer::RenderSceneByCamera(ptr<Scene> scene, RenderCamera& camera, RenderFilter filter, VkCommandBuffer cmd)
    {
        AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
        m_CameraBuffer->GetBuffer()->Write(&camera.GetCameraUBO(), 0, sizeof(CameraUBO));

        struct RenderableObject
        {
            UUID id;
            RenderableComponent render;
            Transform           transform;
        };

        std::vector<RenderableObject> objects;
        auto sort_by_distance = [&](RenderableObject& lhs, RenderableObject& rhs)
        {
            if ((lhs.render.renderOptionFlags & RenderOption_DontCullByDistance) != (rhs.render.renderOptionFlags & RenderOption_DontCullByDistance))
            {
                return kbs_contains_flags(lhs.render.renderOptionFlags, RenderOption_DontCullByDistance);
            }

            vec3 cameraPos = camera.GetCameraTransform().GetPosition();
            return math::length(cameraPos - lhs.transform.GetPosition()) < math::length(cameraPos - rhs.transform.GetPosition());
        };
        auto sort_by_mesh = [&](RenderableObject& lhs, RenderableObject& rhs)
        {
            ShaderID lhsShaderID = GetMaterialByID(lhs.render.targetMaterial)->GetShader()->GetShaderID();
            ShaderID rhsShaderID = GetMaterialByID(rhs.render.targetMaterial)->GetShader()->GetShaderID();

            if(lhsShaderID != rhsShaderID)
            {
                return lhsShaderID < rhsShaderID;
            }
            if (lhs.render.targetMaterial != rhs.render.targetMaterial)
            {
                return lhs.render.targetMaterial < rhs.render.targetMaterial;
            }
            MeshGroupID lhsGroup = GetMeshGroupByComponent(lhs.render)->GetID();
            MeshGroupID rhsGroup = GetMeshGroupByComponent(rhs.render)->GetID();

            if (lhsGroup != rhsGroup)
            {
                return lhsGroup < rhsGroup;
            }
            
            return lhs.id < rhs.id;
        };

        scene->IterateAllEntitiesWith<RenderableComponent>
            (
                [&](Entity e)
                {
                    RenderableComponent render = e.GetComponent<RenderableComponent>();
                    ptr<Material>  mat = assetManager->GetMaterialManager()->GetMaterialByID(render.targetMaterial).value();

                    if (kbs_contains_flags(mat->GetRenderPassFlags(), filter.flags))
                    {
                        TransformComponent  trans = e.GetComponent<TransformComponent>();
                        IDComponent idcomp = e.GetComponent<IDComponent>();
                        objects.push_back(RenderableObject{ idcomp.ID, render, Transform(trans, e) });
                    }
                }
        );

        std::sort(objects.begin(), objects.end(), sort_by_distance);
        if (objects.size() > m_ObjectPoolSize)
        {
            objects.erase(objects.begin() + m_ObjectPoolSize, objects.end());
        }
        std::sort(objects.begin(), objects.end(), sort_by_mesh);

        ShaderID   bindedShaderID;
        MaterialID bindedMaterialID;
        MeshGroupID bindedMeshGroupID;
        
        for (uint32_t i = 0;i < objects.size();i++)
        {
            ObjectUBO objectUbo = objects[i].transform.GetObjectUBO();
            m_ObjectUBOPool->GetBuffer()->Write(&objectUbo, sizeof(ObjectUBO) * i, sizeof(ObjectUBO));

            ptr<Material> mat = GetMaterialByID(objects[i].render.targetMaterial);
            if (mat->GetShader()->GetShaderID() != bindedShaderID)
            {
                bindedShaderID = mat->GetShader()->GetShaderID();
                ptr<gvk::Pipeline> shaderPipeline = m_ShaderPipelines[bindedShaderID];
                GvkBindPipeline(cmd, shaderPipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    shaderPipeline->GetPipelineLayout(), (uint32_t)ShaderSetUsage::perCamera, 1, &m_CameraDescriptorSet, 0, NULL);
            }

            ptr<gvk::Pipeline>      materialPipeline = m_ShaderPipelines[bindedShaderID];
            if (objects[i].render.targetMaterial != bindedMaterialID)
            {
                bindedMaterialID = objects[i].render.targetMaterial;
                ptr<gvk::DescriptorSet> materialDescriptorSet = m_MaterialDescriptors[bindedMaterialID];

                if (materialDescriptorSet != nullptr)
                {
                    mat->UpdateDescriptorSet(m_Context, materialDescriptorSet);

                    GvkDescriptorSetBindingUpdate(cmd, materialPipeline)
                        .BindDescriptorSet(materialDescriptorSet)
                        .Update();
                }
            }

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                materialPipeline->GetPipelineLayout(), (uint32_t)ShaderSetUsage::perObject, 1, &m_ObjectDescriptorSetPool[i], 0, NULL);

            ptr<MeshGroup> meshGroup = GetMeshGroupByComponent(objects[i].render);
            if (meshGroup->GetID() != bindedMeshGroupID)
            {
                bindedMeshGroupID = meshGroup->GetID();
                meshGroup->BindVertexBuffer(cmd);
            }
            Mesh mesh = GetMeshByComponent(objects[i].render);
            mesh.Draw(cmd, 1);
        }

    }

    bool Renderer::InitializeMaterialPipelines()
    {
        View<ptr<Material>> mats = Singleton::GetInstance<AssetManager>()->GetMaterialManager()->GetMaterials();
        m_MaterialDescriptorAllocator = m_Context->CreateDescriptorAllocator();

        for (auto mat : mats)
        {
            ShaderID shaderID = mat->GetShader()->GetShaderID();
            ptr<gvk::Pipeline> pipeline;
            if (m_ShaderPipelines.count(shaderID))
            {
                pipeline = m_ShaderPipelines[shaderID];
            }
            else
            {
                auto targetPassHandle = GetMaterialTargetRenderPass(mat->GetRenderPassFlags());
                auto [targetPass, subPassIdx] = m_Graph->GetCompiledRenderPassAndSubpass(targetPassHandle);

                GvkGraphicsPipelineCreateInfo info;
                mat->GetShader()->OnPipelineStateCreate(info);
                info.subpass_index = subPassIdx;
                info.target_pass = targetPass;

                auto optPipeline = m_Context->CreateGraphicsPipeline(info);
                if (!optPipeline.has_value())
                {
                    KBS_WARN("fail to initialize renderer reason : fail to create pipeline state for material {}", mat->GetName().c_str());
                    return false;
                }
                pipeline = optPipeline.value();
                m_ShaderPipelines[shaderID] = pipeline;
            }
            auto layout = pipeline->GetInternalLayout((uint32_t)ShaderSetUsage::perMaterial);

            ptr<gvk::DescriptorSet> materialSet;
            if (layout.has_value())
            {
                materialSet = m_MaterialDescriptorAllocator->Allocate(layout.value()).value();
            }
            m_MaterialDescriptors[mat->GetID()] = materialSet;
        }
        return true;
    }

    bool Renderer::InitializeObjectDescriptorPool()
    {
        VkDevice device = m_Context->GetDevice();

        VkDescriptorPoolSize poolSizes[] = { VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_ObjectCameraPoolSize} };

        VkDescriptorPoolCreateInfo descPoolInfo{};
        descPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descPoolInfo.poolSizeCount = 1;
        descPoolInfo.pPoolSizes = poolSizes;
        descPoolInfo.maxSets = m_ObjectCameraPoolSize;
        descPoolInfo.flags = 0;

        if (VK_SUCCESS != vkCreateDescriptorPool(device, &descPoolInfo, NULL, &m_ObjectCameraDescriptorPool))
        {
            KBS_LOG("fail to initialize renderer reason : fail to initialize object descriptor pool");
            return  false;
        }

        // m_ObjectDescriptorSet
        {
            VkDescriptorSetLayoutBinding bindings[] = {
                VkDescriptorSetLayoutBinding
                {
                    0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                    NULL
                }
            };

            VkDescriptorSetLayoutCreateInfo descSetCI{};
            descSetCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descSetCI.bindingCount = 1;
            descSetCI.pBindings = bindings;
            descSetCI.flags = 0;

            vkCreateDescriptorSetLayout(device, &descSetCI, NULL, &m_ObjectDescriptorSetLayout);
            m_ObjectDescriptorSetPool.resize(m_ObjectPoolSize);

            std::vector<VkDescriptorSetLayout> layouts(m_ObjectPoolSize, m_ObjectDescriptorSetLayout);
            VkDescriptorSetAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc.descriptorSetCount = m_ObjectPoolSize;
            alloc.descriptorPool = m_ObjectCameraDescriptorPool;
            alloc.pSetLayouts = layouts.data();

            if (VK_SUCCESS != vkAllocateDescriptorSets(device, &alloc, m_ObjectDescriptorSetPool.data()))
            {
                KBS_WARN("fail to initialize renderer reason : fail to allocate descriptor sets for object pool");
                return false;
            }

            if (auto buf = m_Context->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(ObjectUBO) * m_ObjectPoolSize, GVK_HOST_WRITE_SEQUENTIAL);
                buf.has_value())
            {
                m_ObjectUBOPool = std::make_shared<RenderBuffer>(buf.value());
            }
            else
            {
                KBS_WARN("fail to initialize renderer reason : fail to allocate render buffer for object pool");
                return false;
            }

            std::vector<VkWriteDescriptorSet> writes(m_ObjectPoolSize, VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET });
            std::vector<VkDescriptorBufferInfo> buffers(m_ObjectPoolSize, VkDescriptorBufferInfo{});
            for (uint32_t i = 0; i < m_ObjectPoolSize; i++)
            {
                auto& write = writes[i];
                write.descriptorCount = 1;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.dstArrayElement = 0;
                write.dstBinding = 0;
                write.dstSet = m_ObjectDescriptorSetPool[i];

                auto& buffer = buffers[i];
                buffer.buffer = m_ObjectUBOPool->GetBuffer()->GetBuffer();
                buffer.offset = i * sizeof(ObjectUBO);
                buffer.range = sizeof(ObjectUBO);

                write.pBufferInfo = &buffers[i];
            }

            vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, NULL);
        }
        
        // m_CameraDescriptorSet
        {
            VkDescriptorSetLayoutBinding bindings[] = {
               VkDescriptorSetLayoutBinding
               {
                   0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                   1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                   NULL
               }
            };

            VkDescriptorSetLayoutCreateInfo descSetCI{};
            descSetCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descSetCI.bindingCount = 1;
            descSetCI.pBindings = bindings;
            descSetCI.flags = 0;

            vkCreateDescriptorSetLayout(device, &descSetCI, NULL, &m_CameraDescriptorSetLayout);

            VkDescriptorSetAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc.descriptorSetCount = 1;
            alloc.descriptorPool = m_ObjectCameraDescriptorPool;
            alloc.pSetLayouts = &m_CameraDescriptorSetLayout;


            if (VK_SUCCESS != vkAllocateDescriptorSets(device, &alloc, &m_CameraDescriptorSet))
            {
                KBS_WARN("fail to initialize renderer reason : fail to allocate descriptor sets for object pool");
                return false;
            }

            if (auto buf = m_Context->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(CameraUBO), GVK_HOST_WRITE_SEQUENTIAL);
                buf.has_value())
            {
                m_CameraBuffer = std::make_shared<RenderBuffer>(buf.value());
            }
            else
            {
                KBS_WARN("fail to initialize renderer reason : fail to allocate render buffer for object pool");
                return false;
            }

            VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            VkDescriptorBufferInfo buffer;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.dstArrayElement = 0;
            write.dstBinding = 0;
            write.dstSet = m_CameraDescriptorSet;

            buffer.buffer = m_CameraBuffer->GetBuffer()->GetBuffer();
            buffer.offset = 0;
            buffer.range = m_CameraBuffer->GetBuffer()->GetSize();

            write.pBufferInfo = &buffer;

            vkUpdateDescriptorSets(device, 1, &write, 0, NULL);
        }

        return true;
    }

    ptr<Material> Renderer::GetMaterialByID(MaterialID id)
    {
        auto mat = Singleton::GetInstance<AssetManager>()->GetMaterialManager()->GetMaterialByID(id);
        KBS_ASSERT(mat.has_value(), "id passed to this function must be a valid id");

        return mat.value();
    }

    ptr<MeshGroup> Renderer::GetMeshGroupByComponent(const RenderableComponent& comp)
    {
        ptr<MeshPool> pool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
        return pool->GetMeshGroup(pool->GetMesh(comp.targetMesh)->GetMeshGroupID()).value();
    }

    Mesh Renderer::GetMeshByComponent(const RenderableComponent& comp)
    {
        ptr<MeshPool> pool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
        return pool->GetMesh(comp.targetMesh).value();
    }

    void kbs::Renderer::RenderScene(ptr<Scene> scene)
    {
        // TODO better way to initialize materials
        static bool materialInitialized = false;
        if (!materialInitialized)
        {
            InitializeMaterialPipelines();
            materialInitialized = true;
        }

        uint32_t currentFrameIdx = m_Context->CurrentFrameIndex();
        uint32_t currentFenceIdx = 0;
        uint32_t currentSemaphoreIdx = 0;
        uint32_t currentCommandBufferIdx = 0;
        
        vkWaitForFences(m_Context->GetDevice(), 1, &m_Fences[currentFenceIdx], VK_TRUE, 0xffffffff);
        vkResetFences(m_Context->GetDevice(), 1, &m_Fences[currentFenceIdx]);

        VkCommandBuffer cmd = m_PrimaryCmdBuffer[currentCommandBufferIdx];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo cmdBeginInfo{};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = 0;
        vkBeginCommandBuffer(cmd, &cmdBeginInfo);

        auto onResize = [&](uint32_t w, uint32_t h)
        {
            // TODO resize window by resize event
            m_Graph->OnResize(w, h);
            return true;
        };

        std::string error;
        uint32_t currentImageIdx;
        VkSemaphore acquire_image_semaphore;
        if (auto v = m_Context->AcquireNextImageAfterResize(onResize, &error))
        {
            auto [_, a, i] = v.value();
            acquire_image_semaphore = a;
            currentImageIdx = i;
        }
        else
        {
            KBS_ASSERT(false, "error occurs while acquire next image from swap chain window error %s", error.c_str());
        }

        
        auto [state, msg] =  m_Graph->Execute(currentImageIdx, cmd);
        KBS_ASSERT(state == vkrg::RenderGraphRuntimeState::Success, "error occurs while excuting render graph {}", msg.c_str());

        VkSemaphore colorOutputFinish = m_ColorOutputFinish[currentCommandBufferIdx];

        vkEndCommandBuffer(cmd);

        m_PrimaryCmdQueue->Submit(&cmd, 1,
            gvk::SemaphoreInfo()
            .Wait(acquire_image_semaphore, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
            .Signal(colorOutputFinish),
            m_Fences[currentFenceIdx]);

        m_Context->Present(gvk::SemaphoreInfo().Wait(colorOutputFinish, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT));
    }
}

bool kbs::Renderer::CompileRenderGraph(std::string& msg)
{
    vkrg::RenderGraphCompileOptions options;
    options.disableFrameOnFlight = true;
    options.flightFrameCount = 3;
    options.screenHeight = m_Window->GetHeight();
    options.screenWidth = m_Window->GetWidth();

    vkrg::RenderGraphDeviceContext ctx;
    ctx.ctx = m_Context;

    auto [res, gMsg] = m_Graph->Compile(options, ctx);
    if (res != vkrg::RenderGraphCompileState::Success)
    {
        msg = "fail to compile renderer graph reason :" + gMsg;
        return false;
    }
    return true;
}

