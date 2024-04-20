#include "SceneTreeWindow.hpp"

#include "Fyrion/Editor/Editor.hpp"
#include "Fyrion/ImGui/IconsFontAwesome6.h"
#include "Fyrion/ImGui/ImGui.hpp"

namespace Fyrion
{
    MenuItemContext SceneTreeWindow::s_menuItemContext = {};

    SceneTreeWindow::SceneTreeWindow(): m_sceneEditor(Editor::GetSceneEditor())
    {
    }

    void SceneTreeWindow::DrawSceneObject(SceneObjectNode* sceneObjectNode)
    {
        if (sceneObjectNode == nullptr) return;

        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        bool root = sceneObjectNode->parent == nullptr;

        m_nameCache.Clear();
        m_nameCache += root ? ICON_FA_CUBES : ICON_FA_CUBE;
        m_nameCache += " ";
        m_nameCache += sceneObjectNode->name;

        bool isSelected = sceneObjectNode->selected;
        auto treeFlags = isSelected ? ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAllColumns : ImGuiTreeNodeFlags_SpanAllColumns;
        bool open = false;

        ImGuiID treeId = 100000 + static_cast<ImGuiID>(HashValue(sceneObjectNode->rid));

        if (root)
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        }

        if (!sceneObjectNode->children.Empty())
        {
            open = ImGui::TreeNode(treeId, m_nameCache.CStr(), treeFlags);
        }
        else
        {
            ImGui::TreeLeaf(treeId, m_nameCache.CStr(), treeFlags);
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EntityTreePayload))
            {
                // moveEntitiesTo = FY_NULL_ENTITY;
                // removeSelectionParent = true;
            }
            ImGui::EndDragDropTarget();
        }

        bool isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        if ((ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) && isHovered)
        {
            if (!(ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl)) || ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_RightCtrl))))
            {
                m_sceneEditor.CleanSelection();
            }
            m_sceneEditor.SelectObject(sceneObjectNode);
        }


        ImGui::TableNextColumn();
        if (!root)
        {
            ImGui::Text("  " ICON_FA_EYE);
        }


        if (open)
        {
            for(SceneObjectNode* child: sceneObjectNode->children)
            {
                DrawSceneObject(child);
            }
            ImGui::TreePop();
        }
    }

    void SceneTreeWindow::Draw(u32 id, bool& open)
    {
        auto& style = ImGui::GetStyle();
        auto originalWindowPadding = style.WindowPadding;

        ImGui::StyleVar windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(id, ICON_FA_LIST " Scene Tree", &open, ImGuiWindowFlags_NoScrollbar);
        bool openPopup = false;

        {
            ImGui::StyleVar childWindowPadding(ImGuiStyleVar_WindowPadding, originalWindowPadding);

            ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar;
            ImGui::BeginChild("top-fields", ImVec2(0, (25 * style.ScaleFactor) + originalWindowPadding.y), false, flags);

            if (ImGui::Button(ICON_FA_PLUS))
            {
                openPopup = true;
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(-1);
            ImGui::SearchInputText(id + 10, m_searchEntity);
            ImGui::EndChild();
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + originalWindowPadding.y);

        {
            ImGui::StyleVar cellPadding(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
            ImGui::StyleVar frameRounding(ImGuiStyleVar_FrameRounding, 0);
            ImGui::StyleColor childBg(ImGuiCol_ChildBg, IM_COL32(22, 23, 25, 255));
            ImGui::StyleColor borderColor(ImGuiCol_Border, IM_COL32(45, 46, 48, 255));

            if (ImGui::BeginChild("scene-tree-view-child", ImVec2(0, 0), false))
            {
                static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody;

                if (ImGui::BeginTable("scene-tree-view-table", 2, tableFlags))
                {
                    ImGui::TableSetupColumn("  Name", ImGuiTableColumnFlags_NoHide);
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 35 * style.ScaleFactor);
                    ImGui::TableHeadersRow();

                    if (m_sceneEditor.IsLoaded())
                    {
                        ImGui::BeginTreeNode();
                        DrawSceneObject(m_sceneEditor.GetRootNode());
                        ImGui::EndTreeNode();
                    }

                    ImGui::EndTable();
                }
            }

            ImGui::EndChild();
        }


        bool closePopup = false;

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            if (s_menuItemContext.ExecuteHotKeys(this))
            {
                closePopup = true;
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            {
                openPopup = true;
            }
        }

        if (openPopup)
        {
            ImGui::OpenPopup("scene-tree-popup");
        }

        bool popupRes = ImGui::BeginPopupMenu("scene-tree-popup");
        if (popupRes)
        {
            s_menuItemContext.Draw(this);
            if (closePopup)
            {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopupMenu(popupRes);

        ImGui::End();
    }

    void SceneTreeWindow::AddMenuItem(const MenuItemCreation& menuItem)
    {
        s_menuItemContext.AddMenuItem(menuItem);
    }

    void SceneTreeWindow::OpenSceneTree(VoidPtr userData)
    {
        Editor::OpenWindow<SceneTreeWindow>();
    }

    void SceneTreeWindow::AddSceneObject(VoidPtr userData)
    {
        static_cast<SceneTreeWindow*>(userData)->m_sceneEditor.CreateObject();
    }

    void SceneTreeWindow::AddSceneObjectFromAsset(VoidPtr userData)
    {
    }

    void SceneTreeWindow::AddComponent(VoidPtr userData)
    {
    }

    void SceneTreeWindow::RenameSceneObject(VoidPtr userData)
    {
    }

    void SceneTreeWindow::DuplicateSceneObject(VoidPtr userData)
    {
    }

    void SceneTreeWindow::DeleteSceneObject(VoidPtr userData)
    {
        static_cast<SceneTreeWindow*>(userData)->m_sceneEditor.DestroySelectedObjects();
    }

    void SceneTreeWindow::RegisterType(NativeTypeHandler<SceneTreeWindow>& type)
    {
        Editor::AddMenuItem(MenuItemCreation{.itemName = "Window/Scene Tree", .action = OpenSceneTree});

        AddMenuItem(MenuItemCreation{.itemName = "Add Object", .priority = 0, .action = AddSceneObject});
        AddMenuItem(MenuItemCreation{.itemName = "Add Object From Asset", .priority = 10, .action = AddSceneObjectFromAsset});
        AddMenuItem(MenuItemCreation{.itemName = "Add Component", .priority = 20, .action = AddComponent});
        AddMenuItem(MenuItemCreation{.itemName = "Rename", .priority = 200, .itemShortcut = {.presKey = Key::F2}, .action = RenameSceneObject});
        AddMenuItem(MenuItemCreation{.itemName = "Duplicate", .priority = 210, .itemShortcut = {.ctrl = true, .presKey = Key::D}, .action = DuplicateSceneObject});
        AddMenuItem(MenuItemCreation{.itemName = "Delete", .priority = 220, .itemShortcut = {.presKey = Key::Delete}, .action = DeleteSceneObject});

        type.Attribute<EditorWindowProperties>(EditorWindowProperties{
            .dockPosition = DockPosition::TopRight,
            .createOnInit = true
        });
    }

    void InitSceneTreeWindow()
    {
        Registry::Type<SceneTreeWindow, EditorWindow>();
    }
}
