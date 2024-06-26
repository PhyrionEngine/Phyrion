#include "ResourceGraph.hpp"

#include "Repository.hpp"
#include "Fyrion/Assets/AssetTypes.hpp"
#include "Fyrion/Core/Graph.hpp"
#include "Fyrion/Core/GraphTypes.hpp"
#include "Fyrion/Core/Logger.hpp"

namespace Fyrion
{
    namespace
    {
        Logger& logger = Logger::GetLogger("Fyrion::ResourceGraph");
    }

    ResourceGraphNodeData::ResourceGraphNodeData(ResourceGraph* resourceGraph, const ResourceGraphNodeInfo& info) :
        m_resourceGraph(resourceGraph),
        m_info(info),
        m_id(info.id),
        m_functionHandler(info.functionHandler),
        m_typeHandler(info.typeHandler)
    {
    }

    ResourceGraphInstance::ResourceGraphInstance(ResourceGraph* resourceGraph) : m_resourceGraph(resourceGraph)
    {
        m_instanceData = (CharPtr)m_resourceGraph->m_allocator.MemAlloc(m_resourceGraph->m_instanceAllocRequiredSize, 1);

        for (auto& data : m_resourceGraph->m_data)
        {
            if (!data.offsetFromInstance && data.defaultValue == nullptr && data.typeHandler && data.offset != U32_MAX)
            {
                data.typeHandler->Construct(&m_instanceData[data.offset]);
            }
        }

        m_nodeParams.Resize(m_resourceGraph->m_nodes.Size());

        for (int n = 0; n < m_resourceGraph->m_nodes.Size(); ++n)
        {
            const auto& node = m_resourceGraph->m_nodes[n];
            if (node->m_valid)
            {
                auto& funcParams = m_nodeParams[n];
                if (node->m_functionHandler)
                {
                    FY_ASSERT(node->m_offsets.Size() == node->m_functionHandler->GetParams().Size(), "something went wrong");
                    funcParams.Reserve(node->m_offsets.Size());

                    Span<ParamHandler> params = node->m_info.functionHandler->GetParams();

                    for (u32 p = 0; p < params.Size(); ++p)
                    {
                        ParamHandler& param = params[p];
                        if (const auto it = node->m_offsets.Find(param.GetName()))
                        {
                            auto& data = m_resourceGraph->m_data[it->second];
                            funcParams[p] = data.defaultValue == nullptr ? &m_instanceData[data.offset] : data.defaultValue;
                        }
                    }
                }
                else if (node->m_outputOffset != U32_MAX && node->m_typeHandler != nullptr)
                {
                    //type constructor
                    node->m_typeHandler->Construct(&m_instanceData[node->m_outputOffset]);

                    //publish output.
                    auto it = m_outputs.Find(node->m_typeHandler->GetTypeInfo().typeId);
                    if (it == m_outputs.end())
                    {
                        it = m_outputs.Insert(node->m_typeHandler->GetTypeInfo().typeId, {}).first;
                    }
                    it->second.EmplaceBack(&m_instanceData[node->m_outputOffset]);
                }
            }
        }
    }

    void ResourceGraphInstance::Execute()
    {
        for (int n = 0; n < m_resourceGraph->m_nodes.Size(); ++n)
        {
            const auto& node = m_resourceGraph->m_nodes[n];
            if (node->m_valid)
            {
                if (node->m_functionHandler)
                {
                    node->m_functionHandler->Invoke(nullptr, nullptr, m_nodeParams[n].Data());
                }
                else if (node->m_typeHandler)
                {
                    //copy links to type.
                    for(auto& itOffset: node->m_offsets)
                    {
                        auto& data = m_resourceGraph->m_data[itOffset.second];
                        if (data.copyOffset != U32_MAX)
                        {
                            data.typeHandler->Copy(&m_instanceData[data.copyOffset], &m_instanceData[data.offset]);
                        }
                    }
                }
            }
        }
    }

    void ResourceGraphInstance::SetInputValue(const StringView& inputName, ConstPtr data)
    {
        if (auto it = m_resourceGraph->m_publicInputs.Find(inputName))
        {
            auto& nodeData = m_resourceGraph->m_data[it->second];
            nodeData.typeHandler->Copy(data, &m_instanceData[nodeData.offset]);
        }
    }

    Span<ConstPtr> ResourceGraphInstance::GetOutputs(TypeID typeId) const
    {
        if (const auto it = m_outputs.Find(typeId))
        {
            return it->second;
        }
        return {};
    }

    void ResourceGraphInstance::Destroy()
    {
        for (auto& data : m_resourceGraph->m_data)
        {
            if (!data.offsetFromInstance && data.defaultValue == nullptr && data.typeHandler && data.offset != U32_MAX)
            {
                data.typeHandler->Destructor(&m_instanceData[data.offset]);
            }
        }

        for (auto& node : m_resourceGraph->m_nodes)
        {
            if (node->m_typeHandler && node->m_outputOffset != U32_MAX)
            {
                node->m_typeHandler->Destructor(&m_instanceData[node->m_outputOffset]);
            }
        }

        m_resourceGraph->m_allocator.MemFree(m_instanceData);
        m_resourceGraph->m_allocator.DestroyAndFree(this);
    }

    //***************************************************************ResourceGraph*************************************************************

    void ResourceGraph::SetGraph(const Span<ResourceGraphNodeInfo>& nodes, const Span<ResourceGraphLinkInfo>& links)
    {
        m_links = links;

        Graph<u32, SharedPtr<ResourceGraphNodeData>> graph{};

        for (const ResourceGraphNodeInfo& info : nodes)
        {
            SharedPtr<ResourceGraphNodeData> node = MakeShared<ResourceGraphNodeData>(this, info);
            graph.AddNode(info.id, node);
        }

        for (const ResourceGraphLinkInfo& link : links)
        {
            SharedPtr<ResourceGraphNodeData> inputNode = graph.GetNode(link.inputNodeId);
            inputNode->m_inputLinks.Insert(link.inputPin, link);
            graph.AddEdge(link.inputNodeId, link.outputNodeId);
        }

        m_nodes = graph.Sort();


        for (auto& node : m_nodes)
        {
            for (auto& nodeValue : node->m_info.values)
            {
                if (!node->m_inputLinks.Has(nodeValue.name))
                {
                    usize offset = m_data.Size();
                    node->m_offsets.Insert(nodeValue.name, offset);

                    ResourceGraphNodeParamData& data = m_data.EmplaceBack(ResourceGraphNodeParamData{
                        .offset = m_instanceAllocRequiredSize,
                        .typeHandler = nodeValue.typeHandler,
                    });
                    m_instanceAllocRequiredSize += nodeValue.typeHandler->GetTypeInfo().size;

                    if (nodeValue.value != nullptr)
                    {
                        data.defaultValue = nodeValue.typeHandler->NewInstance(m_allocator);
                        nodeValue.typeHandler->Copy(nodeValue.value, data.defaultValue);
                    }

                    if (nodeValue.publicValue)
                    {
                        m_publicInputs.Insert(nodeValue.name, offset);
                    }
                }
            }

            if (node->m_info.functionHandler)
            {
                Span<ParamHandler> params = node->m_info.functionHandler->GetParams();

                for (int i = 0; i < params.Size(); ++i)
                {
                    const ParamHandler& param = params[i];

                    if (param.HasAttribute<GraphInput>())
                    {
                        if (auto it = node->m_inputLinks.Find(param.GetName()))
                        {
                            if (auto itOffset = m_nodes[it->second.outputNodeId]->m_offsets.Find(it->second.outputPin))
                            {
                                //TODO - validate types.
                                // TypeHandler* paramType = Registry::FindTypeById(param.GetFieldInfo().typeInfo.typeId);
                                // FY_ASSERT(m_data[itOffset->second].typeHandler->GetTypeInfo().typeId == paramType->GetTypeInfo().typeId, "different types");
                                node->m_offsets.Insert(param.GetName(), itOffset->second);
                            }
                        }
                    }
                    else if (param.HasAttribute<GraphOutput>())
                    {
                        TypeHandler* paramType = Registry::FindTypeById(param.GetFieldInfo().typeInfo.typeId);
                        FY_ASSERT(paramType, "type not found");
                        if (paramType == nullptr)
                        {
                            break;
                        }

                        node->m_offsets.Insert(param.GetName(), m_data.Size());

                        m_data.EmplaceBack(ResourceGraphNodeParamData{
                            .offset = m_instanceAllocRequiredSize,
                            .typeHandler = paramType
                        });
                        m_instanceAllocRequiredSize += paramType->GetTypeInfo().size;
                    }
                    else
                    {
                        FY_ASSERT(false, "param should have GraphInput or GraphOutput");
                    }

                    if (!node->m_offsets.Has(param.GetName()))
                    {
                        node->m_valid = false;
                    }
                }
            }
            else if (node->m_typeHandler)
            {
                // && node->m_typeHandler->HasAttribute<ResourceGraphOutput>()

                TypeInfo typeInfo = node->m_info.typeHandler->GetTypeInfo();

                node->m_outputOffset = m_instanceAllocRequiredSize;
                m_instanceAllocRequiredSize += typeInfo.size;

                auto itOutput = m_outputs.Find(typeInfo.typeId);
                if (itOutput == m_outputs.end())
                {
                    itOutput = m_outputs.Insert(typeInfo.typeId, {}).first;
                }

                itOutput->second.EmplaceBack(node->m_outputOffset);

                for (FieldHandler* fieldHandler : node->m_info.typeHandler->GetFields())
                {
                    if (fieldHandler->HasAttribute<GraphInput>())
                    {
                        FieldInfo fieldInfo = fieldHandler->GetFieldInfo();

                        TypeHandler* fieldType = Registry::FindTypeById(fieldInfo.typeInfo.typeId);
                        FY_ASSERT(fieldType, "type not found");
                        if (fieldType == nullptr)
                        {
                            break;
                        }

                        node->m_offsets.Insert(fieldHandler->GetName(), m_data.Size());

                        //TODO don't need to c construct.
                        auto& data = m_data.EmplaceBack(ResourceGraphNodeParamData{
                            .offset = node->m_outputOffset + (u32)fieldInfo.offsetOf,
                            .typeHandler = fieldType,
                            .offsetFromInstance = true
                        });

                        if (auto it = node->m_inputLinks.Find(fieldHandler->GetName()))
                        {
                            if (auto itOffset = m_nodes[it->second.outputNodeId]->m_offsets.Find(it->second.outputPin))
                            {
                                data.copyOffset = m_data[itOffset->second].offset;
                            }
                        }
                    }
                }
            }
        }
    }

    void ResourceGraph::SetGraph(RID assetGraph)
    {
        FY_ASSERT(m_nodes.Empty(), "Graph is not empty");

        ResourceObject graphAsset = Repository::Read(assetGraph);

        Array<RID> nodes = graphAsset.GetSubObjectSetAsArray(GraphAsset::Nodes);
        Array<RID> links = graphAsset.GetSubObjectSetAsArray(GraphAsset::Links);

        Array<ResourceGraphNodeInfo> nodesInfo;
        Array<ResourceGraphLinkInfo> nodeLinks;

        nodesInfo.Reserve(nodes.Size());
        nodeLinks.Reserve(links.Size());

        HashMap<RID, u32> nodeIds;
        u32 idCount = 0;

        for (const RID& node : nodes)
        {
            nodeIds[node] = idCount;

            ResourceGraphNodeInfo nodeInfo;
            nodeInfo.id = idCount;

            ResourceObject nodeObject = Repository::Read(node);
            if (nodeObject.Has(GraphNodeAsset::NodeFunction))
            {
                nodeInfo.functionHandler = Registry::FindFunctionByName(nodeObject[GraphNodeAsset::NodeFunction].As<String>());
            }
            else if (nodeObject.Has(GraphNodeAsset::NodeOutput))
            {
                nodeInfo.typeHandler = Registry::FindTypeByName(nodeObject[GraphNodeAsset::NodeOutput].As<String>());
            }

            Array<RID> inputValues = nodeObject.GetSubObjectSetAsArray(GraphNodeAsset::InputValues);

            for (const RID& inputValue : inputValues)
            {
                ResourceObject valueObject = Repository::Read(inputValue);

                RID valueSuboject = valueObject.GetSubObject(GraphNodeValue::Value);
                nodeInfo.values.EmplaceBack(ResourceGraphNodeValue{
                    .name = valueObject[GraphNodeValue::Name].Value<String>(),
                    .typeHandler = Repository::GetResourceTypeHandler(valueSuboject),
                    .value = Repository::ReadData(valueSuboject),
                    .publicValue = valueObject[GraphNodeValue::PublicValue].Value<bool>(),
                });
            }

            nodesInfo.EmplaceBack(nodeInfo);
            idCount++;
        }

        for(const RID& link : links)
        {
            ResourceObject linkObject = Repository::Read(link);
            nodeLinks.EmplaceBack(
                ResourceGraphLinkInfo{
                    .outputNodeId = nodeIds[linkObject[GraphNodeLinkAsset::OutputNode].Value<RID>()],
                    .outputPin = linkObject[GraphNodeLinkAsset::OutputPin].As<String>(),
                    .inputNodeId = nodeIds[linkObject[GraphNodeLinkAsset::InputNode].Value<RID>()],
                    .inputPin = linkObject[GraphNodeLinkAsset::InputPin].As<String>(),
                });
        }

        SetGraph(nodesInfo, nodeLinks);
    }

    ResourceGraphInstance* ResourceGraph::CreateInstance()
    {
        return m_allocator.Alloc<ResourceGraphInstance>(this);
    }

    Span<SharedPtr<ResourceGraphNodeData>> ResourceGraph::GetNodes() const
    {
        return m_nodes;
    }

    void ResourceGraph::Destroy()
    {
        for (auto& data : m_data)
        {
            if (!data.offsetFromInstance && data.defaultValue != nullptr && data.typeHandler)
            {
                data.typeHandler->Destroy(data.defaultValue);
            }
        }

        m_nodes.Clear();
        m_links.Clear();
        m_data.Clear();
        m_publicInputs.Clear();
        m_outputs.Clear();
    }

    void RegisterResourceGraphTypes()
    {
    }
}
