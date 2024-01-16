#include "RenderResource.h"

kbs::RenderBuffer::RenderBuffer(ptr<gvk::Buffer> buffer)
    :buffer(buffer)
{
}

kbs::ptr<gvk::Buffer> kbs::RenderBuffer::GetBuffer()
{
    return buffer;
}

kbs::Texture::Texture(ptr<gvk::Image> image, VkSampler sampler, VkImageView view)
    :m_Image(image), m_Sampler(sampler), m_MainView(view)
{

}

kbs::ptr<gvk::Image> kbs::Texture::GetImage()
{
    return m_Image;
}

VkSampler kbs::Texture::GetSampler()
{
    return m_Sampler;
}

VkImageView kbs::Texture::GetMainView()
{
    return m_MainView;
}
