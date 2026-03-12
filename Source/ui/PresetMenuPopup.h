#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PresetMenuPopup
{
public:
    struct Item
    {
        juce::String category;
        juce::String title;
        int index = -1;
    };

    static int show(const juce::Array<Item>& items, juce::Component* target)
    {
        juce::PopupMenu menu;
        juce::String last;

        for (const auto& item : items)
        {
            if (item.category != last)
            {
                if (last.isNotEmpty())
                    menu.addSeparator();

                menu.addSectionHeader(item.category);
                last = item.category;
            }

            menu.addItem(item.index + 1, item.title);
        }

        return menu.showAt(target);
    }
};
