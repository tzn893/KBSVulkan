#include "Renderer.h"
#include "Asset/AssetManager.h"
#include "Scene/Entity.h"
#include "Core/Singleton.h"

namespace kbs
{
    bool kbs::Renderer::Initialize(ptr<kbs::Window> window, RendererCreateInfo& info)
    {
        m_FrameCounter = 0;

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

        m_APIDescriptorSetAllocator = m_Context->CreateDescriptorAllocator();
		// TODO better way initialize AssetManager::ShaderManager
		Singleton::GetInstance<AssetManager>()->GetShaderManager()->Initialize(m_Context);

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
        return RenderAPI(m_Context, m_APIDescriptorSetAllocator);
    }

    RenderableObjectSorter Renderer::GetDefaultRenderableObjectSorter(vec3 cameraPosition)
    {
        auto sort_by_distance = [cameraPosition](RenderableObject& lhs, RenderableObject& rhs)
        {
            if ((lhs.targetRenderFlag & RenderOption_DontCullByDistance) != (rhs.targetRenderFlag & RenderOption_DontCullByDistance))
            {
                return kbs_contains_flags(lhs.targetRenderFlag, RenderOption_DontCullByDistance);
            }

            vec3 cameraPos = cameraPosition;
            return math::length(cameraPos - lhs.transform.GetPosition()) < math::length(cameraPos - rhs.transform.GetPosition());
        };
        auto sort_by_mesh = [&](RenderableObject& lhs, RenderableObject& rhs)
        {
            ShaderID lhsShaderID = GetMaterialByID(lhs.targetMaterial)->GetShader()->GetShaderID();
            ShaderID rhsShaderID = GetMaterialByID(rhs.targetMaterial)->GetShader()->GetShaderID();

            if (lhsShaderID != rhsShaderID)
            {
                return lhsShaderID < rhsShaderID;
            }
            if (lhs.targetMaterial != rhs.targetMaterial)
            {
                return lhs.targetMaterial < rhs.targetMaterial;
            }
            MeshGroupID lhsGroup = GetMeshGroupByMesh(lhs.targetMeshID)->GetID();
            MeshGroupID rhsGroup = GetMeshGroupByMesh(rhs.targetMeshID)->GetID();

            if (lhsGroup != rhsGroup)
            {
                return lhsGroup < rhsGroup;
            }

            return lhs.id < rhs.id;
        };
        auto defaultObjectSorter = [&](std::vector<RenderableObject>& objects)
        {
            std::sort(objects.begin(), objects.end(), sort_by_distance);
            if (objects.size() > m_ObjectPoolSize)
            {
                objects.erase(objects.begin() + m_ObjectPoolSize, objects.end());
            }
            std::sort(objects.begin(), objects.end(), sort_by_mesh);
        };
        return defaultObjectSorter;
    }

    RenderShaderFilter Renderer::GetDefaultShaderFilter()
    {
        return[](const ShaderID&) {return true; };
    }


    // CollectRenderableObjects() + RenderObjects()
    void Renderer::RenderSceneByCamera(ptr<Scene> scene, RenderCamera& camera, RenderFilter filter, VkCommandBuffer cmd)
    {
        AssetManager* assetManager = Singleton::GetInstance<AssetManager>();
        m_CameraBuffer->GetBuffer()->Write(&camera.GetCameraUBO(), 0, sizeof(CameraUBO));

        std::vector<RenderableObject> objects;
        

        if (filter.renderableObjectSorter == nullptr)
        {
            filter.renderableObjectSorter = GetDefaultRenderableObjectSorter(camera.GetCameraTransform().GetPosition());
        }
        if (filter.shaderFilter == nullptr)
        {
            filter.shaderFilter = GetDefaultShaderFilter();
        }

        scene->IterateAllEntitiesWith<RenderableComponent>
            (
                [&](Entity e)
                {
                    RenderableComponent render = e.GetComponent<RenderableComponent>();

                    for (uint32_t i = 0;i < render.passCount;i++)
                    {
                        ptr<Material>  mat = assetManager->GetMaterialManager()->GetMaterialByID(render.targetMaterials[i]).value();

                        if (kbs_contains_flags(mat->GetRenderPassFlags(), filter.flags) && filter.shaderFilter(mat->GetShader()->GetShaderID()))
                        {
                            TransformComponent  trans = e.GetComponent<TransformComponent>();
                            IDComponent idcomp = e.GetComponent<IDComponent>();
                            objects.push_back(RenderableObject{ idcomp.ID, render.targetMaterials[i], render.renderOptionFlags[i], render.targetMesh, Transform(trans, e)});
                        }
                    }
                }
        );

        filter.renderableObjectSorter(objects);
        KBS_ASSERT(objects.size() <= m_ObjectPoolSize);

        ShaderID   bindedShaderID;
        MaterialID bindedMaterialID;
        MeshGroupID bindedMeshGroupID;
        
        for (uint32_t i = 0;i < objects.size();i++)
        {
            ObjectUBO objectUbo = objects[i].transform.GetObjectUBO();
            m_ObjectUBOPool->GetBuffer()->Write(&objectUbo, sizeof(ObjectUBO) * i, sizeof(ObjectUBO));

            ptr<Material> mat = GetMaterialByID(objects[i].targetMaterial);
            if (mat->GetShader()->GetShaderID() != bindedShaderID)
            {
                bindedShaderID = mat->GetShader()->GetShaderID();
                ptr<gvk::Pipeline> shaderPipeline = m_ShaderPipelines[bindedShaderID];
                GvkBindPipeline(cmd, shaderPipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    shaderPipeline->GetPipelineLayout(), (uint32_t)ShaderSetUsage::perCamera, 1, &m_CameraDescriptorSet, 0, NULL);
            }

            ptr<gvk::Pipeline>      materialPipeline = m_ShaderPipelines[bindedShaderID];
            if (objects[i].targetMaterial != bindedMaterialID)
            {
                bindedMaterialID = objects[i].targetMaterial;
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

            ptr<MeshGroup> meshGroup = GetMeshGroupByMesh(objects[i].targetMeshID);
            if (meshGroup->GetID() != bindedMeshGroupID)
            {
                bindedMeshGroupID = meshGroup->GetID();
                meshGroup->BindVertexBuffer(cmd);
            }
            Mesh mesh = GetMeshByMeshID(objects[i].targetMeshID);
            mesh.Draw(cmd, 1);
        }

    }

	uint32_t Renderer::GetCurrentFrameIdx()
	{
        return m_FrameCounter;
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

    ptr<MeshGroup> Renderer::GetMeshGroupByMesh(const MeshID& id)
    {
        ptr<MeshPool> pool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
        return pool->GetMeshGroup(pool->GetMesh(id)->GetMeshGroupID()).value();
    }

    Mesh Renderer::GetMeshByMeshID(const MeshID& id)
    {
        ptr<MeshPool> pool = Singleton::GetInstance<AssetManager>()->GetMeshPool();
        return pool->GetMesh(id).value();
    }

    void kbs::Renderer::RenderScene(ptr<Scene> scene)
    {
        // TODO better way to initialize materials
        OnSceneRender(scene);

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
        m_FrameCounter++;
    }

    void RendererPass::SetTargetCamera(const RenderCamera& camera)
    {
        m_Camera = camera;
    }

    void RendererPass::SetTargetScene(ptr<Scene> scene)
    {
        m_TargetScene = scene;
    }

    ptr<vkrg::RenderPass> RendererPass::InitializePass(Renderer* renderer, ptr<gvk::Context> ctx,
        ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc,
        const std::string& name)
    {
        auto passHandle = graph->AddGraphRenderPass(name.c_str(), vkrg::RenderPassType::Graphics).value();
        m_RenderPassHandle = passHandle;

        Initialize(passHandle.pass, desc, ctx);

        m_Renderer = renderer;
        m_Context = ctx;

        return passHandle.pass;
    }

    void RendererPass::OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
    {
        if (m_PerdrawDescriptorSet != nullptr)
        {
            GvkDescriptorSetBindingUpdate(cmd, m_PerdrawDummyPipeline)
                .BindDescriptorSet(m_PerdrawDescriptorSet)
                .Update();
        }
       
        OnSceneRender(ctx, cmd, m_Camera, m_TargetScene);
    }

    vkrg::RenderPassHandle RendererPass::GetRenderPassHandle()
    {
        return m_RenderPassHandle;
    }

    RendererPass::PassParameterUpdater RendererPass::UpdatePassParameter()
    {
        RendererPass::PassParameterUpdater updater(m_Context, m_PerdrawDescriptorSet, m_Renderer->GetAPI());
        return updater;
    }



    void RendererPass::SetUpPerpassParameterSet(ShaderID targetShaderID)
    {
        ptr<ShaderManager> shaderManager = Singleton::GetInstance<AssetManager>()->GetShaderManager();
        ptr<Shader> shader = shaderManager->Get(targetShaderID).value();

        KBS_ASSERT(shader->IsGraphicsShader(), "the Perpass parameter set can only be initialized by graphics shader");
        ptr<GraphicsShader> gShader = std::dynamic_pointer_cast<GraphicsShader>(shader);

        GvkGraphicsPipelineCreateInfo pipelineCI;
        gShader->OnPipelineStateCreate(pipelineCI);
        
        m_PerdrawDummyPipeline = m_Context->CreateGraphicsPipeline(pipelineCI).value();
        auto setLayout = m_PerdrawDummyPipeline->GetInternalLayout((uint32_t)ShaderSetUsage::perDraw);
        KBS_ASSERT(setLayout.has_value(), "the per draw set must be used by target shader");

        m_PerdrawDescriptorSet = m_Renderer->GetAPI().AllocateDescriptorSet(setLayout.value());
    }

    void RendererPass::RenderSceneByCamera(RenderFilter filter, VkCommandBuffer cmd)
    {
        m_Renderer->RenderSceneByCamera(m_TargetScene, m_Camera, filter, cmd);
    }


    void RendererPass::PassParameterUpdater::UpdateImage(const std::string& name,VkImageView view, const std::string& samplerName, VkImageLayout layout)
    {   
        auto sampler = api.CreateSamplerByName(samplerName);
        KBS_ASSERT(sampler.has_value(), "invalid sampler name : {}", samplerName.c_str());
        UpdateImage(name, view, sampler.value(), layout);
    }

    void RendererPass::PassParameterUpdater::UpdateImage(const std::string& name, VkImageView view, VkSampler sampler, VkImageLayout layout)
    {
        GvkDescriptorSetWrite()
            .ImageWrite(set, name.c_str(), sampler, view, layout)
        .Emit(ctx->GetDevice());
    }

	ptr<vkrg::RenderPass> ComputePass::InitializePass(Renderer* render, ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc, const std::string& name)
	{
        m_Renderer = render;

        auto passHandle = graph->AddGraphRenderPass(name.c_str(), vkrg::RenderPassType::Compute);
        KBS_ASSERT(passHandle.has_value(), "fail to add compute pass {} to render graph", name.c_str());
        m_ComputePassHandle = passHandle.value();

        Initialize(m_ComputePassHandle.pass, desc);

        return m_ComputePassHandle.pass;
	}

	vkrg::RenderPassHandle ComputePass::GetRenderPassHandle()
	{
        return m_ComputePassHandle;
	}

	void ComputePass::OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
        OnDispatch(ctx, cmd);
	}

	kbs::RenderAPI ComputePass::GetRenderAPI()
	{
        return m_Renderer->GetAPI();
	}

	kbs::ptr<vkrg::RenderPass> RayTracingPass::InitializePass(Renderer* render, ptr<vkrg::RenderGraph> graph, RendererAttachmentDescriptor& desc, const std::string& name)
	{
		m_Renderer = render;

		auto passHandle = graph->AddGraphRenderPass(name.c_str(), vkrg::RenderPassType::Raytracing);
		KBS_ASSERT(passHandle.has_value(), "fail to add ray tracing pass {} to render graph", name.c_str());
		m_RTPassHandle = passHandle.value();

		Initialize(m_RTPassHandle.pass, desc);

		return m_RTPassHandle.pass;
	}

	vkrg::RenderPassHandle RayTracingPass::GetRenderPassHandle()
	{
        return m_RTPassHandle;
	}

	void RayTracingPass::OnRender(vkrg::RenderPassRuntimeContext& ctx, VkCommandBuffer cmd)
	{
        OnDispatch(ctx, cmd);
	}

	kbs::RenderAPI RayTracingPass::GetRenderAPI()
	{
        return m_Renderer->GetAPI();
	}

}

bool kbs::Renderer::CompileRenderGraph(std::string& msg)
{
    vkrg::RenderGraphCompileOptions options;
    options.disableFrameOnFlight = true;
    options.flightFrameCount = 3;
    options.screenHeight = m_Window->GetHeight();
    options.screenWidth = m_Window->GetWidth();
    options.setDebugName = true;

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



