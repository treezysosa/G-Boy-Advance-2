#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace gba {
// Prepared on a loader thread, immutable during rendering. No ownership work in DSP.
struct SampleData {
    struct Level {std::vector<float> left,right;};
    double rate=48000;
    std::vector<Level> levels;
    int frames() const noexcept {return levels.empty()?0:int(levels.front().left.size());}
    void buildMips() {
        while(!levels.empty()&&levels.back().left.size()>32) {
            const auto& previous=levels.back();Level next;
            const auto count=(previous.left.size()+1)/2;next.left.resize(count);next.right.resize(count);
            constexpr std::array<float,7> kernel{1.f/64,6.f/64,15.f/64,20.f/64,15.f/64,6.f/64,1.f/64};
            for(std::size_t i=0;i<count;++i)for(int t=-3;t<=3;++t) {
                const int at=std::clamp(int(i*2)+t,0,int(previous.left.size())-1);
                next.left[i]+=previous.left[std::size_t(at)]*kernel[std::size_t(t+3)];
                next.right[i]+=previous.right[std::size_t(at)]*kernel[std::size_t(t+3)];
            }
            levels.push_back(std::move(next));
        }
    }
    std::array<float,2> read(double position,double step,bool loop) const noexcept {
        if(frames()<4||position<0||position>=frames())return {};
        std::size_t mip=0;double scale=1;
        while(step/scale>=2&&mip+1<levels.size()){++mip;scale*=2;}

        const auto readChannel=[&](const std::vector<float>& data,double point) {
            const int n=int(data.size()),index=int(point);const float fraction=float(point-index);
            const auto get=[&](int p){if(loop)p=(p%n+n)%n;else p=std::clamp(p,0,n-1);return data[std::size_t(p)];};
            const float a=get(index-1),b=get(index),c=get(index+1),d=get(index+2);
            return b+.5f*fraction*(c-a+fraction*(2*a-5*b+4*c-d+fraction*(3*(b-c)+d-a)));
        };
        const auto readAt=[&](const std::vector<float> Level::* channel,double point) {
            const float a=readChannel(levels[mip].*channel,point/scale);
            const float blend=float(std::clamp(step/scale-1,0.0,1.0));
            if(blend<=0||mip+1>=levels.size())return a;
            return std::lerp(a,readChannel(levels[mip+1].*channel,point/(scale*2)),blend);
        };
        std::array<float,2> value{readAt(&Level::left,position),readAt(&Level::right,position)};
        const double fade=std::min(128.0,double(frames())*.125);
        if(loop&&position>frames()-fade) {
            const double t=(position-(frames()-fade))/fade;
            value[0]=std::lerp(value[0],readAt(&Level::left,position-(frames()-fade)),float(t));
            value[1]=std::lerp(value[1],readAt(&Level::right,position-(frames()-fade)),float(t));
        } else if(!loop) {
            const float gain=float(std::min(1.0,(frames()-position)/fade));value[0]*=gain;value[1]*=gain;
        }
        return value;
    }
};
struct SampleOptions {int mode=0,root=60;bool loop=false;}; // 0=synth, 1=sample, 2=layer
}
