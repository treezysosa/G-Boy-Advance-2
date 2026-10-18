#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

namespace gba_ui {
// Original 5x7 vector lettering for the LCD; no external font or bitmap asset.
inline std::array<unsigned,7> glyph(juce::juce_wchar c) {
    switch(c) {
        case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
        case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
        case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
        case 'G':return {14,17,16,23,17,17,14};case 'H':return {17,17,17,31,17,17,17};
        case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,18,18,12};
        case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
        case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,25,21,19,19,17};
        case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
        case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
        case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
        case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
        case 'W':return {17,17,17,21,21,27,17};case 'X':return {17,17,10,4,10,17,17};
        case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
        case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
        case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
        case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
        case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
        case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
        case '-':return {0,0,0,31,0,0,0};case '.':return {0,0,0,0,0,6,6};
        case '&':return {12,18,20,8,21,18,13};case '\'':return {4,4,8,0,0,0,0};
        case '(':return {2,4,8,8,8,4,2};case ')':return {8,4,2,2,2,4,8};
        case '/':return {1,2,2,4,8,8,16};case ' ':return {};
        default:return {14,17,1,2,4,0,4};
    }
}
inline void pixelText(juce::Graphics& g,juce::String value,juce::Rectangle<float> bounds,float maximumCell=1000,bool ellipsize=true) {
    value=value.toUpperCase();
    int maximum=std::max(1,int(bounds.getWidth()/10.8f));if(ellipsize&&value.length()>maximum)value=value.substring(0,std::max(1,maximum-3))+"...";
    float cell=std::min({bounds.getHeight()/9,float(bounds.getWidth())/float(std::max(1,value.length()*6-1)),maximumCell});
    auto origin=juce::Point<float>(bounds.getCentreX()-float(value.length()*6-1)*cell*.5f,bounds.getCentreY()-3.5f*cell);
    for(auto c:value) {auto rows=glyph(c);for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(rows[std::size_t(y)]&(1u<<unsigned(4-x)))g.fillRect(origin.x+float(x)*cell,origin.y+float(y)*cell,cell*.9f,cell*.9f);origin.x+=6*cell;}
}
// Contiguous, slanted block lettering used only for the original product wordmark.
inline juce::Path blockLettering(const juce::String& value,juce::Rectangle<float> bounds) {
    juce::Path path;
    float advance=0;
    for(auto c:value) {
        // Original heavy 7x7 capitals, with a two-cell stroke for the large title.
        auto rows=glyph(c);int columns=5;
        if(c=='G'){rows={62,127,96,111,99,127,62};columns=7;}
        if(c=='B'){rows={126,127,99,126,99,127,126};columns=7;}
        if(c=='O'){rows={62,127,99,99,99,127,62};columns=7;}
        if(c=='Y'){rows={99,99,119,62,28,28,28};columns=7;}
        if(c=='2'){rows={62,127,3,14,56,112,127};columns=7;}
        using Point=juce::Point<int>;
        std::vector<std::pair<Point,Point>> edges;
        auto filled=[&](int x,int y){return x>=0&&x<columns&&y>=0&&y<7&&(rows[std::size_t(y)]&(1u<<unsigned(columns-1-x)))!=0;};
        for(int y=0;y<7;++y)for(int x=0;x<columns;++x)if(filled(x,y)) {
            if(!filled(x,y-1))edges.push_back({{x,y},{x+1,y}});
            if(!filled(x+1,y))edges.push_back({{x+1,y},{x+1,y+1}});
            if(!filled(x,y+1))edges.push_back({{x+1,y+1},{x,y+1}});
            if(!filled(x-1,y))edges.push_back({{x,y+1},{x,y}});
        }
        // Trace only the outside edges; stroking separate pixel rectangles would
        // leave black seams inside the connected letters at fractional DPI scales.
        while(!edges.empty()) {
            const auto start=edges.back().first;auto at=start;
            path.startNewSubPath(advance+float(at.x),float(at.y));
            do {
                auto next=std::find_if(edges.begin(),edges.end(),[&](const auto& edge){return edge.first==at;});
                if(next==edges.end())break;
                at=next->second;edges.erase(next);path.lineTo(advance+float(at.x),float(at.y));
            } while(at!=start);
            path.closeSubPath();
        }
        advance+=c==' '?3.5f:float(columns+1);
    }
    path.applyTransform(juce::AffineTransform(1,-.18f,1.26f,0,1,0));
    path.scaleToFit(bounds.getX(),bounds.getY(),bounds.getWidth(),bounds.getHeight(),false);
    return path;
}
}
