#pragma once

#include "Fyrion/Graphics/GraphicsTypes.hpp"

namespace Fyrion
{
    class VulkanDevice;

    struct VulkanCommands : RenderCommands
    {
        VulkanDevice& vulkanDevice;
        VulkanCommands(VulkanDevice& vulkanDevice);

        void BeginRenderPass(const BeginRenderPassInfo& beginRenderPassInfo) override;
        void EndRenderPass() override;
        void SetViewport(const ViewportInfo& viewportInfo) override;
        void BindVertexBuffer(const GPUBuffer& gpuBuffer) override;
        void BindIndexBuffer(const GPUBuffer& gpuBuffer) override;
        void DrawIndexed(u32 indexCount, u32 instanceCount, u32 firstIndex, i32 vertexOffset, u32 firstInstance) override;
        void Draw(u32 vertexCount, u32 instanceCount, u32 firstVertex, u32 firstInstance) override;
        void PushConstants(const PipelineState& pipeline, ShaderStage stages, const void* data, usize size) override;
        void BindBindingSet(const PipelineState& pipeline, const BindingSet& bindingSet) override;
        void DrawIndexedIndirect(const GPUBuffer& buffer, usize offset, u32 drawCount, u32 stride) override;
        void BindPipelineState(const PipelineState& pipeline) override;
        void Dispatch(u32 x, u32 y, u32 z) override;
        void TraceRays(PipelineState pipeline, u32 x, u32 y, u32 z) override;
        void SetScissor(const Rect& rect) override;
        void BeginLabel(const StringView& name, const Vec4& color) override;
        void EndLabel() override;
        void ResourceBarrier(const ResourceBarrierInfo& resourceBarrierInfo) override;
    };
}