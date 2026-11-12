#pragma once
#include <JuceHeader.h>
#include "UiSkins.h"
#include <cmath>

namespace gba_ui {
// Original geometric decorations. No franchise artwork, fonts, symbols or
// characters are used. Called only when the static housing cache is rebuilt.
inline void paintTheme(juce::Graphics& g,const Skin& skin,juce::Rectangle<float> area) {
    if(skin.pattern==Pattern::none||area.isEmpty())return;
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(area.getSmallestIntegerContainer());
    g.addTransform(juce::AffineTransform::translation(area.getX(),area.getY()));
    const float w=area.getWidth(),h=area.getHeight();
    const auto accent=juce::Colour(skin.accent);
    const auto stroke=juce::PathStrokeType(1.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
    const auto glint=[&](float x,float y,float size){
        juce::Path p;
        for(int i=0;i<8;++i){const float angle=float(i)*juce::MathConstants<float>::pi/4;
            const float radius=i%2?size*.28f:size;const float px=x+std::sin(angle)*radius,py=y+std::cos(angle)*radius;
            if(i==0)p.startNewSubPath(px,py);else p.lineTo(px,py);}
        p.closeSubPath();g.fillPath(p);
    };
    const auto waves=[&](int count,float base){
        for(int row=0;row<count;++row){juce::Path p;
            for(int x=-24;x<=int(w)+24;x+=12){const float y=h*base+float(row)*26+6*std::sin(float(x)*.027f+float(row)*.8f);
                if(x==-24)p.startNewSubPath(float(x),y);else p.lineTo(float(x),y);}
            g.setColour((row%2?juce::Colours::white:accent).withAlpha(.19f));g.strokePath(p,stroke);
        }
    };
    switch(skin.pattern){
        case Pattern::meadow:
            for(int row=0;row<4;++row)for(int col=0;col<9;++col){
                const float x=38+float(col)*154+float(row%2)*35,y=24+float(row)*68;
                juce::Path leaf;leaf.startNewSubPath(x-7,y+9);leaf.cubicTo(x-16,y-2,x-2,y-13,x+11,y-12);
                leaf.cubicTo(x+15,y+2,x+1,y+15,x-7,y+9);leaf.closeSubPath();
                g.setColour(accent.withAlpha(.16f));g.fillPath(leaf);
                g.setColour(juce::Colours::white.withAlpha(.13f));g.drawLine(x-8,y+12,x+7,y-7,1);
            }break;
        case Pattern::trail:
            for(int row=0;row<4;++row){juce::Path p;const float y=18+float(row)*56;
                p.startNewSubPath(-30,y);p.cubicTo(w*.20f,y+56,w*.37f,y-49,w*.55f,y+4);
                p.cubicTo(w*.75f,y+59,w*.85f,y-48,w+30,y+10);
                g.setColour(accent.withAlpha(.18f));g.strokePath(p,stroke);
                for(int col=0;col<7;++col){const float x=90+float(col)*182;
                    juce::Path marker;marker.addQuadrilateral(x,y-4,x+4,y,x,y+4,x-4,y);
                    g.setColour(juce::Colours::white.withAlpha(.2f));g.fillPath(marker);}
            }break;
        case Pattern::grid:
            for(int row=0;row<4;++row)for(int col=0;col<10;++col){
                const float x=18+float(col)*142,y=18+float(row)*60;
                juce::Path trace;trace.startNewSubPath(x,y);trace.lineTo(x+30,y);trace.lineTo(x+48,y+18);trace.lineTo(x+102,y+18);
                g.setColour(accent.withAlpha(.23f));g.strokePath(trace,stroke);
                g.fillRect(x-2,y-2,4.0f,4.0f);g.drawRect(x+100,y+16,5.0f,5.0f,1.0f);
            }break;
        case Pattern::prism:
            for(int row=0;row<4;++row)for(int col=0;col<10;++col){
                const float x=-15+float(col)*152+float(row%2)*76,y=-12+float(row)*70;
                juce::Path p;p.addTriangle(x,y+60,x+76,y,x+152,y+60);
                g.setColour(accent.withAlpha((row+col)%2?.10f:.17f));g.fillPath(p);
                g.setColour(juce::Colours::white.withAlpha(.12f));g.strokePath(p,stroke);
            }break;
        case Pattern::surge:
            for(int i=0;i<13;++i){const float x=-140+float(i)*134;
                g.setColour((i%2?accent:juce::Colours::white).withAlpha(.21f));
                g.drawLine(x,h+20,x+180,-20,float(i%3==0?3:1));}
            g.setColour(accent.withAlpha(.3f));
            for(int i=0;i<9;++i)glint(76+float(i)*149,30+float(i%3)*63,7);
            break;
        case Pattern::ink:
            for(int row=0;row<11;++row)for(int col=0;col<28;++col){
                const float x=18+float(col)*49+float(row%2)*10,y=12+float(row)*27;
                g.setColour(accent.withAlpha((row+col)%3==0?.18f:.07f));g.fillEllipse(x,y,3.0f,3.0f);
            }
            for(int row=0;row<3;++row){juce::Path ribbon;const float y=35+float(row)*72;
                ribbon.startNewSubPath(-20,y);ribbon.cubicTo(w*.23f,y-60,w*.32f,y+60,w*.49f,y);
                ribbon.cubicTo(w*.72f,y-66,w*.80f,y+70,w+20,y-10);
                g.setColour(accent.withAlpha(.2f));g.strokePath(ribbon,juce::PathStrokeType(2));}
            break;
        case Pattern::evergreen:
            for(int row=0;row<4;++row)for(int col=0;col<8;++col){
                const float x=38+float(col)*180+float(row%2)*34,y=25+float(row)*71;
                g.setColour(accent.withAlpha(.24f));g.drawLine(x-13,y+13,x+12,y-12,1.5f);
                for(int n=0;n<4;++n){const float dx=x-10+float(n)*6,dy=y+10-float(n)*6;
                    g.drawLine(dx,dy,dx-8,dy-4,1.3f);g.drawLine(dx,dy,dx+4,dy+8,1.3f);}
                glint(x+49,y+2,5);
            }break;
        case Pattern::frost:
            g.setColour(accent.withAlpha(.26f));
            for(int row=0;row<4;++row)for(int col=0;col<8;++col){
                const float x=48+float(col)*178+float(row%2)*37,y=29+float(row)*69;
                for(int arm=0;arm<6;++arm){const float angle=float(arm)*juce::MathConstants<float>::pi/3;
                    const float dx=std::cos(angle),dy=std::sin(angle);
                    g.drawLine(x,y,x+dx*12,y+dy*12,1.2f);
                    for(float turn:{-2.2f,2.2f})g.drawLine(x+dx*8,y+dy*8,x+dx*8+std::cos(angle+turn)*4,y+dy*8+std::sin(angle+turn)*4,1);
                }
            }break;
        case Pattern::waves:waves(5,.15f);break;
        case Pattern::sunset:
            g.setColour(accent.withAlpha(.20f));g.fillEllipse(w*.82f,h*.02f,69,69);
            g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(w*.82f-6,h*.02f-6,81,81,1);
            waves(4,.43f);break;
        case Pattern::none:break;
    }
}
}
