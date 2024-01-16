#include "RenderAPI.h"

namespace kbs 
{
    RenderAPI::RenderAPI(ptr<gvk::Context> ctx)
    {
        m_Ctx = ctx;
    }

    ptr<RenderBuffer> kbs::RenderAPI::CreateBuffer(VkBufferUsageFlags usage, uint32_t size, GVK_HOST_WRITE_PROPERTY prop)
    {
        ptr<gvk::Buffer> buffer = m_Ctx->CreateBuffer(usage, size, prop).value();

        return std::make_shared<RenderBuffer>(buffer);
    }

}

