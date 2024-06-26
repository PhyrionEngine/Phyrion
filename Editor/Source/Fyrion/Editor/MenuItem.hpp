#pragma once

#include "Fyrion/Common.hpp"
#include "Fyrion/Core/StringView.hpp"
#include "Fyrion/IO/InputTypes.hpp"
#include "Fyrion/Core/String.hpp"
#include "Fyrion/Core/Array.hpp"
#include "Fyrion/Core/SharedPtr.hpp"
#include "Fyrion/Core/HashMap.hpp"

namespace Fyrion
{
	struct MenuItemEventData
	{
		VoidPtr drawData{};
		VoidPtr itemData{};
	};


	typedef void            (*FnMenuItemAction)(const MenuItemEventData& eventData);
	typedef bool            (*FnMenuItemEnable)(const MenuItemEventData& eventData);

	struct MenuItemCreation
	{
		StringView       itemName{};
		StringView       icon{};
		i32              priority{};
		Shortcut         itemShortcut{};
		FnMenuItemAction action{};
		FnMenuItemEnable enable{};
		VoidPtr          menuData{};
	};

    class FY_API MenuItemContext
    {
    public:
        void AddMenuItem(const MenuItemCreation& menuItem);
        void Draw(VoidPtr userData = nullptr);
        bool ExecuteHotKeys(VoidPtr userData = nullptr, bool executeOnFocus = false);
    private:
        String                                       m_label{};
        String                                       m_itemName{};
        i32                                          m_priority{};
        Array<MenuItemContext*>                      m_children{};
        HashMap <String, SharedPtr<MenuItemContext>> m_menuItemsMap{};
        FnMenuItemAction                             m_action{};
        FnMenuItemEnable                             m_enable{};
        Shortcut                                     m_itemShortcut{};
        VoidPtr                                      m_itemMenuData{};

        static void DrawMenuItemChildren(MenuItemContext* context, VoidPtr userData);
        static bool ExecuteHotKeys(MenuItemContext* context, VoidPtr userData, bool executeOnFocus);
    };
}