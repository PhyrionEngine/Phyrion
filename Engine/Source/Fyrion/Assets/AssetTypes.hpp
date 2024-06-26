#pragma once

#include "Fyrion/Common.hpp"

namespace Fyrion
{
    struct UIFont
    {
        constexpr static u32 FontBytes = 0;
    };

    struct ImageAsset
    {
        constexpr static u32 Extent = 0;
        constexpr static u32 Channels = 1;
        constexpr static u32 Data = 2;
    };

    struct ShaderAsset
    {
        constexpr static u32 Bytes = 0;
        constexpr static u32 Info = 1;
        constexpr static u32 Stages = 2;
    };


    struct GraphNodeValue
    {
        constexpr static u32 Name = 0;
        constexpr static u32 PublicValue = 1;
        constexpr static u32 Type = 2;
        constexpr static u32 ResourceType = 3;
        constexpr static u32 Value = 4;
    };


    struct GraphNodeInput
    {
        constexpr static u32 Name = 0;
        constexpr static u32 Value = 1;
    };

    struct GraphNodeAsset
    {
        constexpr static u32 NodeFunction = 0;
        constexpr static u32 NodeOutput = 1;
        constexpr static u32 Position = 2;
        constexpr static u32 InputValues = 3;
        constexpr static u32 Label = 4;
    };

    struct GraphNodeLinkAsset
    {
        constexpr static u32 InputNode = 0;
        constexpr static u32 InputPin = 1;
        constexpr static u32 OutputNode = 2;
        constexpr static u32 OutputPin = 3;
    };

    struct GraphAsset
    {
        constexpr static u32 Nodes = 0;
        constexpr static u32 Links = 1;
    };

    struct GraphAssetInstance
    {
        constexpr static u32 Graph = 0;
        constexpr static u32 Inputs = 0;
    };

    struct DCCMesh
    {

    };
}
