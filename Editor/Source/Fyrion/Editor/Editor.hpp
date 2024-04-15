#pragma once

#include "Fyrion/Common.hpp"
#include "EditorTypes.hpp"
#include "MenuItem.hpp"
#include "Fyrion/Resource/ResourceTypes.hpp"

namespace Fyrion
{
    class WorldEditor;
}

namespace Fyrion::Editor
{
    FY_API void             Init();
    FY_API void             AddMenuItem(const MenuItemCreation& menuItem);
    FY_API void             OpenWindow(TypeID windowType, VoidPtr initUserData = nullptr);
    FY_API void             OpenProject(RID rid);

    FY_API WorldEditor&     GetWorldEditor();
    FY_API AssetTree&       GetAssetTree();

    template<typename T>
    FY_API void OpenWindow(VoidPtr initUserData = nullptr)
    {
        OpenWindow(GetTypeID<T>(), initUserData);
    }
}
