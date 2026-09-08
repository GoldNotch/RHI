#include "Framebuffer.hpp"

#include <format>

#include <Attachments/SurfacedAttachment.hpp>
#include <Utils/CastHelper.hpp>
#include <VulkanContext.hpp>

namespace
{

bool operator==(const VkExtent3D & e1, const VkExtent3D & e2)
{
  return std::memcmp(&e1, &e2, sizeof(VkExtent3D)) == 0;
}

} // namespace

namespace RHI::vulkan
{

Framebuffer::Framebuffer(Context & ctx)
  : OwnedBy<Context>(ctx)
  , m_renderPass(ctx, *this)
{
}

Framebuffer::~Framebuffer()
{
}

size_t Framebuffer::GetImagesCount() const noexcept
{
  return m_targets.size();
}

void Framebuffer::Invalidate()
{
  bool targetsChanged = false;
  //rebuild attachments
  if (m_attachmentsChanged)
  {
    if (m_attachments.empty())
      throw std::runtime_error("Framebuffer has no attachments");

    std::vector<VkImageUsageFlags> attachmentsUsage;
    attachmentsUsage.resize(m_attachments.size(), 0);
    m_renderPass.CollectAttachmentsUsageInfo(attachmentsUsage);

    std::vector<VkAttachmentDescription> newAttachmentsDescription;
    newAttachmentsDescription.reserve(m_attachments.size());
    for (size_t i = 0; auto * attachment : m_attachments)
    {
      if (attachment)
      {
        attachment->Invalidate(attachmentsUsage[i]);
        newAttachmentsDescription.push_back(attachment->BuildDescription());
      }
      ++i;
    }

    m_attachmentDescriptions = std::move(newAttachmentsDescription);

    uint32_t buffersCount = m_attachments[0]->GetBuffering();
    auto extent = m_attachments[0]->GetInternalExtent();
    // all attachments must have equal count of buffers
    assert(std::all_of(m_attachments.begin(), m_attachments.end(),
                       [buffersCount, extent](IInternalAttachment * att)
                       {
                         return buffersCount == att->GetBuffering() &&
                                att->GetInternalExtent() == extent;
                       }));

    // set attachments to render Pass
    m_renderPass.SetAttachments(buffersCount, m_attachmentDescriptions);
    targetsChanged = true;
  }

  //rebuild render pass
  m_renderPass.Invalidate();

  //rebuild RenderTarget(VkFramebuffer)
  if (targetsChanged)
  {
    uint32_t buffersCount = m_attachments[0]->GetBuffering();
    auto extent = m_attachments[0]->GetInternalExtent();
    if (m_targets.size() != buffersCount)
    {
      while (m_targets.size() > buffersCount)
        m_targets.pop_back();

      while (m_targets.size() < buffersCount)
        m_targets.emplace_back(GetContext());
    }

    // build RenderTargets
    for (auto && target : m_targets)
    {
      target.SetExtent(extent);
      target.BindRenderPass(m_renderPass.GetHandle());
    }
    targetsChanged = false;
  }
}

void Framebuffer::ForEachAttachment(AttachmentProcessFunc && func)
{
  std::for_each(m_attachments.begin(), m_attachments.end(), func);
}

IInternalAttachment * Framebuffer::GetAttachment(uint32_t idx) const
{
  return m_attachments[idx];
}

RHI::SamplesCount Framebuffer::CalcSamplesCount() const noexcept
{
  // find first not SurfacedAttachment and return it's value
  for (auto * attachment : m_attachments)
  {
    if (dynamic_cast<GenericAttachment *>(attachment) != nullptr)
      return attachment->GetSamplesCount();
  }
  return RHI::SamplesCount::One;
}

RenderTarget * Framebuffer::BeginFrame()
{
  if (m_attachments.empty())
    return nullptr;

  std::vector<VkImageView> renderingImages;
  std::vector<VkSemaphore> semaphores;
  std::vector<VkClearValue> clearValues;
  renderingImages.reserve(m_attachments.size());
  semaphores.reserve(m_attachments.size());
  clearValues.reserve(m_attachments.size());
  bool success = true;

  auto processAttachment =
    [&renderingImages, &semaphores, &clearValues, &success](IInternalAttachment * attachment)
  {
    if (attachment && success)
    {
      auto [imageView, imgAvailSemaphore] = attachment->AcquireForRendering();
      if (!imageView)
        success = false;
      if (imgAvailSemaphore)
        semaphores.push_back(imgAvailSemaphore);
      renderingImages.push_back(imageView);
      clearValues.push_back(attachment->GetClearValue());
    }
  };

  std::for_each(m_attachments.begin(), m_attachments.end(), processAttachment);
  if (!success)
  {
    m_attachmentsChanged = true;
    return nullptr;
  }

  m_activeTarget = (m_activeTarget + 1) % m_targets.size();
  //AcquireForRendering can return random imageView set, so probably it could rebuild VkFramebuffer for each frame
  m_targets[m_activeTarget].SetAttachments(std::move(renderingImages), std::move(clearValues),
                                           std::move(semaphores));
  m_targets[m_activeTarget].Invalidate(); // rebuilds VkFramebuffer if need it
  return &m_targets[m_activeTarget];
}

void Framebuffer::RecordCommands(details::CommandBuffer & commands)
{
  m_renderPass.RecordCommands(commands, m_targets[m_activeTarget]);
}

void Framebuffer::CollectResources(std::vector<ResourcePtr> & resources) const
{
  m_renderPass.CollectResources(resources);
}

void Framebuffer::SynchroniseResources(details::CommandBuffer & commands) const
{
  m_renderPass.SynchroniseResources(commands);
}

void Framebuffer::EndFrame(VkSemaphore renderPassSemaphore)
{
  for (auto && attachment : m_attachments)
  {
    if (attachment)
      attachment->FinalRendering(renderPassSemaphore);
  }
}

void Framebuffer::SetSubpass(uint32_t index, PipelinePtr pipeline, PipelineProcessPtr process)
{
  m_renderPass.SetSubpass(index, std::move(pipeline), std::move(process));
}

void Framebuffer::AddAttachment(uint32_t binding, IAttachment * attachment)
{
  while (m_attachments.size() < binding + 1)
  {
    m_attachments.push_back(nullptr);
  }

  if (IInternalAttachment * ptr = dynamic_cast<IInternalAttachment *>(attachment))
  {
    //ptr->SetBuffering(m_framesCount);
    m_attachments[binding] = ptr;
    m_attachmentsChanged = true;
  }
  else
  {
    throw std::runtime_error("Failed to cast ITexture * to IInternalAttachment *");
  }
}

void Framebuffer::ClearAttachments() noexcept
{
  m_attachments.clear();
  m_attachmentsChanged = true;
}

void Framebuffer::Resize(uint32_t width, uint32_t height)
{
  for (auto * attachment : m_attachments)
    if (attachment)
      attachment->Resize(VkExtent2D(width, height));
  //TODO: return
  //m_renderPass.ForEachSubpass([](SubpassLayout & sp) { sp.SetDirtyCommands(); });
  m_attachmentsChanged = true;
}

RHI::TexelIndex Framebuffer::GetExtent() const
{
  auto internalExtent = m_attachments[0]->GetInternalExtent();
  return {static_cast<texel_t>(internalExtent.width), static_cast<texel_t>(internalExtent.height),
          static_cast<texel_t>(internalExtent.depth)};
}

} // namespace RHI::vulkan
