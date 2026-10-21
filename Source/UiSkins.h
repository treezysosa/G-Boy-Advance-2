#pragma once
#include <array>
#include <string_view>

namespace gba_ui {
struct Skin {std::string_view id,name;float hue=.565f,saturation=.72f,brightness=1;};
// Persistent IDs: do not rename or reorder. Appearance is not an audio parameter.
inline constexpr std::array<Skin,30> skins {{{"purple","Purple"},{"glacier","Glacier Blue",.535f},
    {"black","Black"},{"white","White"},{"spice","Spice Orange",.065f,1},
    {"crimson","Crimson Red",.99f,.9f},{"emerald","Emerald Green",.36f,.88f},
    {"midnight","Midnight Blue",.645f,.82f,.65f},{"gold","Gold",.125f,.85f},
    {"rose","Rose Pink",.93f,.65f},
    {"arctic","Arctic Cyan",.49f,.82f},{"atomic","Atomic Violet",.775f,1},
    {"graphite","Graphite Grey",.60f,.10f,.64f},{"mint","Ice Mint",.43f,.28f},
    {"cobalt","Cobalt Blue",.62f,1,.92f},{"copper","Sunset Copper",.035f,.78f,.82f},
    {"lavender","Pearl Lavender",.72f,.32f},{"coral","Candy Coral",.025f,.61f},
    {"forest","Forest Green",.295f,.95f,.59f},{"lemon","Lemon Yellow",.17f,.80f},
    {"amethyst","Amethyst",.79f,.60f,.78f},{"seafoam","Seafoam",.455f,.46f,.88f},
    {"sunset","Sunset",.045f,.96f,1.f},{"iceblue","Ice Blue",.565f,.26f,.98f},
    {"wine","Wine Red",.965f,.87f,.55f},{"olive","Olive",.225f,.74f,.68f},
    {"pearl","Pearl",.13f,.075f,.98f},{"bronze","Bronze",.088f,.90f,.72f},
    {"bubblegum","Bubblegum",.88f,.80f,1.f},{"smoke","Smoke",.69f,.055f,.48f}}};
inline int skinIndex(std::string_view id) noexcept {
    for(std::size_t i=0;i<skins.size();++i)if(skins[i].id==id)return int(i);
    return 0;
}
inline int validSkin(int index) noexcept {return index>=0&&index<int(skins.size())?index:0;}
// Map a uniform draw in [0, skin count - 2] to every colour except the last one.
inline int differentSkin(int previous,int draw) noexcept {
    previous=validSkin(previous);
    draw=draw>=0&&draw<int(skins.size())-1?draw:0;
    return draw>=previous?draw+1:draw;
}
}
