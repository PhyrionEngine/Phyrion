#include "PropertiesWindow.hpp"

#include "GraphEditorWindow.hpp"
#include "imgui_internal.h"
#include "Fyrion/Assets/AssetTypes.hpp"
#include "Fyrion/Core/StringUtils.hpp"
#include "Fyrion/Editor/Editor.hpp"
#include "Fyrion/Editor/Editor/SceneEditor.hpp"
#include "Fyrion/ImGui/IconsFontAwesome6.h"
#include "Fyrion/ImGui/ImGui.hpp"
#include "Fyrion/Resource/Repository.hpp"
#include "Fyrion/Scene/SceneAssets.hpp"

namespace Fyrion
{
    PropertiesWindow::PropertiesWindow() : m_sceneEditor(Editor::GetSceneEditor())
    {
    }

    void PropertiesWindow::Draw(u32 id, bool& open)
    {
        m_idCount = id;

        ImGui::Begin(id, ICON_FA_CIRCLE_INFO " Properties", &open, ImGuiWindowFlags_NoScrollbar);

        //TODO temporary
        if (RID object = m_sceneEditor.GetLastSelectedObject())
        {
            DrawSceneObject(id, object);
        }
        else if (GraphEditor* graphEditor = GraphEditorWindow::GetLastGraphEditorSelected())
        {
            if (graphEditor->lastNodeSelected)
            {
                DrawGraphNode(graphEditor, graphEditor->lastNodeSelected);
            }
        }

        ImGui::End();
    }

    void PropertiesWindow::OpenProperties(const MenuItemEventData& eventData)
    {
        Editor::OpenWindow<PropertiesWindow>();
    }

    void PropertiesWindow::DrawSceneObject(u32 id, RID rid)
    {
        bool           root = m_sceneEditor.GetRootObject() == rid;
        ResourceObject read = Repository::Read(rid);

        ImGuiStyle& style = ImGui::GetStyle();
        bool        readOnly = false;

        ImGuiInputTextFlags nameFlags = 0;
        if (readOnly || root)
        {
            nameFlags |= ImGuiInputTextFlags_ReadOnly;
        }

        if (ImGui::BeginTable("#object-table", 2))
        {
            ImGui::BeginDisabled(readOnly);

            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();

            ImGui::Text("Name");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);



            StringView objectName = root ? m_sceneEditor.GetRootName() : read[SceneObjectAsset::Name].Value<StringView>();
            m_stringCache = objectName;
            u32 hash = HashValue(rid);

            if (ImGui::InputText(hash, m_stringCache, nameFlags))
            {
                m_renamingCache = m_stringCache;
                m_renamingFocus = true;
                m_renamingObject = rid;
            }

            if (!ImGui::IsItemActive() && m_renamingFocus)
            {
                m_sceneEditor.RenameObject(m_renamingObject, m_renamingCache);
                m_renamingObject = {};
                m_renamingFocus = false;
                m_renamingCache.Clear();
            }

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UUID");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            String uuid = ToString(Repository::GetUUID(rid));
            ImGui::InputText(hash + 10, uuid, ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            ImGui::EndTable();
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5 * style.ScaleFactor);

        f32  width = ImGui::GetContentRegionAvail().x;
        auto size = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;

        ImGui::BeginHorizontal("horizontal-01", ImVec2(width, size));

        ImGui::Spring(1.f);
        bool addComponent = false;

        ImGui::BeginDisabled(readOnly);

        if (ImGui::BorderedButton("Add Component", ImVec2(width * 2 / 3, size)))
        {
            addComponent = true;
        }

        ImGui::EndDisabled();

        ImVec2 max = ImGui::GetItemRectMax();
        ImVec2 min = ImGui::GetItemRectMin();

        ImGui::Spring(1.f);

        ImGui::EndHorizontal();

        RID prototype = Repository::GetPrototype(rid);
        if (prototype)
        {
            ImGui::BeginHorizontal(9999, ImVec2(width, size));
            ImGui::Spring(1.f);

            if (ImGui::BorderedButton("horizontal-01-02", ImVec2((width * 2) / 3, size)))
            {
                int a = 0;
                //openAsset = entityData.rid;
            }

            ImGui::Spring(1.f);
            ImGui::EndHorizontal();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5 * style.ScaleFactor);

        Array<RID> components = read.GetSubObjectSetAsArray(SceneObjectAsset::Components);

        bool openComponentSettings = false;

        for (RID component : components)
        {
            TypeHandler* typeHandler = Repository::GetResourceTypeHandler(component);
            if (typeHandler)
            {
                bool propClicked = false;
                bool open = ImGui::CollapsingHeaderProps(HashValue(component), FormatName(typeHandler->GetSimpleName()).CStr(), &propClicked);
                if (propClicked)
                {
                    openComponentSettings = true;
                    m_selectedComponent = component;
                }
                if (open)
                {
                    ImGui::Indent();
                    ImGui::DrawType(ImGui::DrawTypeDesc{
                        .itemId = component.id,
                        .rid = component,
                        .typeHandler = typeHandler,
                        .instance = Repository::ReadData(component),
                        .flags = readOnly ? ImGuiDrawTypeFlags_ReadOnly : 0u,
                        .userData = &m_sceneEditor,
                        .callback = [](ImGui::DrawTypeDesc& desc, ConstPtr newValue)
                        {
                            static_cast<SceneEditor*>(desc.userData)->UpdateComponent(desc.rid, newValue);
                        },
                        .onCreateSubobject = [](ImGui::DrawTypeDesc& desc, RID subobject)
                        {
                            int a = 0;
                        }
                    });
                    ImGui::Unindent();
                }
            }
        }

        if (addComponent)
        {
            ImGui::OpenPopup("add-component-popup");
        }

        ImGui::SetNextWindowPos(ImVec2(min.x, max.y + 5));
        auto sizePopup = max.x - min.x;
        ImGui::SetNextWindowSize(ImVec2(sizePopup, 0), ImGuiCond_Appearing);

        auto popupRes = ImGui::BeginPopupMenu("add-component-popup", 0, false);
        if (popupRes)
        {
            ImGui::SetNextItemWidth(sizePopup - style.WindowPadding.x * 2);
            ImGui::SearchInputText(id + 100, m_searchComponentString);
            ImGui::Separator();

            TypeHandler* componentHandler = Registry::FindType<Component>();
            if (componentHandler)
            {
                for (DerivedType& derivedType : componentHandler->GetDerivedTypes())
                {
                    TypeHandler* typeHandler = Registry::FindTypeById(derivedType.typeId);
                    if (typeHandler)
                    {
                        String name = FormatName(typeHandler->GetSimpleName());
                        if (ImGui::Selectable(name.CStr()))
                        {
                            m_sceneEditor.AddComponent(rid, typeHandler);
                        }
                    }
                }
            }
        }
        ImGui::EndPopupMenu(popupRes);

        if (openComponentSettings)
        {
            ImGui::OpenPopup("open-component-settings");
        }

        bool popupOpenSettings = ImGui::BeginPopupMenu("open-component-settings", 0, false);
        if (popupOpenSettings)
        {
            if (ImGui::MenuItem("Reset"))
            {
                m_sceneEditor.ResetComponent(m_selectedComponent);
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Remove"))
            {
                m_sceneEditor.RemoveComponent(rid, m_selectedComponent);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopupMenu(popupOpenSettings);
    }

    void PropertiesWindow::DrawGraphNode(GraphEditor* graphEditor, RID node)
    {
        bool readOnly = false;

        ImGuiInputTextFlags nameFlags = 0;
        if (readOnly)
        {
            nameFlags |= ImGuiInputTextFlags_ReadOnly;
        }

        GraphEditorNode* graphNode = graphEditor->GetNodeByRID(node);

        if (ImGui::BeginTable("#object-table", 2))
        {
            ImGui::BeginDisabled(readOnly);

            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();

            ImGui::Text("Name");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);

            m_stringCache = graphNode->label;

            if (ImGui::InputText(PushId(), m_stringCache, nameFlags))
            {
                m_renamingCache = m_stringCache;
                m_renamingFocus = true;
                m_renamingObject = node;
            }

            if (!ImGui::IsItemActive() && m_renamingFocus && m_renamingObject == node)
            {
                graphEditor->RenameNode(graphNode, m_renamingCache);
                m_renamingObject = {};
                m_renamingFocus = false;
                m_renamingCache.Clear();
            }

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UUID");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            String uuid = ToString(Repository::GetUUID(node));
            ImGui::InputText(PushId(), uuid, ImGuiInputTextFlags_ReadOnly);

            ImGui::EndDisabled();
            ImGui::EndTable();
        }

        for (GraphEditorNodePin* pin : graphNode->inputs)
        {

        }

        bool openPopup = false;

        for (GraphEditorNodePin* pin : graphNode->outputs)
        {
            ImGui::BeginDisabled(!pin->editable);
            TypeHandler* typeHandler = Registry::FindTypeById(pin->typeId);
            if (typeHandler)
            {
                bool propClicked = false;
                bool open = ImGui::CollapsingHeaderProps(reinterpret_cast<usize>(pin), pin->label.CStr(), &propClicked);
                if (propClicked)
                {
                    openPopup = true;
                    m_selectedNodePin = pin;
                }

                if (open)
                {
                    if (ImGui::BeginTable("##node-table", 2))
                    {
                        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.4f);
                        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);

                        ImGui::BeginDisabled(readOnly);

                        ImGui::TableNextColumn();
                        ImGui::AlignTextToFramePadding();

                        ImGui::Text("Name");
                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-1);

                        m_stringCache = pin->label;

                        if (ImGui::InputText(PushId(), m_stringCache, nameFlags))
                        {
                            m_renamingCache = m_stringCache;
                            m_renamingFocus = true;
                            m_renmaingPin = pin;
                        }

                        if (!ImGui::IsItemActive() && m_renamingFocus && m_renmaingPin == pin)
                        {
                            graphEditor->RenamePin(pin, m_renamingCache);
                            m_renmaingPin = {};
                            m_renamingFocus = false;
                            m_renamingCache.Clear();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text("Public Value");
                        ImGui::TableNextColumn();

                        bool publicValue = pin->publicValue;

                        if (ImGui::Checkbox("###aaaaa", &publicValue))
                        {
                            graphEditor->ChangePinVisibility(pin, publicValue);
                        }

                        ImGui::EndDisabled();
                        ImGui::EndTable();
                    }
                }
            }
            ImGui::EndDisabled();
        }

        if (openPopup)
        {
            ImGui::OpenPopup("open-node-settings");
        }

        bool popupOpenSettings = ImGui::BeginPopupMenu("open-node-settings", 0, false);
        if (popupOpenSettings)
        {
            if (ImGui::MenuItem("Remove"))
            {
                graphEditor->DeletePin(m_selectedNodePin);
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopupMenu(popupOpenSettings);
    }

    void PropertiesWindow::RegisterType(NativeTypeHandler<PropertiesWindow>& type)
    {
        Editor::AddMenuItem(MenuItemCreation{.itemName = "Window/Properties", .action = OpenProperties});

        type.Attribute<EditorWindowProperties>(EditorWindowProperties{
            .dockPosition = DockPosition::BottomRight,
            .createOnInit = true
        });
    }

    void InitPropertiesWindow()
    {
        Registry::Type<PropertiesWindow, EditorWindow>();
    }
}
