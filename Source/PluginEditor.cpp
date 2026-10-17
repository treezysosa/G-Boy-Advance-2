#include "PluginEditor.h"
#include "PixelText.h"

namespace {
const juce::Colour purple {0xff7943ff}, deep {0xff131c61}, ink {0xff080c25}, silver {0xffe6e5e5}, text {0xffeeeeff};
juce::Font font(float height,bool bold=false) {
#if JUCE_WINDOWS
    const juce::String family="Consolas";
#else
    const auto family=juce::Font::getDefaultMonospacedFontName();
#endif
    return juce::Font(juce::FontOptions(family,height,bold?juce::Font::bold:juce::Font::plain));
}
void panel(juce::Graphics& g,juce::Rectangle<float> bounds,float radius) {
    g.setColour(juce::Colours::black.withAlpha(.55f));g.fillRoundedRectangle(bounds.expanded(2).translated(0,2),radius+2);
    juce::ColourGradient surface(juce::Colour(0xff263179),bounds.getX(),bounds.getY(),ink,bounds.getRight(),bounds.getBottom(),false);
    surface.addColour(.35,juce::Colour(0xff11194c));g.setGradientFill(surface);g.fillRoundedRectangle(bounds,radius);
    g.setColour(juce::Colour(0xffb792ff).withAlpha(.38f));g.drawRoundedRectangle(bounds.reduced(.5f),radius,1.5f);
    g.setColour(juce::Colours::black.withAlpha(.5f));g.drawRoundedRectangle(bounds.reduced(3),std::max(2.0f,radius-3),1);
}
void caption(juce::Graphics& g,const juce::String& label,juce::Rectangle<float> bounds,float size=17) {g.setColour(text);gba_ui::pixelText(g,label,bounds,size/9.5f);}
void rotary(juce::Slider& slider,const juce::String& name) {
    slider.setName(name);slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    slider.setPopupDisplayEnabled(true,true,nullptr);slider.setMouseDragSensitivity(160);slider.setRotaryParameters(juce::MathConstants<float>::pi*1.2f,juce::MathConstants<float>::pi*2.8f,true);
}
void asyncClose(juce::Component* owner,std::function<void()> close) {
    juce::Component::SafePointer<juce::Component> safe(owner);juce::MessageManager::callAsync([safe,close=std::move(close)] {if(safe!=nullptr)close();});
}
class DirectionButton final : public juce::Button {
public:
    explicit DirectionButton(const juce::String& name):Button(name) {setTooltip(name);}
private:
    void paintButton(juce::Graphics&,bool,bool) override {} // The shared silver cross is painted by its parent.
};

class ParameterRow final : public juce::Component {
public:
    ParameterRow(GbaProcessor& processor,gba::Param p) {
        const auto& spec=gba::specs[gba::index(p)];label.setText(spec.name.data(),juce::dontSendNotification);label.setFont(font(13));addAndMakeVisible(label);
        juce::String id(spec.id.data());
        if(!spec.choices.empty()) {
            auto items=juce::StringArray::fromTokens(juce::String(spec.choices.data()),"|","");combo.addItemList(items,1);addAndMakeVisible(combo);
            choice=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,id,combo);
            combo.setTooltip(spec.name.data());
        } else {
            slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,76,23);slider.setNumDecimalPlacesToDisplay(spec.step>=1?0:3);slider.setTooltip(spec.name.data());addAndMakeVisible(slider);
            attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,slider);
            slider.textFromValueFunction=[step=spec.step](double value){return juce::String(value,step>=1?0:(std::abs(value)>=1000?0:3));};
            slider.updateText();
        }
    }
    void resized() override {auto b=getLocalBounds().reduced(7,3);label.setBounds(b.removeFromTop(22));slider.setBounds(b);combo.setBounds(b.reduced(2));}
private:
    juce::Label label;
    juce::Slider slider;
    juce::ComboBox combo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> choice;
};
class ParameterGrid final : public juce::Component {
public:
    ParameterGrid(GbaProcessor& p,std::initializer_list<gba::Param> params) {for(auto value:params){auto row=std::make_unique<ParameterRow>(p,value);addAndMakeVisible(*row);rows.push_back(std::move(row));}}
    void resized() override {int columns=3,width=getWidth()/columns;for(std::size_t i=0;i<rows.size();++i)rows[i]->setBounds(int(i%3)*width,int(i/3)*67,width-4,64);}
    int requiredHeight() const {return int((rows.size()+2)/3)*67+10;}
private:std::vector<std::unique_ptr<ParameterRow>> rows;
};
class ParameterPage final : public juce::Component {
public:
    ParameterPage(GbaProcessor& p,std::initializer_list<gba::Param> params):grid(p,params) {viewport.setViewedComponent(&grid,false);viewport.setScrollBarsShown(true,false);addAndMakeVisible(viewport);}
    void resized() override {viewport.setBounds(getLocalBounds());grid.setSize(std::max(100,getWidth()-16),grid.requiredHeight());}
private:ParameterGrid grid;juce::Viewport viewport;
};
class AdvancedPanel final : public juce::Component {
public:
    AdvancedPanel(GbaProcessor& p,std::function<void()> close):tabs(juce::TabbedButtonBar::TabsAtTop) {
        using P=gba::Param;
        addAndMakeVisible(tabs);addAndMakeVisible(back);back.onClick=[this,close]{asyncClose(this,close);};
        tabs.addTab("OSCILLATORS",deep,new ParameterPage(p,{P::Engine,P::Wave1,P::Wave2,P::OscMix,P::Detune,P::Coarse,P::PulseWidth,P::Brightness,P::FmRatio,P::FmIndex,P::Harmonics,P::Wavetable,P::StringDamping,P::Vowel,P::FormantShift,P::Breath,P::Choir,P::Syllable}),true);
        tabs.addTab("FILTER & ENVELOPES",deep,new ParameterPage(p,{P::Cutoff,P::Resonance,P::FilterType,P::FilterEnv,P::FilterAttack,P::FilterDecay,P::FilterSustain,P::FilterRelease,P::Attack,P::Decay,P::Sustain,P::Release}),true);
        tabs.addTab("MODULATION & VOICES",deep,new ParameterPage(p,{P::Lfo1Rate,P::Lfo1Depth,P::Lfo1Dest,P::Lfo2Rate,P::Lfo2Depth,P::Lfo2Dest,P::Vibrato,P::Tremolo,P::Glide,P::Voices,P::Mode,P::BendRange,P::Velocity}),true);
        tabs.addTab("EFFECTS & MACROS",deep,new ParameterPage(p,{P::Reverb,P::ReverbSize,P::Delay,P::DelayTime,P::DelayFeedback,P::Chorus,P::Phaser,P::Flanger,P::Drive,P::Distortion,P::CrusherBits,P::CrusherRate,P::Wow,P::EqLow,P::EqHigh,P::Width,P::MacroA,P::MacroB,P::MacroC,P::MacroD}),true);
        setWantsKeyboardFocus(true);
    }
    void paint(juce::Graphics& g) override {panel(g,getLocalBounds().toFloat(),14);caption(g,"ADVANCED SYNTHESIS",{20,12,float(getWidth()-210),28},22);}
    void resized() override {back.setBounds(getWidth()-165,12,145,30);tabs.setBounds(16,56,getWidth()-32,getHeight()-70);}
private:juce::TabbedComponent tabs;juce::TextButton back {"BACK / SELECT"};
};
class BrowserPanel final : public juce::Component,private juce::ListBoxModel {
public:
    BrowserPanel(GbaProcessor& p,std::function<void()> close):processor(p),list("Presets",this) {
        for(auto* c:std::initializer_list<juce::Component*>{&search,&category,&favoritesOnly,&recentOnly,&list,&back,&preview,&stop,&favorite,&save,&rename,&remove,&restore,&name,&status})addAndMakeVisible(c);
        search.setTextToShowWhenEmpty("Search names, categories, tags...",text.withAlpha(.55f));search.onTextChange=[this]{refresh();};
        category.addItem("All categories",1);category.addItemList(processor.presets.categories(),2);category.setSelectedId(1,juce::dontSendNotification);category.onChange=[this]{refresh();};
        favoritesOnly.onClick=[this]{refresh();};recentOnly.onClick=[this]{refresh();};
        back.onClick=[this,close]{asyncClose(this,close);};preview.onClick=[this]{processor.preview();};stop.onClick=[this]{processor.stopPreview();};
        favorite.onClick=[this]{processor.presets.toggleFavorite(processor.presetId());refresh();};
        save.onClick=[this]{result(processor.presets.saveAs(name.getText()));};
        rename.onClick=[this]{result(processor.presets.renameUser(processor.presetId(),name.getText()));};
        remove.onClick=[this]{
            auto id=processor.presetId();auto* selected=processor.presets.find(id);if(!selected||selected->factory)return;
            juce::Component::SafePointer<BrowserPanel> safe(this);
            (void)juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Delete user preset?","Delete "+selected->name+" from disk?","Delete","Cancel",this,juce::ModalCallbackFunction::create([safe,id](int answer){if(safe!=nullptr&&answer==1)safe->result(safe->processor.presets.deleteUser(id));}));
        };
        restore.onClick=[this]{processor.presets.restoreDefault();refresh();};
        name.setTextToShowWhenEmpty("Name for Save As / Rename",text.withAlpha(.55f));name.setInputRestrictions(100);
        list.setRowHeight(38);list.setColour(juce::ListBox::backgroundColourId,ink);list.setMultipleSelectionEnabled(false);
        status.setFont(font(12));status.setText(processor.presets.lastError,juce::dontSendNotification);refresh();
    }
    ~BrowserPanel() override {list.setModel(nullptr);}
    void paint(juce::Graphics& g) override {panel(g,getLocalBounds().toFloat(),14);caption(g,"PRESET LIBRARY",{20,10,float(getWidth()-220),30},22);}
    void resized() override {
        int w=getWidth(),h=getHeight();back.setBounds(w-160,12,140,30);search.setBounds(20,56,w-660,30);category.setBounds(w-630,56,300,30);favoritesOnly.setBounds(w-320,56,145,30);recentOnly.setBounds(w-175,56,155,30);
        list.setBounds(20,99,w-40,h-221);
        preview.setBounds(20,h-110,110,29);stop.setBounds(138,h-110,90,29);favorite.setBounds(236,h-110,145,29);restore.setBounds(w-215,h-110,195,29);
        name.setBounds(20,h-69,w-480,30);save.setBounds(w-450,h-69,130,30);rename.setBounds(w-310,h-69,135,30);remove.setBounds(w-165,h-69,145,30);status.setBounds(20,h-35,w-40,25);
    }
private:
    GbaProcessor& processor;
    juce::TextEditor search,name;
    juce::ComboBox category;
    juce::ToggleButton favoritesOnly {"Favorites only"},recentOnly {"Recently used"};
    juce::ListBox list;
    juce::TextButton back {"BACK / START"},preview {"A  PREVIEW"},stop {"B  STOP"},favorite {"FAVORITE"},save {"SAVE AS"},rename {"RENAME"},remove {"DELETE"},restore {"RESTORE DREAM PAD"};
    juce::Label status;
    std::vector<juce::String> visible;
    bool updating=false;
    void result(juce::Result r) {status.setText(r.wasOk()?"Saved. User presets: "+processor.presets.userFolder().getFullPathName():r.getErrorMessage(),juce::dontSendNotification);category.clear(juce::dontSendNotification);category.addItem("All categories",1);category.addItemList(processor.presets.categories(),2);category.setSelectedId(1,juce::dontSendNotification);refresh();}
    void refresh() {
        updating=true;visible.clear();auto query=search.getText().trim();auto selectedCategory=category.getSelectedId()==1?juce::String():category.getText();
        for(const auto& p:processor.presets.all()) {
            if(selectedCategory.isNotEmpty()&&p.category!=selectedCategory)continue;
            if(favoritesOnly.getToggleState()&&!processor.presets.isFavorite(p.id))continue;
            if(recentOnly.getToggleState()&&!processor.presets.isRecent(p.id))continue;
            if(query.isNotEmpty()&&!(p.name+" "+p.category+" "+p.tags).containsIgnoreCase(query))continue;
            visible.push_back(p.id);
        }
        if(recentOnly.getToggleState())std::stable_sort(visible.begin(),visible.end(),[this](const auto& a,const auto& b){return processor.presets.recentIds().indexOf(a)<processor.presets.recentIds().indexOf(b);});
        list.updateContent();list.deselectAllRows();
        for(std::size_t i=0;i<visible.size();++i)if(visible[i]==processor.presetId())list.selectRow(int(i));
        updateButtons();list.repaint();updating=false;
    }
    void updateButtons() {
        auto* p=processor.presets.find(processor.presetId());rename.setEnabled(p&&!p->factory);remove.setEnabled(p&&!p->factory);
        favorite.setButtonText(processor.presets.isFavorite(processor.presetId())?"UNFAVORITE":"FAVORITE");
    }
    int getNumRows() override {return int(visible.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected) override {
        if(row<0||row>=int(visible.size()))return;
        auto* p=processor.presets.find(visible[std::size_t(row)]);
        if(!p)return;
        g.fillAll(selected?purple.darker(.4f):(row%2?deep.withAlpha(.4f):ink));
        g.setColour(text);g.setFont(font(15,true));g.drawText((processor.presets.isFavorite(p->id)?"* ":"  ")+p->name,10,1,width-330,height-2,juce::Justification::centredLeft,true);
        g.setColour(text.withAlpha(.6f));g.setFont(font(12));g.drawText(p->category+(p->factory?" / FACTORY":" / USER"),width-320,0,310,height,juce::Justification::centredRight,true);
    }
    void selectedRowsChanged(int row) override {if(updating||row<0||row>=int(visible.size()))return;processor.presets.load(visible[std::size_t(row)]);updateButtons();}
    void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override {processor.preview();}
    void returnKeyPressed(int) override {processor.preview();}
    void deleteKeyPressed(int) override {if(remove.isEnabled())remove.triggerClick();}
};
}

ConsoleLook::ConsoleLook() {
    setColour(juce::Label::textColourId,text);setColour(juce::TextButton::textColourOffId,text);setColour(juce::TextButton::textColourOnId,text);
    setColour(juce::Slider::textBoxTextColourId,text);setColour(juce::Slider::textBoxBackgroundColourId,ink);setColour(juce::Slider::textBoxOutlineColourId,purple.withAlpha(.3f));
    setColour(juce::Slider::thumbColourId,purple.brighter());setColour(juce::Slider::trackColourId,purple);setColour(juce::Slider::backgroundColourId,ink);
    setColour(juce::TextEditor::backgroundColourId,ink);setColour(juce::TextEditor::textColourId,text);setColour(juce::TextEditor::outlineColourId,purple.withAlpha(.6f));
    setColour(juce::ComboBox::backgroundColourId,ink);setColour(juce::ComboBox::textColourId,text);setColour(juce::ComboBox::outlineColourId,purple);
    setColour(juce::PopupMenu::backgroundColourId,ink);setColour(juce::PopupMenu::textColourId,text);setColour(juce::ToggleButton::textColourId,text);setColour(juce::ToggleButton::tickColourId,purple.brighter());
}
void ConsoleLook::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                                   float position,float start,float end,juce::Slider& slider) {
    const bool master=slider.getName()=="Master volume";
    const float inset=master?.055f:.13f;
    auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height))
                    .reduced(float(std::min(width,height))*inset);
    const float radius=std::min(bounds.getWidth(),bounds.getHeight())*.5f;
    const float cx=bounds.getCentreX(),cy=bounds.getCentreY();
    bounds={cx-radius,cy-radius,2*radius,2*radius};
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.fillEllipse(bounds.translated(1,5).expanded(3));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xfffaf9f7),cx-radius,cy-radius,
                                          juce::Colour(0xff686b75),cx+radius,cy+radius,false));
    g.fillEllipse(bounds);
    g.setColour(ink);
    g.drawEllipse(bounds.reduced(radius*.11f),std::max(2.0f,radius*.045f));
    auto face=bounds.reduced(radius*.16f);
    g.setGradientFill(juce::ColourGradient(silver,cx,cy-radius,juce::Colour(0xff9a9ca2),cx,cy+radius,false));
    g.fillEllipse(face);
    for(int segment=0;segment<160;++segment) {
        const float angle=float(segment)*juce::MathConstants<float>::twoPi/160;
        juce::Path wedge;
        wedge.addPieSegment(face,angle,angle+.04f,0);
        const float light=.75f+.17f*std::cos(angle*2+.4f)+.06f*std::cos(angle*4+.8f)
                         +.009f*std::cos(angle*53);
        g.setColour(juce::Colour::greyLevel(juce::jlimit(.48f,.98f,light)).withAlpha(.85f));
        g.fillPath(wedge);
    }
    g.setColour(juce::Colours::white.withAlpha(.65f));
    g.drawEllipse(face,1);
    const float angle=start+position*(end-start);
    if(master) {
        auto ringBounds=bounds.reduced(radius*.08f);
        g.setColour(juce::Colour(0xff3f1488));g.drawEllipse(ringBounds,radius*.065f);
        g.setColour(purple.withAlpha(.55f));g.drawEllipse(ringBounds,radius*.035f);
        for(int i=0;i<84;++i) {
            const float at=juce::MathConstants<float>::twoPi*float(i)/84,ledRadius=radius*.92f;
            const float size=std::max(1.6f,radius*.025f);
            g.setColour(juce::Colour(0xffeadbff).withAlpha(.78f));
            g.fillEllipse(cx+std::sin(at)*ledRadius-size*.5f,cy-std::cos(at)*ledRadius-size*.5f,size,size);
        }
    } else {
        for(int i=0;i<29;++i) {
            const float ledAngle=start+(end-start)*float(i)/28,ledRadius=radius*1.09f;
            g.setColour(i<=int(position*28)?juce::Colour(0xffc09aff):purple.withAlpha(.32f));
            g.fillEllipse(cx+std::sin(ledAngle)*ledRadius-1.5f,
                          cy-std::cos(ledAngle)*ledRadius-1.5f,3,3);
        }
    }
    juce::Path arc;
    const float ring=master?.92f:.89f;
    arc.addCentredArc(cx,cy,radius*ring,radius*ring,0,start,angle,true);
    g.setColour(purple.brighter(.55f));
    g.strokePath(arc,juce::PathStrokeType(std::max(1.5f,radius*.025f)));
    g.setColour(ink);
    g.drawLine(cx+std::sin(angle)*radius*.57f,cy-std::cos(angle)*radius*.57f,
               cx+std::sin(angle)*radius*.75f,cy-std::cos(angle)*radius*.75f,
               std::max(2.5f,radius*.045f));
    if(slider.hasKeyboardFocus(false)) {
        g.setColour(juce::Colour(0xffe0caff));g.drawEllipse(bounds.expanded(2),1.5f);
    }
}
void ConsoleLook::drawButtonBackground(juce::Graphics& g,juce::Button& button,
                                       const juce::Colour&,bool over,bool down) {
    auto bounds=button.getLocalBounds().toFloat().reduced(3);
    const bool round=button.getProperties()["round"];
    if(button.getButtonText()=="START"||button.getButtonText()=="SELECT") {
        g.setColour(ink);g.fillRoundedRectangle(bounds,bounds.getHeight()*.5f);
        g.setColour(purple.withAlpha(.35f));g.drawRoundedRectangle(bounds,bounds.getHeight()*.5f,1);
        auto cap=juce::Rectangle<float>(bounds.getX()+3,bounds.getY()+bounds.getHeight()*.18f+(down?1.5f:0.0f),
                                         bounds.getWidth()*.38f,bounds.getHeight()*.64f);
        g.setColour(juce::Colours::black);g.fillRoundedRectangle(cap.expanded(2),cap.getHeight()*.5f);
        g.setGradientFill(juce::ColourGradient(down?silver:juce::Colours::white,cap.getX(),cap.getY(),
                                              juce::Colour(0xff8c9098),cap.getX(),cap.getBottom(),false));
        g.fillRoundedRectangle(cap,cap.getHeight()*.5f);
        g.setColour(juce::Colours::white.withAlpha(over?.55f:.25f));g.drawRoundedRectangle(cap.reduced(1),cap.getHeight()*.5f,1);
        if(button.hasKeyboardFocus(false)){g.setColour(text);g.drawRoundedRectangle(bounds.reduced(1),bounds.getHeight()*.5f,1);}
        return;
    }
    g.setColour(juce::Colours::black.withAlpha(.8f));
    if(round) {
        g.fillEllipse(bounds.translated(0,4).expanded(2));
        g.setGradientFill(juce::ColourGradient(silver,bounds.getX(),bounds.getY(),
                                              juce::Colour(0xff5d6169),bounds.getRight(),bounds.getBottom(),false));
        g.fillEllipse(bounds);
        auto face=bounds.reduced(bounds.getWidth()*.055f);
        auto light=down?juce::Colour(0xffa6a8ac):juce::Colour(0xfff0efeb);
        auto dark=down?silver:juce::Colour(0xffaaaeb4);
        if(over)light=light.brighter(.08f);
        g.setGradientFill(juce::ColourGradient(light,face.getCentreX(),face.getY(),dark,
                                              face.getCentreX(),face.getBottom(),false));
        g.fillEllipse(face);
        g.setColour(juce::Colours::white.withAlpha(.65f));
        g.drawEllipse(face,1.5f);
        g.setColour(ink.withAlpha(.55f));g.drawEllipse(bounds.reduced(.5f),1.5f);
        if(button.hasKeyboardFocus(false)){g.setColour(text);g.drawEllipse(bounds.expanded(1),1.5f);}
        return;
    }
    g.fillRoundedRectangle(bounds.translated(0,2),7);
    auto light=juce::Colour(0xff29364d),dark=ink;
    if(button.getName()=="Preset display")light=juce::Colour(0xff18233c);
    if(down)std::swap(light,dark);
    if(over)light=light.brighter(.1f);
    g.setGradientFill(juce::ColourGradient(light,bounds.getX(),bounds.getY(),dark,
                                          bounds.getRight(),bounds.getBottom(),false));
    g.fillRoundedRectangle(bounds,7);
    g.setColour(purple.withAlpha(.65f));
    g.drawRoundedRectangle(bounds,7,1);
    g.setColour(juce::Colours::white.withAlpha(.12f));g.drawLine(bounds.getX()+8,bounds.getY()+2,bounds.getRight()-8,bounds.getY()+2,1);
    if(button.hasKeyboardFocus(false)){g.setColour(text);g.drawRoundedRectangle(bounds.reduced(1),6,1);}
}
void ConsoleLook::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) {
    bool round=button.getProperties()["round"];g.setColour((round?ink:text).withAlpha(button.isEnabled()?1.0f:.35f));
    if(button.getName()=="Preset display") {gba_ui::pixelText(g,button.getButtonText(),button.getLocalBounds().toFloat().reduced(12,10),float(button.getHeight())*.043f);return;}
    if(button.getButtonText()=="START"||button.getButtonText()=="SELECT") {
        auto label=button.getLocalBounds().toFloat();label.removeFromLeft(label.getWidth()*.45f);
        gba_ui::pixelText(g,button.getButtonText(),label,float(button.getHeight())*.05f);return;
    }
    if(button.getButtonText()=="<"||button.getButtonText()==">") {
        const auto centre=button.getLocalBounds().toFloat().getCentre();
        const float radius=float(button.getHeight())*.23f;
        const float direction=button.getButtonText()==">"?1.0f:-1.0f;
        juce::Path arrow;arrow.addTriangle(centre.x+radius*direction,centre.y,
            centre.x-radius*direction,centre.y-radius,centre.x-radius*direction,centre.y+radius);
        g.setGradientFill(juce::ColourGradient(juce::Colours::white,centre.x,centre.y-radius,
                                              silver.darker(.25f),centre.x,centre.y+radius,false));
        g.fillPath(arrow);return;
    }
    if(round)g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(),float(button.getHeight())*.43f,juce::Font::bold)));
    else g.setFont(font(std::min(18.0f,float(button.getHeight())*.43f),true));
    g.drawText(button.getButtonText(),button.getLocalBounds().reduced(8),juce::Justification::centred,true);
}
void ConsoleLook::drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,float position,float min,float max,juce::Slider::SliderStyle style,juce::Slider& slider) {
    if(slider.getName()!="Pitch wheel") {LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,position,min,max,style,slider);return;}
    auto b=juce::Rectangle<float>(float(x)+float(width)*.2f,float(y),float(width)*.6f,float(height));
    g.setColour(ink);g.fillRoundedRectangle(b.expanded(7),6);
    juce::ColourGradient roller(juce::Colour(0xffa070ff),b.getX(),b.getY(),juce::Colour(0xff050916),b.getRight(),b.getY(),false);
    roller.addColour(.12,purple);roller.addColour(.2,juce::Colour(0xff14152c));roller.addColour(.55,juce::Colour(0xff343143));roller.addColour(.86,juce::Colour(0xff10111e));roller.addColour(.95,purple.darker(.35f));
    g.setGradientFill(roller);g.fillRoundedRectangle(b,5);
    const float phase=std::fmod(position*.17f,5.0f);
    for(float at=b.getY()+4+phase;at<b.getBottom()-2;at+=5) {
        g.setColour(juce::Colours::black.withAlpha(.7f));g.drawLine(b.getX()+8,at,b.getRight()-7,at,2);
        g.setColour(juce::Colour(0xffa2a1ad).withAlpha(.35f));g.drawLine(b.getX()+8,at+1.5f,b.getRight()-7,at+1.5f,1);
    }
    g.setColour(juce::Colour(0xffd4c2ff));g.fillRoundedRectangle(b.getRight()-5,juce::jlimit(b.getY()+2,b.getBottom()-4,position-1),3,3,1);
    if(slider.hasKeyboardFocus(false)){g.setColour(text);g.drawRoundedRectangle(b.expanded(3),6,1);}
}
DirectionPad::DirectionPad() {
    setWantsKeyboardFocus(true);setInterceptsMouseClicks(false,true);
    setTitle("Preset directional pad");setDescription("Left and right select presets. Up and down select categories.");
    const char* names[]{"Previous preset","Next preset","Previous preset category","Next preset category"};
    for(std::size_t i=0;i<directions.size();++i) {
        directions[i]=std::make_unique<DirectionButton>(names[i]);
        directions[i]->onClick=[this,i]{if(onDirection)onDirection(int(i));};
        directions[i]->onStateChange=[this]{repaint();};
        addAndMakeVisible(*directions[i]);
    }
}
void DirectionPad::paint(juce::Graphics& g) {
    auto b=getLocalBounds().toFloat().reduced(7);float w=b.getWidth(),h=b.getHeight();
    juce::Path path;path.startNewSubPath(b.getX()+w*.34f,b.getY());path.lineTo(b.getX()+w*.66f,b.getY());path.lineTo(b.getX()+w*.66f,b.getY()+h*.34f);path.lineTo(b.getRight(),b.getY()+h*.34f);path.lineTo(b.getRight(),b.getY()+h*.66f);path.lineTo(b.getX()+w*.66f,b.getY()+h*.66f);path.lineTo(b.getX()+w*.66f,b.getBottom());path.lineTo(b.getX()+w*.34f,b.getBottom());path.lineTo(b.getX()+w*.34f,b.getY()+h*.66f);path.lineTo(b.getX(),b.getY()+h*.66f);path.lineTo(b.getX(),b.getY()+h*.34f);path.lineTo(b.getX()+w*.34f,b.getY()+h*.34f);path.closeSubPath();
    path=path.createPathWithRoundedCorners(w*.045f);
    g.setColour(ink);g.strokePath(path,juce::PathStrokeType(14,juce::PathStrokeType::curved));
    g.setGradientFill(juce::ColourGradient(silver,b.getX(),b.getY(),juce::Colour(0xff92969c),b.getRight(),b.getBottom(),false));g.fillPath(path);g.setColour(juce::Colours::white.withAlpha(.65f));g.strokePath(path,juce::PathStrokeType(3,juce::PathStrokeType::curved));
    juce::Rectangle<float> areas[]{ {b.getX(),b.getY()+h*.34f,w*.34f,h*.32f},{b.getX()+w*.66f,b.getY()+h*.34f,w*.34f,h*.32f},{b.getX()+w*.34f,b.getY(),w*.32f,h*.34f},{b.getX()+w*.34f,b.getY()+h*.66f,w*.32f,h*.34f}};
    for(int i=0;i<4;++i) {
        const auto centre=areas[i].getCentre();
        const float size=w*.06f;
        juce::Path arrow;
        if(i==0)arrow.addTriangle(centre.x-size,centre.y,centre.x+size,centre.y-size,centre.x+size,centre.y+size);
        if(i==1)arrow.addTriangle(centre.x+size,centre.y,centre.x-size,centre.y-size,centre.x-size,centre.y+size);
        if(i==2)arrow.addTriangle(centre.x,centre.y-size,centre.x-size,centre.y+size,centre.x+size,centre.y+size);
        if(i==3)arrow.addTriangle(centre.x,centre.y+size,centre.x-size,centre.y-size,centre.x+size,centre.y-size);
        const auto& key=*directions[std::size_t(i)];
        g.setColour(key.isDown()?purple:(key.isOver()?juce::Colour(0xff242847):juce::Colour(0xff62656b)));
        g.fillPath(arrow);
        if(key.hasKeyboardFocus(false)) {
            g.setColour(purple);g.drawRoundedRectangle(areas[i].reduced(4),4,1.5f);
        }
    }
}
void DirectionPad::resized() {
    auto b=getLocalBounds().reduced(7);int w=b.getWidth(),h=b.getHeight();
    directions[0]->setBounds(b.getX(),b.getY()+h/3,w/3,h/3);
    directions[1]->setBounds(b.getRight()-w/3,b.getY()+h/3,w/3,h/3);
    directions[2]->setBounds(b.getX()+w/3,b.getY(),w/3,h/3);
    directions[3]->setBounds(b.getX()+w/3,b.getBottom()-h/3,w/3,h/3);
}
bool DirectionPad::keyPressed(const juce::KeyPress& key) {int d=key==juce::KeyPress::leftKey?0:key==juce::KeyPress::rightKey?1:key==juce::KeyPress::upKey?2:key==juce::KeyPress::downKey?3:-1;if(d<0)return false;if(onDirection)onDirection(d);return true;}

void ConsoleKeyboard::drawWhiteNote(int note,juce::Graphics& g,juce::Rectangle<float> area,
                                   bool down,bool over,juce::Colour,juce::Colour labelColour) {
    auto key=area.reduced(.6f,0).withTrimmedBottom(2);
    auto light=down?juce::Colour(0xffb292ec):juce::Colour(0xfff9f8f4);
    auto shade=down?juce::Colour(0xff8162bc):juce::Colour(0xffd6d7d9);
    if(over&&!down)light=juce::Colour(0xffe0ceff);
    juce::ColourGradient surface(light,key.getX(),key.getY(),shade,key.getRight(),key.getY(),false);
    surface.addColour(.22,light);surface.addColour(.88,light.darker(.04f));
    g.setGradientFill(surface);g.fillRoundedRectangle(key,2);
    g.setColour(juce::Colours::black.withAlpha(.22f));g.drawRoundedRectangle(key,2,1);
    g.setColour(juce::Colours::white.withAlpha(.6f));g.drawLine(key.getX()+2,key.getBottom()-2,key.getRight()-2,key.getBottom()-2,1);
    // Octave labels appear on hover/focus, leaving the reference-style key bed uncluttered.
    if(note%12==0&&(hasKeyboardFocus(false)||isMouseOver(true))) {
        g.setColour(labelColour);g.setFont(juce::Font(juce::FontOptions(std::min(11.0f,getKeyWidth()*.5f))));
        g.drawText(getWhiteNoteText(note),key.reduced(1),juce::Justification::centredBottom,false);
    }
}
void ConsoleKeyboard::drawBlackNote(int,juce::Graphics& g,juce::Rectangle<float> area,
                                   bool down,bool over,juce::Colour) {
    auto key=area.reduced(.5f,0).withTrimmedBottom(2);
    g.setColour(juce::Colour(0xff02040d));g.fillRoundedRectangle(key,2);
    auto face=key.reduced(std::max(1.0f,key.getWidth()*.12f),0).withTrimmedBottom(6);
    auto light=down?juce::Colour(0xff6740a0):(over?juce::Colour(0xff393056):juce::Colour(0xff222836));
    g.setGradientFill(juce::ColourGradient(light,face.getX(),face.getY(),juce::Colour(0xff090c19),face.getRight(),face.getBottom(),false));
    g.fillRoundedRectangle(face,1.5f);
    g.setColour(juce::Colour(0xff667087).withAlpha(.5f));g.drawLine(face.getX(),face.getY()+1,face.getX(),face.getBottom(),1);
    g.drawLine(face.getX(),face.getBottom(),face.getRight(),face.getBottom(),1);
}

GbaEditor::GbaEditor(GbaProcessor& p):AudioProcessorEditor(p),processor(p),keyboard(p.keyboard) {
    setLookAndFeel(&look);setOpaque(true);setWantsKeyboardFocus(true);
    for(auto* c:std::initializer_list<juce::Component*>{&keyboard,&previous,&next,&presetDisplay,&start,&select,&aButton,&bButton,&dpad,&volume,&glide,&pitch})addAndMakeVisible(c);
    rotary(volume,"Master volume");rotary(glide,"Glide");
    volume.setTooltip("Master output volume");glide.setTooltip("Glide / portamento time. Voice mode is under SELECT.");
    for(std::size_t i=0;i<macros.size();++i){rotary(macros[i],"Macro "+juce::String::charToString(char('A'+i)));addAndMakeVisible(macros[i]);}
    volumeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"volume",volume);
    glideAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"voice.glide",glide);
    volume.setDoubleClickReturnValue(true,.72);glide.setDoubleClickReturnValue(true,0);
    pitch.setName("Pitch wheel");pitch.setSliderStyle(juce::Slider::LinearVertical);pitch.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);pitch.setRange(0,16383,1);pitch.setValue(8192,juce::dontSendNotification);pitch.setTooltip("Pitch bend. Release to return to center.");
    pitch.onValueChange=[this]{processor.uiPitch(int(pitch.getValue()));};pitch.onDragEnd=[this]{pitch.setValue(8192,juce::sendNotificationSync);};pitch.setDoubleClickReturnValue(true,8192);
    previous.setTooltip("Previous preset");next.setTooltip("Next preset");previous.onClick=[this]{processor.presets.step(-1);};next.onClick=[this]{processor.presets.step(1);};
    previous.setTitle("Previous preset");next.setTitle("Next preset");
    presetDisplay.setName("Preset display");presetDisplay.onClick=[this]{showBrowser();};start.setTooltip("Open preset browser (F1)");select.setTooltip("Open advanced synthesis (F2)");start.onClick=[this]{showBrowser();};select.onClick=[this]{showAdvanced();};
    aButton.setTooltip("Preview current patch with a short synthesized C4");bButton.setTooltip("Stop preview; DAW MIDI notes continue");aButton.onClick=[this]{processor.preview();};bButton.onClick=[this]{processor.stopPreview();};
    aButton.setTitle("Preview current preset");bButton.setTitle("Stop preset preview");
    dpad.onDirection=[this](int direction){if(direction<2)processor.presets.step(direction==0?-1:1);else processor.presets.stepCategory(direction==2?-1:1);};
    keyboard.setName("Piano keyboard");keyboard.setOctaveForMiddleC(4);keyboard.setAvailableRange(24,108);keyboard.setLowestVisibleKey(48);keyboard.setKeyPressBaseOctave(4);keyboard.setScrollButtonsVisible(true);
    keyboard.setDescription("Playable piano. Click or drag keys, or focus here to use the computer keyboard. Side arrows scroll octaves.");
    keyboard.setBlackNoteLengthProportion(.63f);keyboard.setBlackNoteWidthProportion(.63f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffeeeef0));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,ink);
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,purple.withAlpha(.7f));keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,purple.withAlpha(.3f));keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId,juce::Colour(0xff4b4b63));
    setResizable(true,true);setResizeLimits(960,485,1920,970);getConstrainer()->setFixedAspectRatio(1280.0/646.0);
    int w=processor.editorWidth.load();setSize(w,int(std::round(double(w)*646/1280)));keyboard.setLowestVisibleKey(48);updateMacros();timerCallback();startTimerHz(25);
}
GbaEditor::~GbaEditor() {stopTimer();processor.uiPitch(8192);processor.keyboard.allNotesOff(0);overlay.reset();setLookAndFeel(nullptr);}
void GbaEditor::paint(juce::Graphics& g) {
    g.addTransform(juce::AffineTransform::scale(float(getWidth())/1280.0f,float(getHeight())/646.0f));
    g.fillAll(juce::Colour(0xff11143d));
    auto body=juce::Rectangle<float>(2,2,1276,642);
    juce::ColourGradient bodyColour(juce::Colour(0xff3032a3),310,210,juce::Colour(0xff111552),1210,520,true);
    bodyColour.addColour(.5,juce::Colour(0xff24267f));g.setGradientFill(bodyColour);g.fillRoundedRectangle(body,22);
    // Deterministic moulded-plastic pores, drawn as vectors rather than a reference-image layer.
    juce::Random pores(0x47424132);
    for(int i=0;i<310;++i) {
        float x=12+pores.nextFloat()*1256,y=95+pores.nextFloat()*360;
        float size=.5f+pores.nextFloat()*2;
        g.setColour(ink.withAlpha(.11f));g.fillEllipse(x,y,size,size);
        g.setColour(juce::Colours::white.withAlpha(.035f));g.fillEllipse(x,y+size*.6f,size,size*.5f);
    }
    // Glossy, angular upper housing follows the approved wide rectangular reference.
    juce::Path shell;shell.startNewSubPath(0,0);shell.lineTo(270,0);shell.lineTo(285,25);shell.lineTo(995,25);shell.lineTo(1010,0);shell.lineTo(1280,0);shell.lineTo(1280,95);shell.lineTo(1165,95);shell.lineTo(1116,148);shell.lineTo(1089,161);shell.lineTo(1084,191);shell.lineTo(194,191);shell.lineTo(187,163);shell.lineTo(157,151);shell.lineTo(112,95);shell.lineTo(0,95);shell.closeSubPath();
    g.setColour(juce::Colours::black.withAlpha(.5f));
    g.strokePath(shell,juce::PathStrokeType(12),juce::AffineTransform::translation(0,5));
    juce::ColourGradient plastic(juce::Colour(0xffbb96ff),0,0,juce::Colour(0xff6534db),0,195,false);
    plastic.addColour(.08,juce::Colour(0xffa477ff));plastic.addColour(.14,juce::Colour(0xff8450ec));
    plastic.addColour(.76,juce::Colour(0xff7d3df1));plastic.addColour(.92,juce::Colour(0xff9861ff));
    g.setGradientFill(plastic);g.fillPath(shell);
    g.setColour(juce::Colour(0xff3b147d));g.strokePath(shell,juce::PathStrokeType(9));
    g.setColour(juce::Colour(0xffa97aff));g.strokePath(shell,juce::PathStrokeType(5));
    g.setColour(juce::Colour(0xffe1c6ff).withAlpha(.8f));g.strokePath(shell,juce::PathStrokeType(1.5f),juce::AffineTransform::translation(0,-2));
    g.setColour(juce::Colours::white.withAlpha(.25f));g.drawLine(12,12,251,12,4);g.drawLine(1028,12,1268,12,4);
    panel(g,{255,-12,770,117},26);panel(g,{342,17,596,77},16);
    // Original typography and branding, without manufacturer marks from real consoles.
    auto logo=gba_ui::blockLettering("G BOY",{397,125,200,43});
    juce::GlyphArrangement smallTitle;
    smallTitle.addFittedText(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(),43,juce::Font::bold)),
                            "ADVANCE",603,126,207,44,juce::Justification::centred,1);
    juce::Path advance;smallTitle.createPath(advance);
    logo.addPath(advance);
    g.setColour(ink);g.strokePath(logo,juce::PathStrokeType(9,juce::PathStrokeType::curved));
    g.setColour(juce::Colours::black.withAlpha(.7f));g.fillPath(logo,juce::AffineTransform::translation(1,5));
    g.setGradientFill(juce::ColourGradient(juce::Colours::white,0,125,silver.darker(.1f),0,170,false));g.fillPath(logo);
    auto numberPath=gba_ui::blockLettering("2",{825,122,54,55});
    g.setColour(juce::Colours::black.withAlpha(.65f));g.strokePath(numberPath,juce::PathStrokeType(10),juce::AffineTransform::translation(2,4));
    g.setColour(ink);g.strokePath(numberPath,juce::PathStrokeType(8));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd2b7ff),0,104,purple,0,181,false));g.fillPath(numberPath);
    panel(g,{607,177,66,19},8);caption(g,"VST",{609,177,62,18},12);
    const auto led=[&](float x,juce::Colour colour,bool active) {
        g.setColour(ink);g.fillEllipse(x-14,43,28,28);g.setColour(colour.withAlpha(active?.35f:.08f));g.fillEllipse(x-12,45,24,24);
        g.setGradientFill(juce::ColourGradient(active?juce::Colours::white:colour.darker(),x-3,49,colour.darker(.2f),x+7,66,false));g.fillEllipse(x-8,49,16,16);
    };
    led(300,juce::Colour(0xff64ff36),signalLed);led(979,juce::Colour(0xffffb733),true);
    // D-pad recess and speaker / fastener details are vector decoration.
    g.setColour(ink.withAlpha(.3f));g.fillEllipse(35,172,191,181);g.setColour(purple.withAlpha(.2f));g.drawEllipse(35,172,191,181,2);
    panel(g,{278,307,725,100},46);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff35439e),0,405,juce::Colour(0xff111856),0,440,false));g.fillRoundedRectangle(307,390,664,51,13);
    g.setColour(purple.withAlpha(.32f));g.drawRoundedRectangle({307,390,664,51},13,1);
    panel(g,{548,224,184,30},7);caption(g,"VOLUME",{552,224,176,29},18);
    for(int i=0;i<4;++i)caption(g,juce::String::charToString(char('A'+i)),{float(i<2?340+i*107:778+(i-2)*107),407,54,30},19);
    for(int i=0;i<4;++i) {
        float y=381+float(i)*16;
        juce::Path slit;slit.startNewSubPath(1078,y);slit.lineTo(1220,y-24);
        g.setColour(juce::Colour(0xff010317));g.strokePath(slit,juce::PathStrokeType(8,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(juce::Colour(0xff6c52ca).withAlpha(.65f));g.strokePath(slit,juce::PathStrokeType(2),juce::AffineTransform::translation(0,5));
    }
    for(auto point:std::initializer_list<juce::Point<float>>{{26,57},{1254,57},{30,446},{1250,446}}) {
        g.setColour(ink.withAlpha(.6f));g.fillEllipse(point.x-7,point.y-7,14,14);g.setColour(purple.withAlpha(.4f));g.drawEllipse(point.x-7,point.y-7,14,14,1);
    }
    // Lower chassis and the three inset control wells.
    juce::Path lower;lower.startNewSubPath(0,461);lower.lineTo(30,455);lower.lineTo(1250,455);lower.lineTo(1280,467);lower.lineTo(1280,646);lower.lineTo(0,646);lower.closeSubPath();
    juce::ColourGradient lowerPlastic(juce::Colour(0xffb187ff),0,455,juce::Colour(0xff5630bd),0,646,false);
    lowerPlastic.addColour(.07,juce::Colour(0xff824ff3));lowerPlastic.addColour(.15,juce::Colour(0xff6238cf));lowerPlastic.addColour(.85,juce::Colour(0xff5330b3));lowerPlastic.addColour(.98,juce::Colour(0xff9467ef));
    g.setGradientFill(lowerPlastic);g.fillPath(lower);
    g.setColour(juce::Colour(0xff3e1b8a));g.strokePath(lower,juce::PathStrokeType(11));
    g.setColour(juce::Colour(0xff9865ef));g.strokePath(lower,juce::PathStrokeType(6));
    g.setColour(juce::Colour(0xffd4b3ff));g.strokePath(lower,juce::PathStrokeType(2),juce::AffineTransform::translation(0,-2));
    panel(g,{46,485,131,140},12);panel(g,{194,489,890,140},7);panel(g,{1102,485,129,140},12);
    caption(g,"PITCH",{70,489,84,25},15);caption(g,"GLIDE",{1117,489,99,25},15);
    g.setColour(juce::Colour(0xffb789ff).withAlpha(.6f));g.drawRoundedRectangle(body,20,3);
}
void GbaEditor::resized() {
    const auto firstKey=keyboard.getLowestVisibleKey();
    float sx=float(getWidth())/1280,sy=float(getHeight())/646;
    auto bounds=[sx,sy](juce::Component& component,float x,float y,float w,float h){component.setBounds(int(std::round(x*sx)),int(std::round(y*sy)),int(std::round(w*sx)),int(std::round(h*sy)));};
    bounds(previous,350,28,68,56);bounds(presetDisplay,418,28,444,56);bounds(next,862,28,68,56);
    bounds(dpad,55,183,160,160);bounds(start,76,375,130,30);bounds(select,76,412,130,30);
    bounds(volume,538,258,204,198);bounds(macros[0],325,320,85,85);bounds(macros[1],432,320,85,85);bounds(macros[2],762,320,85,85);bounds(macros[3],869,320,85,85);
    bounds(aButton,1162,198,96,96);bounds(bButton,1059,242,96,96);
    bounds(keyboard,203,497,870,128);keyboard.setKeyWidth(28*sx);keyboard.setLowestVisibleKey(firstKey);bounds(pitch,73,522,77,89);bounds(glide,1122,516,90,90);
    if(overlay)bounds(*overlay,20,12,1240,455);
    processor.editorWidth.store(getWidth());
}
void GbaEditor::updateMacros() {
    const gba::Param assignments[]{gba::Param::MacroA,gba::Param::MacroB,gba::Param::MacroC,gba::Param::MacroD};
    for(int i=0;i<4;++i) {
        int selected=int(processor.state.getRawParameterValue(gba::specs[gba::index(assignments[i])].id.data())->load());selected=juce::jlimit(0,7,selected);
        if(mapping[std::size_t(i)]==selected)continue;
        macroAttachments[std::size_t(i)].reset();mapping[std::size_t(i)]=selected;
        const auto& spec=gba::specs[gba::index(gba::macroTargets[std::size_t(selected)])];
        macroAttachments[std::size_t(i)]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,spec.id.data(),macros[std::size_t(i)]);
        macros[std::size_t(i)].setTooltip(juce::String::charToString(char('A'+i))+": "+spec.name.data()+" (reassign in SELECT)");
        macros[std::size_t(i)].setTitle("Macro "+juce::String::charToString(char('A'+i))+": "+spec.name.data());
        macros[std::size_t(i)].setDoubleClickReturnValue(true,spec.initial);
    }
}
void GbaEditor::timerCallback() {
    presetDisplay.setButtonText(processor.presetName().toUpperCase());presetDisplay.setTooltip(processor.presetCategory()+" / "+processor.presetName()+" / Click to browse");updateMacros();
    const bool lit=processor.outputLevel()>.001f;
    if(lit!=signalLed) {signalLed=lit;repaint(juce::Rectangle<float>(284,41,32,32).transformedBy(juce::AffineTransform::scale(float(getWidth())/1280,float(getHeight())/646)).getSmallestIntegerContainer());}
}
void GbaEditor::setMainControlsEnabled(bool enabled) {
    for(auto* c:std::initializer_list<juce::Component*>{&previous,&next,&presetDisplay,&start,&select,&aButton,&bButton,&dpad,&volume})c->setEnabled(enabled);
    for(auto& macro:macros)macro.setEnabled(enabled);
}
void GbaEditor::showBrowser() {
    if(dynamic_cast<BrowserPanel*>(overlay.get())){closeOverlay();return;}
    overlay=std::make_unique<BrowserPanel>(processor,[this]{closeOverlay();});addAndMakeVisible(*overlay);setMainControlsEnabled(false);resized();overlay->toFront(true);
}
void GbaEditor::showAdvanced() {
    if(dynamic_cast<AdvancedPanel*>(overlay.get())){closeOverlay();return;}
    overlay=std::make_unique<AdvancedPanel>(processor,[this]{closeOverlay();});addAndMakeVisible(*overlay);setMainControlsEnabled(false);resized();overlay->toFront(true);
}
void GbaEditor::closeOverlay() {overlay.reset();setMainControlsEnabled(true);grabKeyboardFocus();}
bool GbaEditor::keyPressed(const juce::KeyPress& key) {
    if(key==juce::KeyPress::escapeKey&&overlay){closeOverlay();return true;}
    if(key==juce::KeyPress::F1Key){showBrowser();return true;}if(key==juce::KeyPress::F2Key){showAdvanced();return true;}
    if(overlay)return false;
    if(key==juce::KeyPress::spaceKey){processor.preview();return true;}
    return dpad.keyPressed(key);
}
