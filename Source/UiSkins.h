#pragma once
#include <array>
#include <string_view>

namespace gba_ui {
struct Skin {std::string_view id,name;};
// Persistent IDs: do not rename or reorder. Appearance is not an audio parameter.
inline constexpr std::array<Skin,5> skins {{{"purple","Purple"},{"glacier","Glacier Blue"},
    {"black","Black"},{"white","White"},{"spice","Spice Orange"}}};
inline int skinIndex(std::string_view id) noexcept {
    for(std::size_t i=0;i<skins.size();++i)if(skins[i].id==id)return int(i);
    return 0;
}
inline int validSkin(int index) noexcept {return index>=0&&index<int(skins.size())?index:0;}
}
