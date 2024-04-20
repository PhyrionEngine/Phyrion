#include "PropertiesWindow.hpp"

#include "Fyrion/Editor/Editor.hpp"
#include "Fyrion/Editor/Scene/SceneEditor.hpp"
#include "Fyrion/ImGui/IconsFontAwesome6.h"
#include "Fyrion/ImGui/ImGui.hpp"

namespace Fyrion
{
    void PropertiesWindow::Draw(u32 id, bool& open)
    {
        ImGui::Begin(id, ICON_FA_CIRCLE_INFO " Properties", &open, ImGuiWindowFlags_NoScrollbar);

        SceneObjectNode* object = Editor::GetSceneEditor().GetLastSelectedObject();
        if (object)
        {
            DrawSceneObject(object);
        }

        ImGui::End();
    }

    void PropertiesWindow::OpenProperties(VoidPtr userData)
    {
        Editor::OpenWindow<PropertiesWindow>();
    }

    void PropertiesWindow::DrawSceneObject(SceneObjectNode* sceneObjectNode)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        bool readOnly = false;

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

            u32 hash = HashValue(sceneObjectNode->rid);

            if (ImGui::InputText(hash, sceneObjectNode->name))
            {
                m_renamingCache = sceneObjectNode->name;
                m_renamingFocus = true;
               //propertiesWindow->renamingEntity = entity;
            }

            if (!ImGui::IsItemActive() && m_renamingFocus)
            {
                //WorldController::RenameEntity(propertiesWindow->renamingEntity, propertiesWindow->renamingCache);
                m_renamingFocus = false;
                m_renamingCache.Clear();
            }

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("UUID");
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            String uuid = ToString(sceneObjectNode->uuid);
            ImGui::InputText(hash + 10, uuid, ImGuiInputTextFlags_ReadOnly);
            ImGui::EndDisabled();
            ImGui::EndTable();

        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5 * style.ScaleFactor);

    }

    void PropertiesWindow::RegisterType(NativeTypeHandler<PropertiesWindow>& type)
    {
        Editor::AddMenuItem(MenuItemCreation{.itemName="Window/Properties", .action = OpenProperties});

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
