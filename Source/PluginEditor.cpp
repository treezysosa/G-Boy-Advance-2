#include "PluginEditor.h"
#include "PixelText.h"

namespace {
const juce::Colour silver {0xffe6e5e5}, text {0xffeeeeff};
juce::Colour uiColour(const juce::Component& owner,juce::uint32 argb) {
    const auto colour=juce::Colour(argb);
    if(auto* look=dynamic_cast<ConsoleLook*>(&owner.getLookAndFeel()))return look->colour(colour);
    return colour;
}
juce::Font font(float height,bool bold=false) {
#if JUCE_WINDOWS
    const juce::String family="Consolas";
#else
    const auto family=juce::Font::getDefaultMonospacedFontName();
#endif
    return juce::Font(juce::FontOptions(family,height,bold?juce::Font::bold:juce::Font::plain));
}
void panel(const juce::Component& owner,juce::Graphics& g,juce::Rectangle<float> bounds,float radius) {
    g.setColour(juce::Colours::black.withAlpha(.55f));g.fillRoundedRectangle(bounds.expanded(2).translated(0,2),radius+2);
    juce::ColourGradient surface(uiColour(owner,0xff263179),bounds.getX(),bounds.getY(),uiColour(owner,0xff080c25),bounds.getRight(),bounds.getBottom(),false);
    surface.addColour(.35,uiColour(owner,0xff11194c));g.setGradientFill(surface);g.fillRoundedRectangle(bounds,radius);
    g.setColour(uiColour(owner,0xffb792ff).withAlpha(.38f));g.drawRoundedRectangle(bounds.reduced(.5f),radius,1.5f);
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
class SoundFontPanel final : public juce::Component,private juce::Timer {
public:
    SoundFontPanel(GbaProcessor& p,std::function<void()> close):processor(p){
        setName("ROM Hacks browser");setWantsKeyboardFocus(true);processor.romHacks.startLibrary();
        for(auto* c:std::initializer_list<juce::Component*>{&load,&back,&instruments,&file,&status,&preview,&stop,&synth,&enable,&bankFiles,&folder,&openFolder,&chooseFolder,&refreshFolder,&remember,&libraryStatus})addAndMakeVisible(c);
        instruments.setName("SoundFont instrument");file.setName("SoundFont file");status.setName("SoundFont status");bankFiles.setName("SoundFont bank");folder.setName("SoundFont folder");libraryStatus.setName("SoundFont library status");remember.setName("Remember last SoundFont");
        file.setFont(font(16,true));status.setFont(font(14));folder.setFont(font(12));libraryStatus.setFont(font(12));
        load.onClick=[this]{choose(false);};back.onClick=[this,close]{asyncClose(this,close);};
        instruments.onChange=[this]{processor.romHacks.select(instruments.getSelectedId()-1);refresh();};
        bankFiles.onChange=[this]{const int index=bankFiles.getSelectedId()-1;if(index>=0&&index<shownFiles.size())processor.romHacks.loadFile(juce::File(shownFiles[index]));refresh();};
        openFolder.onClick=[this]{processor.romHacks.libraryView().folder.revealToUser();};
        chooseFolder.onClick=[this]{choose(true);};refreshFolder.onClick=[this]{processor.romHacks.refreshLibrary();refresh();};
        remember.setTooltip("Restore the last loaded bank and instrument in new instances. Saved DAW projects keep their own selection.");
        remember.onClick=[this]{processor.romHacks.setRememberLast(remember.getToggleState());};
        preview.onClick=[this]{processor.preview();};stop.onClick=[this]{processor.stopPreview();};
        synth.onClick=[this]{processor.romHacks.useSoundFont(false);processor.panic();refresh();};
        enable.onClick=[this]{processor.romHacks.useSoundFont(true);refresh();};
        refresh();startTimerHz(10);
    }
    ~SoundFontPanel() override{stopTimer();}
    void paint(juce::Graphics& g) override{
        panel(*this,g,getLocalBounds().toFloat(),14);caption(g,"ROM HACKS",{22,15,float(getWidth()-300),28},25);
        const float sy=float(getHeight())/455;
        g.setColour(text.withAlpha(.8f));g.setFont(font(std::max(12.0f,15*sy)));
        g.drawFittedText("Choose a bank from your SoundFont folder, drop one .sf2 anywhere, or use LOAD SF2.",24,int(57*sy),getWidth()-48,int(31*sy),juce::Justification::centredLeft,2);
    }
    void resized() override{
        const int w=getWidth();const float sy=float(getHeight())/455;
        auto place=[sy](juce::Component& c,int x,int y,int width,int height){c.setBounds(x,int(float(y)*sy),width,int(float(height)*sy));};
        place(back,w-245,14,220,30);place(folder,24,96,w-480,28);place(openFolder,w-445,96,130,28);place(chooseFolder,w-305,96,165,28);place(refreshFolder,w-130,96,105,28);
        place(bankFiles,24,140,w-250,34);place(load,w-205,140,180,34);place(file,24,183,w-48,28);place(instruments,24,220,w-48,34);
        place(preview,24,270,135,30);place(stop,170,270,105,30);place(enable,300,270,185,30);place(synth,w-245,270,220,30);
        place(remember,24,315,w-48,28);place(status,24,353,w-48,58);place(libraryStatus,24,419,w-48,24);
    }
private:
    GbaProcessor& processor;juce::TextButton load{"LOAD SF2"},back{"BACK / ROM HACKS"},preview{"A  PREVIEW"},stop{"B  STOP"},synth{"RETURN TO SYNTH"},enable{"USE SOUNDFONT"},openFolder{"OPEN FOLDER"},chooseFolder{"CHOOSE FOLDER"},refreshFolder{"REFRESH"};
    juce::ComboBox instruments,bankFiles;juce::Label file,status,folder,libraryStatus;juce::ToggleButton remember{"REMEMBER LAST BANK"};juce::StringArray shown,shownFiles;std::unique_ptr<juce::FileChooser> chooser;
    void choose(bool directory){
        const auto initial=processor.romHacks.libraryView().folder;
        chooser=std::make_unique<juce::FileChooser>(directory?"Choose your SoundFont folder":"Load a SoundFont 2 bank",initial,directory?"":"*.sf2",true);
        juce::Component::SafePointer<SoundFontPanel> safe(this);
        chooser->launchAsync(juce::FileBrowserComponent::openMode|(directory?juce::FileBrowserComponent::canSelectDirectories:juce::FileBrowserComponent::canSelectFiles),[safe,directory](const juce::FileChooser& dialog){if(safe!=nullptr&&dialog.getResult()!=juce::File()){if(directory)safe->processor.romHacks.setLibraryFolder(dialog.getResult());else safe->processor.romHacks.loadFile(dialog.getResult());safe->refresh();}});
    }
    void timerCallback() override{refresh();}
    void refresh(){const auto info=processor.romHacks.view();const auto library=processor.romHacks.libraryView();
        if(shown!=info.instruments){shown=info.instruments;instruments.clear(juce::dontSendNotification);instruments.addItemList(shown,1);}
        if(shownFiles!=library.files){shownFiles=library.files;bankFiles.clear(juce::dontSendNotification);bankFiles.addItemList(library.names,1);}
        const int selected=library.files.indexOf(library.source);bankFiles.setSelectedId(info.enabled&&selected>=0?selected+1:0,juce::dontSendNotification);
        bankFiles.setTextWhenNothingSelected(library.scanning?"Scanning SoundFont folder...":"Choose a bank from your SoundFont folder");bankFiles.setEnabled(!library.scanning&&!library.files.isEmpty());
        folder.setText(library.folder.getFullPathName(),juce::dontSendNotification);folder.setTooltip(folder.getText());libraryStatus.setText(library.status,juce::dontSendNotification);libraryStatus.setTooltip(library.status);
        remember.setToggleState(library.rememberLast,juce::dontSendNotification);refreshFolder.setEnabled(!library.scanning);
        instruments.setSelectedId(shown.isEmpty()?0:info.selected+1,juce::dontSendNotification);
        instruments.setTextWhenNothingSelected("Load an .sf2 file to see its instruments");instruments.setEnabled(!info.loading&&!shown.isEmpty());load.setEnabled(!info.loading);
        file.setText(info.fileName.isEmpty()?"No SoundFont loaded":info.fileName,juce::dontSendNotification);
        status.setText(info.status.isEmpty()?"Choose a bank, drop one .sf2 here, or click LOAD SF2.":info.status,juce::dontSendNotification);status.setTooltip(status.getText());
        preview.setEnabled(info.enabled&&!info.loading&&!shown.isEmpty());enable.setEnabled(!info.loading&&!shown.isEmpty()&&!info.enabled);synth.setEnabled(info.enabled||info.loading);
    }
};
class AdvancedPanel final : public juce::Component {
public:
    AdvancedPanel(GbaProcessor& p,std::function<void()> close,std::function<void()> skinChanged):processor(p),tabs(juce::TabbedButtonBar::TabsAtTop) {
        using P=gba::Param;
        skinLabel.setText("SKIN",juce::dontSendNotification);skinLabel.setFont(font(13));
        skin.setName("UI skin");skin.setTooltip("Console colour for this instance. Turn off NEW COLOUR ON LOAD to keep it when reopening your project.");
        for(std::size_t i=0;i<gba_ui::skins.size();++i)skin.addItem(gba_ui::skins[i].name.data(),int(i)+1);
        syncSkin();skin.onChange=[this,skinChanged]{processor.setEditorSkin(skin.getSelectedId()-1);skinChanged();};
        automaticSkin.setName("New colour on load");automaticSkin.setTooltip("Choose a different colour on the next plugin load. Reopening this window or changing presets keeps the current colour. Turn off to save a fixed project colour.");
        automaticSkin.onClick=[this]{processor.setNewSkinOnLaunch(automaticSkin.getToggleState());};
        syncSkin();addAndMakeVisible(skinLabel);addAndMakeVisible(skin);addAndMakeVisible(automaticSkin);
        tabs.setName("Advanced tabs");addAndMakeVisible(tabs);addAndMakeVisible(back);back.onClick=[this,close]{asyncClose(this,close);};
        tabs.addTab("OSCILLATORS",uiColour(*this,0xff131c61),new ParameterPage(p,{P::Engine,P::Wave1,P::Wave2,P::OscMix,P::Detune,P::Coarse,P::PulseWidth,P::Brightness,P::FmRatio,P::FmIndex,P::Harmonics,P::Wavetable,P::StringDamping,P::Vowel,P::FormantShift,P::Breath,P::Choir,P::Syllable}),true);
        tabs.addTab("FILTER & ENVELOPES",uiColour(*this,0xff131c61),new ParameterPage(p,{P::Cutoff,P::Resonance,P::FilterType,P::FilterEnv,P::FilterAttack,P::FilterDecay,P::FilterSustain,P::FilterRelease,P::Attack,P::Decay,P::Sustain,P::Release}),true);
        tabs.addTab("MODULATION & VOICES",uiColour(*this,0xff131c61),new ParameterPage(p,{P::Lfo1Rate,P::Lfo1Depth,P::Lfo1Dest,P::Lfo2Rate,P::Lfo2Depth,P::Lfo2Dest,P::Vibrato,P::Tremolo,P::Glide,P::Voices,P::Mode,P::BendRange,P::Velocity}),true);
        tabs.addTab("EFFECTS & MACROS",uiColour(*this,0xff131c61),new ParameterPage(p,{P::Reverb,P::ReverbSize,P::Delay,P::DelayTime,P::DelayFeedback,P::Chorus,P::Phaser,P::Flanger,P::Drive,P::Distortion,P::CrusherBits,P::CrusherRate,P::Wow,P::EqLow,P::EqHigh,P::Width,P::MacroA,P::MacroB,P::MacroC,P::MacroD}),true);
        setWantsKeyboardFocus(true);
    }
    void paint(juce::Graphics& g) override {panel(*this,g,getLocalBounds().toFloat(),14);caption(g,"ADVANCED SYNTHESIS",{20,12,float(getWidth()-210),28},22);}
    void resized() override {skinLabel.setBounds(20,49,50,28);skin.setBounds(72,49,juce::jmin(280,getWidth()/3),28);automaticSkin.setBounds(82+skin.getWidth(),49,juce::jmin(300,getWidth()-skin.getWidth()-102),28);back.setBounds(getWidth()-165,12,145,30);tabs.setBounds(16,88,getWidth()-32,getHeight()-102);}
    void syncSkin() {skin.setSelectedId(gba_ui::validSkin(processor.editorSkin.load())+1,juce::dontSendNotification);automaticSkin.setToggleState(processor.newSkinOnLaunch.load(),juce::dontSendNotification);}
    void lookAndFeelChanged() override {
        for(int i=0;i<tabs.getNumTabs();++i)tabs.setTabBackgroundColour(i,uiColour(*this,0xff131c61));
    }
private:GbaProcessor& processor;juce::TabbedComponent tabs;juce::ComboBox skin;juce::Label skinLabel;juce::ToggleButton automaticSkin {"NEW COLOUR ON LOAD"};juce::TextButton back {"BACK / SELECT"};
};
class BrowserPanel final : public juce::Component,private juce::ListBoxModel,private juce::KeyListener,private juce::Timer {
public:
    BrowserPanel(GbaProcessor& p,std::function<void()> close):processor(p),closePanel(std::move(close)),list("Presets",this){
        for(auto* c:std::initializer_list<juce::Component*>{&search,&category,&subcategory,&source,&favoritesOnly,&recentOnly,&list,&back,&preview,&stop,&favorite,&save,&rename,&remove,&restore,&name,&tags,&rescan,&importSound,&exportSound,&status}){addAndMakeVisible(c);c->addKeyListener(this);}
        setName("Preset browser");setWantsKeyboardFocus(true);search.setName("Preset search");category.setName("Preset category");source.setName("Preset source");name.setName("Preset name");tags.setName("Preset tags");status.setName("Preset status");
        search.setTextToShowWhenEmpty("Search words or \"quoted phrases\", names and tags...",text.withAlpha(.55f));search.onTextChange=[this]{message.clear();refresh();};
        subcategory.setName("Preset subfolder");subcategory.setTooltip("Choose a folder inside the selected preset category, or browse all its sounds.");
        category.onChange=[this]{subcategory.setSelectedId(1,juce::dontSendNotification);message.clear();refresh();};subcategory.onChange=[this]{message.clear();refresh();};source.addItemList({"All sources","Factory","User"},1);source.setSelectedId(1,juce::dontSendNotification);source.onChange=[this]{message.clear();refresh();};
        favoritesOnly.onClick=[this]{message.clear();refresh();};recentOnly.onClick=[this]{message.clear();refresh();};
        back.onClick=[this]{asyncClose(this,closePanel);};preview.onClick=[this]{activate(true);};stop.onClick=[this]{processor.stopPreview();};
        favorite.onClick=[this]{auto id=selectedId();if(id.isNotEmpty())result(processor.presets.toggleFavorite(id),"Favorite updated.");};
        save.onClick=[this]{result(processor.presets.saveAs(name.getText(),tags.getText()),"User preset saved.",true);};
        rename.onClick=[this]{auto id=selectedId();if(id.isNotEmpty())result(processor.presets.renameUser(id,name.getText()),"User preset renamed.");};
        remove.onClick=[this]{auto id=selectedId();auto* selected=processor.presets.find(id);if(!selected||selected->factory)return;juce::Component::SafePointer<BrowserPanel> safe(this);
            (void)juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::QuestionIcon,"Delete user preset?","Delete "+selected->name+" from disk?","Delete","Cancel",this,juce::ModalCallbackFunction::create([safe,id](int answer){if(safe!=nullptr&&answer==1)safe->result(safe->processor.presets.deleteUser(id),"User preset deleted.",true);}));};
        restore.onClick=[this]{processor.presets.restoreDefault();message="Dream Pad restored.";refresh(true);};
        importSound.onClick=[this]{choosePreset(false);};exportSound.onClick=[this]{choosePreset(true);};
        importSound.setTooltip("Add a .gba2preset synthesis patch to your user library. Imports are copies; existing patches are kept.");
        exportSound.setTooltip("Export the currently loaded sound, including unsaved synthesis edits, to share or back up.");
        rescan.onClick=[this]{result(processor.presets.refresh(),"Library refreshed.");};
        name.setTextToShowWhenEmpty("Name for Save As / Rename",text.withAlpha(.55f));name.setInputRestrictions(100);tags.setInputRestrictions(512);tags.setTextToShowWhenEmpty("Tags for Save As",text.withAlpha(.55f));
        search.setTooltip("All words must match. Use quotes for a phrase. Ctrl+F focuses search; Down browses results.");tags.setTooltip("Space-separated tags saved with Save As.");list.setTooltip("Arrows browse; click loads; Enter or Space loads and previews; Delete asks before deleting a user preset.");rescan.setTooltip("Reload user files, favorites and recent history from disk. F5.");
        list.setRowHeight(38);list.setMultipleSelectionEnabled(false);status.setFont(font(12));refresh(true);startTimerHz(10);
    }
    ~BrowserPanel() override {stopTimer();list.setModel(nullptr);}
    void paint(juce::Graphics& g) override {panel(*this,g,getLocalBounds().toFloat(),14);caption(g,"PRESET LIBRARY",{20,10,float(std::max(100,getWidth()-620)),30},22);}
    void resized() override {
        int w=getWidth(),h=getHeight();back.setBounds(w-160,12,140,30);source.setBounds(w-300,12,130,30);category.setBounds(w-550,12,240,30);
        search.setBounds(20,56,w-(subcategory.isVisible()?660:470),30);subcategory.setBounds(w-630,56,180,30);favoritesOnly.setBounds(w-440,56,135,30);recentOnly.setBounds(w-300,56,135,30);rescan.setBounds(w-155,56,135,30);list.setBounds(20,99,w-40,h-221);
        preview.setBounds(20,h-110,110,29);stop.setBounds(138,h-110,90,29);favorite.setBounds(236,h-110,145,29);importSound.setBounds(389,h-110,125,29);exportSound.setBounds(522,h-110,135,29);restore.setBounds(w-215,h-110,195,29);
        const int input=w-490,n=int(input*.55);name.setBounds(20,h-69,n,30);tags.setBounds(30+n,h-69,input-n,30);save.setBounds(w-450,h-69,130,30);rename.setBounds(w-310,h-69,135,30);remove.setBounds(w-165,h-69,145,30);status.setBounds(20,h-35,w-40,25);
    }
private:
    GbaProcessor& processor;std::function<void()> closePanel;
    juce::TextEditor search,name,tags;juce::ComboBox category,subcategory,source;
    juce::ToggleButton favoritesOnly {"Favorites only"},recentOnly {"Recently used"};juce::ListBox list;
    juce::TextButton back {"BACK / START"},preview {"A  PREVIEW"},stop {"B  STOP"},favorite {"FAVORITE"},save {"SAVE AS"},rename {"RENAME"},remove {"DELETE"},restore {"RESTORE DREAM PAD"},rescan {"REFRESH"},importSound {"IMPORT"},exportSound {"EXPORT SOUND"};
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Label status;std::vector<juce::String> visible;juce::String activeSeen,shownId,message;std::uint64_t revisionSeen=0;bool updating=false;
    void choosePreset(bool exporting) {
        const auto initial=exporting?juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(juce::File::createLegalFileName(processor.presetName())+".gba2preset"):juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        chooser=std::make_unique<juce::FileChooser>(exporting?"Export current synthesis sound":"Import synthesis preset",initial,"*.gba2preset",true);
        const int flags=exporting?(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting)
                                 :(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles);
        juce::Component::SafePointer<BrowserPanel> safe(this);
        chooser->launchAsync(flags,[safe,exporting](const juce::FileChooser& dialog){
            if(safe==nullptr)return;
            auto file=dialog.getResult();if(file==juce::File())return;
            if(exporting){safe->result(safe->processor.presets.exportCurrent(file,safe->name.getText(),safe->tags.getText()),"Sound exported.");}
            else safe->result(safe->processor.presets.importPreset(file),"Synthesis preset imported.",true);
        });
    }
    juce::String selectedId() const {int row=list.getSelectedRow();return row>=0&&row<int(visible.size())?visible[std::size_t(row)]:juce::String();}
    void result(juce::Result r,const juce::String& success,bool selectActive=false){message=r.failed()?r.getErrorMessage():(processor.presets.lastError.isEmpty()?success:processor.presets.lastError);refresh(selectActive);if(r.wasOk()&&(selectActive||success=="Library refreshed."))updateButtons(true);}
    void refresh(bool selectActive=false){
        updating=true;auto keep=selectActive?processor.presetId():selectedId();auto selectedCategory=category.getSelectedId()>1?category.getText():juce::String();category.clear(juce::dontSendNotification);category.addItem("All categories",1);category.addItemList(processor.presets.categories(),2);int index=processor.presets.categories().indexOf(selectedCategory);category.setSelectedId(index<0?1:index+2,juce::dontSendNotification);
        auto folders=processor.presets.subcategories(category.getSelectedId()>1?category.getText():juce::String());auto selectedFolder=subcategory.getSelectedId()>1?subcategory.getText():juce::String();subcategory.clear(juce::dontSendNotification);subcategory.addItem("All subfolders",1);subcategory.addItemList(folders,2);int folderIndex=folders.indexOf(selectedFolder);subcategory.setSelectedId(folderIndex<0?1:folderIndex+2,juce::dontSendNotification);subcategory.setVisible(!folders.isEmpty());if(getWidth()>0)resized();
        PresetManager::Filter filter;filter.query=search.getText();filter.subcategory=subcategory.getSelectedId()>1?subcategory.getText():juce::String();filter.category=category.getSelectedId()>1?category.getText():juce::String();filter.source=source.getSelectedId()==2?PresetManager::Source::factory:source.getSelectedId()==3?PresetManager::Source::user:PresetManager::Source::all;filter.favorites=favoritesOnly.getToggleState();filter.recent=recentOnly.getToggleState();visible=processor.presets.filtered(filter);
        list.updateContent();list.deselectAllRows();int row=-1;for(std::size_t i=0;i<visible.size();++i)if(visible[i]==keep)row=int(i);if(row<0)for(std::size_t i=0;i<visible.size();++i)if(visible[i]==processor.presetId())row=int(i);if(row>=0)list.selectRow(row);
        activeSeen=processor.presetId();revisionSeen=processor.presets.revision();updating=false;updateButtons();list.repaint();
    }
    void updateButtons(bool forceFields=false){
        auto id=selectedId();auto* p=processor.presets.find(id);const bool loaded=p&&id==processor.presetId();rename.setEnabled(p&&!p->factory);remove.setEnabled(p&&!p->factory);preview.setEnabled(p!=nullptr);favorite.setEnabled(p!=nullptr);save.setEnabled(!p||loaded);exportSound.setEnabled(!p||loaded);favorite.setButtonText(p&&processor.presets.isFavorite(id)?"UNFAVORITE":"FAVORITE");
        auto fieldId=p?id:processor.presetId();if(forceFields||fieldId!=shownId){shownId=fieldId;if(forceFields||!name.hasKeyboardFocus(false))name.setText(p?p->name:processor.presetName(),false);if(forceFields||!tags.hasKeyboardFocus(false))tags.setText(p?p->tags:juce::String(),false);}
        juce::String detail=message.isNotEmpty()?message:processor.presets.lastError;if(detail.isEmpty())detail=processor.stateNotice();if(detail.isEmpty()&&p&&!loaded)detail="Click or Enter to load the selected preset.";if(detail.isEmpty()&&!p)detail="No selected result. Save As stores the currently loaded sound.";
        auto value=juce::String(int(visible.size()))+(visible.size()==1?" result":" results")+(detail.isEmpty()?juce::String():" / "+detail);status.setText(value,juce::dontSendNotification);status.setTooltip(value);
    }
    void activate(bool audition){auto id=selectedId();if(id.isEmpty())return;if(id!=processor.presetId()&&!processor.presets.load(id)){message=processor.presets.lastError;updateButtons();return;}if(audition)processor.preview();message=processor.presets.lastError;refresh();}
    void timerCallback() override {if(activeSeen!=processor.presetId())refresh(true);else if(revisionSeen!=processor.presets.revision())refresh();}
    bool keyPressed(const juce::KeyPress& key,juce::Component* origin) override {
        if(key==juce::KeyPress::escapeKey){asyncClose(this,closePanel);return true;}
        if(key.getModifiers().isCtrlDown()&&(key.getKeyCode()=='F'||key.getKeyCode()=='f')){search.grabKeyboardFocus();search.selectAll();return true;}
        if(key==juce::KeyPress::F5Key){rescan.triggerClick();return true;}
        if(origin==&search&&(key==juce::KeyPress::downKey||key==juce::KeyPress::upKey)){if(!visible.empty()){list.selectRow(juce::jlimit(0,int(visible.size())-1,list.getSelectedRow()+(key==juce::KeyPress::downKey?1:-1)));list.grabKeyboardFocus();}return true;}
        if(origin==&list&&key==juce::KeyPress::spaceKey){activate(true);return true;}return false;
    }
    int getNumRows() override {return int(visible.size());}
    void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected) override {
        if(row<0||row>=int(visible.size()))return;
        auto* p=processor.presets.find(visible[std::size_t(row)]);if(!p)return;
        g.fillAll(selected?uiColour(*this,0xff7943ff).darker(.4f):(row%2?uiColour(*this,0xff131c61).withAlpha(.4f):uiColour(*this,0xff080c25)));g.setColour(text);g.setFont(font(15,true));g.drawText((processor.presets.isFavorite(p->id)?"* ":"  ")+p->name,10,1,width-330,height-2,juce::Justification::centredLeft,true);g.setColour(text.withAlpha(.6f));g.setFont(font(12));g.drawText((p->factory?p->category+(p->subcategory.isEmpty()?juce::String():" / "+p->subcategory)+" / FACTORY":juce::String("USER"))+(p->id==processor.presetId()?" / LOADED":""),width-320,0,310,height,juce::Justification::centredRight,true);
    }
    void selectedRowsChanged(int) override {if(!updating){message.clear();updateButtons();}}
    void listBoxItemClicked(int,const juce::MouseEvent&) override {activate(false);}
    void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override {activate(true);}
    void returnKeyPressed(int) override {activate(true);}
    void deleteKeyPressed(int) override {if(remove.isEnabled())remove.triggerClick();}
};
}

void RomCartridgeButton::paintButton(juce::Graphics& g,bool over,bool down){
    juce::Graphics::ScopedSaveState saved(g);g.addTransform(juce::AffineTransform::scale(float(getWidth())/220,float(getHeight())/72));
    const float dy=down?2.0f:0.0f;g.addTransform(juce::AffineTransform::translation(0,dy));
    juce::Path cart;cart.startNewSubPath(4,15);cart.lineTo(24,15);cart.lineTo(24,4);cart.lineTo(196,4);cart.lineTo(196,15);cart.lineTo(216,15);cart.lineTo(216,67);cart.lineTo(4,67);cart.closeSubPath();
    g.setColour(juce::Colours::black.withAlpha(.7f));g.fillPath(cart,juce::AffineTransform::translation(0,3));
    g.setGradientFill(juce::ColourGradient(juce::Colour(over?0xfff4f2fa:0xffd2d3dc),0,3,juce::Colour(0xff555b70),0,72,false));g.fillPath(cart);
    g.setColour(uiColour(*this,0xff080c25));g.strokePath(cart,juce::PathStrokeType(2));g.setColour(juce::Colours::white.withAlpha(.5f));g.drawLine(27,7,193,7,1.5f);
    panel(*this,g,{34,15,152,41},5);caption(g,"ROM Hacks",{39,21,142,25},19);
    for(int side:{12,202})for(int i=0;i<4;++i){g.setColour(juce::Colour(0xff404858));g.drawLine(float(side),25+float(i)*7,float(side+6),25+float(i)*7,2);}
    for(int i=0;i<12;++i){g.setColour(juce::Colour(0xffd2b16e));g.fillRect(46+i*11,62,6,4);}
    if(hasKeyboardFocus(false)){g.setColour(uiColour(*this,0xffd2b7ff));g.strokePath(cart,juce::PathStrokeType(2.5f));}
}

ConsoleLook::ConsoleLook() {setSkin(0);}
juce::Colour ConsoleLook::colour(juce::Colour source) const {
    if(currentSkin==0)return source;
    // Recolour the existing vector palette, keeping its original shading and contrast.
    const auto& skin=gba_ui::skins[std::size_t(currentSkin)];
    return juce::Colour::fromHSV(skin.hue,source.getSaturation()*skin.saturation,
                                  source.getBrightness()*skin.brightness,source.getFloatAlpha());
}
juce::Colour ConsoleLook::shell(juce::Colour source,bool body) const {
    if(currentSkin==0)return source;
    if(currentSkin==2||currentSkin==3) {
        const auto brightness=source.getBrightness();
        const float value=currentSkin==2?(body?.065f+.14f*brightness:.10f+.24f*brightness)
                                       :(body?.48f+.30f*brightness:.68f+.31f*brightness);
        return juce::Colour::fromHSV(.58f,currentSkin==2?.06f:.025f,value,source.getFloatAlpha());
    }
    return colour(source);
}
void ConsoleLook::setSkin(int index) {
    currentSkin=gba_ui::validSkin(index);
    setColour(juce::Label::textColourId,text);setColour(juce::TextButton::textColourOffId,text);setColour(juce::TextButton::textColourOnId,text);
    setColour(juce::Slider::textBoxTextColourId,text);setColour(juce::Slider::textBoxBackgroundColourId,colour(juce::Colour(0xff080c25)));setColour(juce::Slider::textBoxOutlineColourId,colour(juce::Colour(0xff7943ff)).withAlpha(.3f));
    setColour(juce::Slider::thumbColourId,colour(juce::Colour(0xff7943ff)).brighter());setColour(juce::Slider::trackColourId,colour(juce::Colour(0xff7943ff)));setColour(juce::Slider::backgroundColourId,colour(juce::Colour(0xff080c25)));
    setColour(juce::TextEditor::backgroundColourId,colour(juce::Colour(0xff080c25)));setColour(juce::TextEditor::textColourId,text);setColour(juce::TextEditor::outlineColourId,colour(juce::Colour(0xff7943ff)).withAlpha(.6f));
    setColour(juce::ComboBox::backgroundColourId,colour(juce::Colour(0xff080c25)));setColour(juce::ComboBox::textColourId,text);setColour(juce::ComboBox::outlineColourId,colour(juce::Colour(0xff7943ff)));
    setColour(juce::ListBox::backgroundColourId,colour(juce::Colour(0xff080c25)));
    setColour(juce::TabbedButtonBar::tabTextColourId,text);setColour(juce::TabbedButtonBar::frontTextColourId,text);
    setColour(juce::TextEditor::highlightColourId,colour(juce::Colour(0xff7943ff)).darker(.4f));
    setColour(juce::TextEditor::highlightedTextColourId,text);
    setColour(juce::PopupMenu::backgroundColourId,colour(juce::Colour(0xff080c25)));setColour(juce::PopupMenu::textColourId,text);setColour(juce::ToggleButton::textColourId,text);setColour(juce::ToggleButton::tickColourId,colour(juce::Colour(0xff7943ff)).brighter());
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
    g.setColour(colour(juce::Colour(0xff080c25)));
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
        g.setColour(colour(juce::Colour(0xff3f1488)));g.drawEllipse(ringBounds,radius*.065f);
        g.setColour(colour(juce::Colour(0xff7943ff)).withAlpha(.55f));g.drawEllipse(ringBounds,radius*.035f);
        for(int i=0;i<84;++i) {
            const float at=juce::MathConstants<float>::twoPi*float(i)/84,ledRadius=radius*.92f;
            const float size=std::max(1.6f,radius*.025f);
            g.setColour(colour(juce::Colour(0xffeadbff)).withAlpha(.78f));
            g.fillEllipse(cx+std::sin(at)*ledRadius-size*.5f,cy-std::cos(at)*ledRadius-size*.5f,size,size);
        }
    } else {
        for(int i=0;i<29;++i) {
            const float ledAngle=start+(end-start)*float(i)/28,ledRadius=radius*1.09f;
            g.setColour(i<=int(position*28)?colour(juce::Colour(0xffc09aff)):colour(juce::Colour(0xff7943ff)).withAlpha(.32f));
            g.fillEllipse(cx+std::sin(ledAngle)*ledRadius-1.5f,
                          cy-std::cos(ledAngle)*ledRadius-1.5f,3,3);
        }
    }
    juce::Path arc;
    const float ring=master?.92f:.89f;
    arc.addCentredArc(cx,cy,radius*ring,radius*ring,0,start,angle,true);
    g.setColour(colour(juce::Colour(0xff7943ff)).brighter(.55f));
    g.strokePath(arc,juce::PathStrokeType(std::max(1.5f,radius*.025f)));
    g.setColour(colour(juce::Colour(0xff080c25)));
    g.drawLine(cx+std::sin(angle)*radius*.57f,cy-std::cos(angle)*radius*.57f,
               cx+std::sin(angle)*radius*.75f,cy-std::cos(angle)*radius*.75f,
               std::max(2.5f,radius*.045f));
    if(slider.hasKeyboardFocus(false)) {
        g.setColour(colour(juce::Colour(0xffe0caff)));g.drawEllipse(bounds.expanded(2),1.5f);
    }
}
void ConsoleLook::drawButtonBackground(juce::Graphics& g,juce::Button& button,
                                       const juce::Colour&,bool over,bool down) {
    auto bounds=button.getLocalBounds().toFloat().reduced(3);
    const bool round=button.getProperties()["round"];
    if(button.getButtonText()=="START"||button.getButtonText()=="SELECT") {
        g.setColour(colour(juce::Colour(0xff080c25)));g.fillRoundedRectangle(bounds,bounds.getHeight()*.5f);
        g.setColour(colour(juce::Colour(0xff7943ff)).withAlpha(.35f));g.drawRoundedRectangle(bounds,bounds.getHeight()*.5f,1);
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
        g.setColour(colour(juce::Colour(0xff080c25)).withAlpha(.55f));g.drawEllipse(bounds.reduced(.5f),1.5f);
        if(button.hasKeyboardFocus(false)){g.setColour(text);g.drawEllipse(bounds.expanded(1),1.5f);}
        return;
    }
    g.fillRoundedRectangle(bounds.translated(0,2),7);
    auto light=juce::Colour(0xff29364d),dark=colour(juce::Colour(0xff080c25));
    if(button.getName()=="Preset display")light=juce::Colour(0xff18233c);
    if(down)std::swap(light,dark);
    if(over)light=light.brighter(.1f);
    g.setGradientFill(juce::ColourGradient(light,bounds.getX(),bounds.getY(),dark,
                                          bounds.getRight(),bounds.getBottom(),false));
    g.fillRoundedRectangle(bounds,7);
    g.setColour(colour(juce::Colour(0xff7943ff)).withAlpha(.65f));
    g.drawRoundedRectangle(bounds,7,1);
    g.setColour(juce::Colours::white.withAlpha(.12f));g.drawLine(bounds.getX()+8,bounds.getY()+2,bounds.getRight()-8,bounds.getY()+2,1);
    if(button.hasKeyboardFocus(false)){g.setColour(text);g.drawRoundedRectangle(bounds.reduced(1),6,1);}
}
void ConsoleLook::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool,bool) {
    bool round=button.getProperties()["round"];g.setColour((round?colour(juce::Colour(0xff080c25)):text).withAlpha(button.isEnabled()?1.0f:.35f));
    if(button.getName()=="Preset display") {gba_ui::pixelText(g,button.getButtonText(),button.getLocalBounds().toFloat().reduced(12,10),float(button.getHeight())*.043f);return;}
    if(button.getButtonText()=="START"||button.getButtonText()=="SELECT") {
        auto label=button.getLocalBounds().toFloat();label.removeFromLeft(label.getWidth()*.45f);
        gba_ui::pixelText(g,button.getButtonText(),label,float(button.getHeight())*.05f,false);return;
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
    g.setColour(colour(juce::Colour(0xff080c25)));g.fillRoundedRectangle(b.expanded(7),6);
    juce::ColourGradient roller(colour(juce::Colour(0xffa070ff)),b.getX(),b.getY(),juce::Colour(0xff050916),b.getRight(),b.getY(),false);
    roller.addColour(.12,colour(juce::Colour(0xff7943ff)));roller.addColour(.2,juce::Colour(0xff14152c));roller.addColour(.55,juce::Colour(0xff343143));roller.addColour(.86,juce::Colour(0xff10111e));roller.addColour(.95,colour(juce::Colour(0xff7943ff)).darker(.35f));
    g.setGradientFill(roller);g.fillRoundedRectangle(b,5);
    const float phase=std::fmod(position*.17f,5.0f);
    for(float at=b.getY()+4+phase;at<b.getBottom()-2;at+=5) {
        g.setColour(juce::Colours::black.withAlpha(.7f));g.drawLine(b.getX()+8,at,b.getRight()-7,at,2);
        g.setColour(juce::Colour(0xffa2a1ad).withAlpha(.35f));g.drawLine(b.getX()+8,at+1.5f,b.getRight()-7,at+1.5f,1);
    }
    g.setColour(colour(juce::Colour(0xffd4c2ff)));g.fillRoundedRectangle(b.getRight()-5,juce::jlimit(b.getY()+2,b.getBottom()-4,position-1),3,3,1);
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
    g.setColour(uiColour(*this,0xff080c25));g.strokePath(path,juce::PathStrokeType(14,juce::PathStrokeType::curved));
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
        g.setColour(key.isDown()?uiColour(*this,0xff7943ff):(key.isOver()?uiColour(*this,0xff242847):juce::Colour(0xff62656b)));
        g.fillPath(arrow);
        if(key.hasKeyboardFocus(false)) {
            g.setColour(uiColour(*this,0xff7943ff));g.drawRoundedRectangle(areas[i].reduced(4),4,1.5f);
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
    auto light=down?uiColour(*this,0xffb292ec):juce::Colour(0xfff9f8f4);
    auto shade=down?uiColour(*this,0xff8162bc):juce::Colour(0xffd6d7d9);
    if(over&&!down)light=uiColour(*this,0xffe0ceff);
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
    auto light=down?uiColour(*this,0xff6740a0):(over?uiColour(*this,0xff393056):juce::Colour(0xff222836));
    g.setGradientFill(juce::ColourGradient(light,face.getX(),face.getY(),juce::Colour(0xff090c19),face.getRight(),face.getBottom(),false));
    g.fillRoundedRectangle(face,1.5f);
    g.setColour(juce::Colour(0xff667087).withAlpha(.5f));g.drawLine(face.getX(),face.getY()+1,face.getX(),face.getBottom(),1);
    g.drawLine(face.getX(),face.getBottom(),face.getRight(),face.getBottom(),1);
}

GbaEditor::GbaEditor(GbaProcessor& p):AudioProcessorEditor(p),processor(p),keyboard(p.keyboard) {
    look.setSkin(processor.editorSkin.load());setLookAndFeel(&look);setOpaque(true);setWantsKeyboardFocus(true);
    for(auto* c:std::initializer_list<juce::Component*>{&keyboard,&previous,&next,&presetDisplay,&start,&select,&aButton,&bButton,&dpad,&romCartridge,&volume,&glide,&pitch})addAndMakeVisible(c);
    romCartridge.setName("ROM Hacks cartridge");romCartridge.setTooltip("ROM Hacks: drop one .sf2 file anywhere on this window, or click to load (F3)");romCartridge.onClick=[this]{showSoundFonts();};
    rotary(volume,"Master volume");rotary(glide,"Glide");
    volume.setTooltip("Master output volume");glide.setTooltip("Glide / portamento time. Voice mode is under SELECT.");
    for(std::size_t i=0;i<macros.size();++i){rotary(macros[i],"Macro "+juce::String::charToString(char('A'+i)));addAndMakeVisible(macros[i]);}
    volumeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"volume",volume);
    glideAttachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"voice.glide",glide);
    volume.setDoubleClickReturnValue(true,.72);glide.setDoubleClickReturnValue(true,0);
    pitch.setName("Pitch wheel");pitch.setSliderStyle(juce::Slider::LinearVertical);pitch.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);pitch.setRange(0,16383,1);pitch.setValue(8192,juce::dontSendNotification);pitch.setTooltip("Pitch bend. Release to return to center.");
    pitch.onValueChange=[this]{processor.uiPitch(int(pitch.getValue()));};pitch.onDragEnd=[this]{pitch.setValue(8192,juce::sendNotificationSync);};pitch.setDoubleClickReturnValue(true,8192);
    previous.setTooltip("Previous preset");next.setTooltip("Next preset");previous.onClick=[this]{if(processor.romHacks.enabled())processor.romHacks.step(-1);else processor.presets.step(-1);};next.onClick=[this]{if(processor.romHacks.enabled())processor.romHacks.step(1);else processor.presets.step(1);};
    previous.setTitle("Previous preset");next.setTitle("Next preset");
    presetDisplay.setName("Preset display");presetDisplay.onClick=[this]{if(processor.romHacks.enabled())showSoundFonts();else showBrowser();};start.setTooltip("Open preset browser (F1)");select.setTooltip("Open advanced synthesis (F2)");start.onClick=[this]{showBrowser();};select.onClick=[this]{showAdvanced();};
    aButton.setTooltip("Preview current patch with a short C4");bButton.setTooltip("Stop preview; DAW MIDI notes continue");aButton.onClick=[this]{processor.preview();};bButton.onClick=[this]{processor.stopPreview();};
    aButton.setTitle("Preview current preset");bButton.setTitle("Stop preset preview");
    dpad.onDirection=[this](int direction){if(processor.romHacks.enabled()){processor.romHacks.step((direction==0||direction==2)?-1:1,direction>=2);return;}if(direction<2)processor.presets.step(direction==0?-1:1);else processor.presets.stepCategory(direction==2?-1:1);};
    keyboard.setName("Piano keyboard");keyboard.setOctaveForMiddleC(4);keyboard.setAvailableRange(24,108);keyboard.setLowestVisibleKey(48);keyboard.setKeyPressBaseOctave(4);keyboard.setScrollButtonsVisible(true);
    keyboard.setDescription("Playable piano. Click or drag keys, or focus here to use the computer keyboard. Side arrows scroll octaves.");
    keyboard.setBlackNoteLengthProportion(.63f);keyboard.setBlackNoteWidthProportion(.63f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffeeeef0));keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,uiColour(*this,0xff080c25));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,uiColour(*this,0xff7943ff).withAlpha(.7f));keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,uiColour(*this,0xff7943ff).withAlpha(.3f));keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId,juce::Colour(0xff4b4b63));
    setResizable(true,true);setResizeLimits(960,485,1920,970);getConstrainer()->setFixedAspectRatio(1280.0/646.0);
    int w=processor.editorWidth.load();setSize(w,int(std::round(double(w)*646/1280)));keyboard.setLowestVisibleKey(48);updateMacros();timerCallback();startTimerHz(25);
}
GbaEditor::~GbaEditor() {stopTimer();processor.uiPitch(8192);processor.keyboard.allNotesOff(0);overlay.reset();setLookAndFeel(nullptr);}
void GbaEditor::configureRenderer() {
#if JUCE_WINDOWS && GBA_WINDOWS_SOFTWARE_EDITOR
    // The VST3 wrapper owns the native peer; it does not exist in our constructor.
    // Reapply after attachment/reparenting rather than retaining a stale peer pointer.
    if(auto* peer=getPeer()) {
        const int software=peer->getAvailableRenderingEngines().indexOf("Software Renderer");
        if(software>=0&&peer->getCurrentRenderingEngine()!=software)
            peer->setCurrentRenderingEngine(software);
    }
#endif
}
void GbaEditor::parentHierarchyChanged() {
    juce::AudioProcessorEditor::parentHierarchyChanged();
    configureRenderer();
}
void GbaEditor::visibilityChanged() {
    juce::AudioProcessorEditor::visibilityChanged();
    configureRenderer();
}
void GbaEditor::paint(juce::Graphics& g) {
    if(getLocalBounds().isEmpty())return;
    // Cache only the static vector housing. Live controls and LEDs remain separate.
    // Rebuild at the actual rendering density so DAW DPI scaling stays sharp.
    const float density=juce::jlimit(1.0f,4.0f,g.getInternalContext().getPhysicalPixelScaleFactor());
    const int w=int(std::ceil(float(getWidth())*density)),h=int(std::ceil(float(getHeight())*density));
    // Very large/high-DPI windows fall back to the original vector path rather
    // than retaining an oversized image in every open plugin instance.
    if(juce::int64(w)*h>16*1024*1024) {
        housingCache={};paintHousing(g);
    } else {
    if(housingCache.getWidth()!=w||housingCache.getHeight()!=h||housingScale!=density||housingSkin!=look.skin()) {
        housingCache=juce::Image(juce::Image::RGB,w,h,false);
        juce::Graphics cached(housingCache);cached.addTransform(juce::AffineTransform::scale(density));paintHousing(cached);
        housingScale=density;housingSkin=look.skin();
    }
    g.drawImage(housingCache,getLocalBounds().toFloat());
    }
    g.addTransform(juce::AffineTransform::scale(float(getWidth())/1280.0f,float(getHeight())/646.0f));
    const auto led=[&](float x,juce::Colour colour,bool active) {
        g.setColour(uiColour(*this,0xff080c25));g.fillEllipse(x-14,43,28,28);g.setColour(colour.withAlpha(active?.35f:.08f));g.fillEllipse(x-12,45,24,24);
        g.setGradientFill(juce::ColourGradient(active?juce::Colours::white:colour.darker(),x-3,49,colour.darker(.2f),x+7,66,false));g.fillEllipse(x-8,49,16,16);
    };
    led(300,juce::Colour(0xff64ff36),signalLed);led(979,juce::Colour(0xffffb733),true);
}
void GbaEditor::paintHousing(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState saved(g);
    if(getLocalBounds().isEmpty())return;
    g.addTransform(juce::AffineTransform::scale(float(getWidth())/1280.0f,float(getHeight())/646.0f));
    g.fillAll(look.shell(juce::Colour(0xff11143d),true));
    auto body=juce::Rectangle<float>(2,2,1276,642);
    juce::ColourGradient bodyColour(look.shell(juce::Colour(0xff3032a3),true),310,210,look.shell(juce::Colour(0xff111552),true),1210,520,true);
    bodyColour.addColour(.5,look.shell(juce::Colour(0xff24267f),true));g.setGradientFill(bodyColour);g.fillRoundedRectangle(body,22);
    // Deterministic moulded-plastic pores, drawn as vectors rather than a reference-image layer.
    juce::Random pores(0x47424132);
    for(int i=0;i<310;++i) {
        float x=12+pores.nextFloat()*1256,y=95+pores.nextFloat()*360;
        float size=.5f+pores.nextFloat()*2;
        g.setColour(uiColour(*this,0xff080c25).withAlpha(.11f));g.fillEllipse(x,y,size,size);
        g.setColour(juce::Colours::white.withAlpha(.035f));g.fillEllipse(x,y+size*.6f,size,size*.5f);
    }
    // Glossy, angular upper housing follows the approved wide rectangular reference.
    juce::Path shell;shell.startNewSubPath(0,0);shell.lineTo(270,0);shell.lineTo(285,25);shell.lineTo(995,25);shell.lineTo(1010,0);shell.lineTo(1280,0);shell.lineTo(1280,95);shell.lineTo(1165,95);shell.lineTo(1116,148);shell.lineTo(1089,161);shell.lineTo(1084,191);shell.lineTo(194,191);shell.lineTo(187,163);shell.lineTo(157,151);shell.lineTo(112,95);shell.lineTo(0,95);shell.closeSubPath();
    g.setColour(juce::Colours::black.withAlpha(.5f));
    g.strokePath(shell,juce::PathStrokeType(12),juce::AffineTransform::translation(0,5));
    juce::ColourGradient plastic(look.shell(juce::Colour(0xffbb96ff)),0,0,look.shell(juce::Colour(0xff6534db)),0,195,false);
    plastic.addColour(.08,look.shell(juce::Colour(0xffa477ff)));plastic.addColour(.14,look.shell(juce::Colour(0xff8450ec)));
    plastic.addColour(.76,look.shell(juce::Colour(0xff7d3df1)));plastic.addColour(.92,look.shell(juce::Colour(0xff9861ff)));
    g.setGradientFill(plastic);g.fillPath(shell);
    g.setColour(look.shell(juce::Colour(0xff3b147d)));g.strokePath(shell,juce::PathStrokeType(9));
    g.setColour(look.shell(juce::Colour(0xffa97aff)));g.strokePath(shell,juce::PathStrokeType(5));
    g.setColour(look.shell(juce::Colour(0xffe1c6ff)).withAlpha(.8f));g.strokePath(shell,juce::PathStrokeType(1.5f),juce::AffineTransform::translation(0,-2));
    g.setColour(juce::Colours::white.withAlpha(.25f));g.drawLine(12,12,251,12,4);g.drawLine(1028,12,1268,12,4);
    panel(*this,g,{255,-12,770,117},26);panel(*this,g,{342,17,596,77},16);
    // Original typography and branding, without manufacturer marks from real consoles.
    auto logo=gba_ui::blockLettering("G BOY",{397,125,200,43});
    juce::GlyphArrangement smallTitle;
    smallTitle.addFittedText(juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(),43,juce::Font::bold)),
                            "ADVANCE",603,126,207,44,juce::Justification::centred,1);
    juce::Path advance;smallTitle.createPath(advance);
    logo.addPath(advance);
    g.setColour(uiColour(*this,0xff080c25));g.strokePath(logo,juce::PathStrokeType(9,juce::PathStrokeType::curved));
    g.setColour(juce::Colours::black.withAlpha(.7f));g.fillPath(logo,juce::AffineTransform::translation(1,5));
    g.setGradientFill(juce::ColourGradient(juce::Colours::white,0,125,silver.darker(.1f),0,170,false));g.fillPath(logo);
    auto numberPath=gba_ui::blockLettering("2",{825,122,54,55});
    g.setColour(juce::Colours::black.withAlpha(.65f));g.strokePath(numberPath,juce::PathStrokeType(10),juce::AffineTransform::translation(2,4));
    g.setColour(uiColour(*this,0xff080c25));g.strokePath(numberPath,juce::PathStrokeType(8));
    g.setGradientFill(juce::ColourGradient(uiColour(*this,0xffd2b7ff),0,104,uiColour(*this,0xff7943ff),0,181,false));g.fillPath(numberPath);
    panel(*this,g,{607,177,66,19},8);caption(g,"VST",{609,177,62,18},12);
    // D-pad recess and speaker / fastener details are vector decoration.
    g.setColour(uiColour(*this,0xff080c25).withAlpha(.3f));g.fillEllipse(35,172,191,181);g.setColour(uiColour(*this,0xff7943ff).withAlpha(.2f));g.drawEllipse(35,172,191,181,2);
    panel(*this,g,{278,307,725,100},46);
    g.setGradientFill(juce::ColourGradient(uiColour(*this,0xff35439e),0,405,uiColour(*this,0xff111856),0,440,false));g.fillRoundedRectangle(307,390,664,51,13);
    g.setColour(uiColour(*this,0xff7943ff).withAlpha(.32f));g.drawRoundedRectangle({307,390,664,51},13,1);
    panel(*this,g,{548,224,184,30},7);caption(g,"VOLUME",{552,224,176,29},18);
    for(int i=0;i<4;++i)caption(g,juce::String::charToString(char('A'+i)),{float(i<2?340+i*107:778+(i-2)*107),407,54,30},19);
    for(int i=0;i<4;++i) {
        float y=381+float(i)*16;
        juce::Path slit;slit.startNewSubPath(1078,y);slit.lineTo(1220,y-24);
        g.setColour(juce::Colour(0xff010317));g.strokePath(slit,juce::PathStrokeType(8,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        g.setColour(uiColour(*this,0xff6c52ca).withAlpha(.65f));g.strokePath(slit,juce::PathStrokeType(2),juce::AffineTransform::translation(0,5));
    }
    for(auto point:std::initializer_list<juce::Point<float>>{{26,57},{1254,57},{30,446},{1250,446}}) {
        g.setColour(uiColour(*this,0xff080c25).withAlpha(.6f));g.fillEllipse(point.x-7,point.y-7,14,14);g.setColour(uiColour(*this,0xff7943ff).withAlpha(.4f));g.drawEllipse(point.x-7,point.y-7,14,14,1);
    }
    // Lower chassis and the three inset control wells.
    juce::Path lower;lower.startNewSubPath(0,461);lower.lineTo(30,455);lower.lineTo(1250,455);lower.lineTo(1280,467);lower.lineTo(1280,646);lower.lineTo(0,646);lower.closeSubPath();
    juce::ColourGradient lowerPlastic(look.shell(juce::Colour(0xffb187ff)),0,455,look.shell(juce::Colour(0xff5630bd)),0,646,false);
    lowerPlastic.addColour(.07,look.shell(juce::Colour(0xff824ff3)));lowerPlastic.addColour(.15,look.shell(juce::Colour(0xff6238cf)));lowerPlastic.addColour(.85,look.shell(juce::Colour(0xff5330b3)));lowerPlastic.addColour(.98,look.shell(juce::Colour(0xff9467ef)));
    g.setGradientFill(lowerPlastic);g.fillPath(lower);
    g.setColour(look.shell(juce::Colour(0xff3e1b8a)));g.strokePath(lower,juce::PathStrokeType(11));
    g.setColour(look.shell(juce::Colour(0xff9865ef)));g.strokePath(lower,juce::PathStrokeType(6));
    g.setColour(look.shell(juce::Colour(0xffd4b3ff)));g.strokePath(lower,juce::PathStrokeType(2),juce::AffineTransform::translation(0,-2));
    panel(*this,g,{46,485,131,140},12);panel(*this,g,{194,489,890,140},7);panel(*this,g,{1102,485,129,140},12);
    caption(g,"PITCH",{70,489,84,25},15);caption(g,"GLIDE",{1117,489,99,25},15);
    g.setColour(uiColour(*this,0xffb789ff).withAlpha(.6f));g.drawRoundedRectangle(body,20,3);
}
void GbaEditor::resized() {
    // Hosts can temporarily collapse an editor while attaching or hiding it.
    // Do not pass a zero key width to JUCE or persist that transient editor size.
    if(getWidth()<=0||getHeight()<=0)return;
    const auto firstKey=keyboard.getLowestVisibleKey();
    float sx=float(getWidth())/1280,sy=float(getHeight())/646;
    auto bounds=[sx,sy](juce::Component& component,float x,float y,float w,float h){component.setBounds(int(std::round(x*sx)),int(std::round(y*sy)),int(std::round(w*sx)),int(std::round(h*sy)));};
    bounds(previous,350,28,68,56);bounds(presetDisplay,418,28,444,56);bounds(next,862,28,68,56);
    bounds(romCartridge,310,224,220,72);
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
void GbaEditor::applySkin() {
    look.setSkin(processor.editorSkin.load());sendLookAndFeelChange();
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,look.colour(juce::Colour(0xff080c25)));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,look.colour(juce::Colour(0xff7943ff)).withAlpha(.7f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,look.colour(juce::Colour(0xff7943ff)).withAlpha(.3f));
    if(auto* advanced=dynamic_cast<AdvancedPanel*>(overlay.get()))advanced->syncSkin();
    repaint();
}
void GbaEditor::timerCallback() {
    if(look.skin()!=gba_ui::validSkin(processor.editorSkin.load()))applySkin();
    if(auto* advanced=dynamic_cast<AdvancedPanel*>(overlay.get()))advanced->syncSkin();
    const auto sf=processor.romHacks.view();
    if(sf.enabled){presetDisplay.setButtonText(sf.instruments.isEmpty()?"SF2: LOAD BANK":sf.instruments[sf.selected].fromFirstOccurrenceOf("  ",false,false).toUpperCase());presetDisplay.setTooltip(sf.fileName+" / Click for ROM Hacks");}
    else {presetDisplay.setButtonText(processor.presetName().toUpperCase());presetDisplay.setTooltip(processor.presetCategory()+" / "+processor.presetName()+" / Click to browse");}updateMacros();
    const bool lit=processor.outputLevel()>.001f;
    if(lit!=signalLed) {signalLed=lit;repaint(juce::Rectangle<float>(284,41,32,32).transformedBy(juce::AffineTransform::scale(float(getWidth())/1280,float(getHeight())/646)).getSmallestIntegerContainer());}
}
void GbaEditor::setMainControlsEnabled(bool enabled) {
    for(auto* c:std::initializer_list<juce::Component*>{&previous,&next,&presetDisplay,&start,&select,&aButton,&bButton,&dpad,&romCartridge,&volume})c->setEnabled(enabled);
    for(auto& macro:macros)macro.setEnabled(enabled);
}
void GbaEditor::showBrowser() {
    if(dynamic_cast<BrowserPanel*>(overlay.get())){closeOverlay();return;}
    overlay=std::make_unique<BrowserPanel>(processor,[this]{closeOverlay();});addAndMakeVisible(*overlay);setMainControlsEnabled(false);resized();overlay->toFront(true);
}
void GbaEditor::showAdvanced() {
    if(dynamic_cast<AdvancedPanel*>(overlay.get())){closeOverlay();return;}
    overlay=std::make_unique<AdvancedPanel>(processor,[this]{closeOverlay();},[this]{applySkin();});addAndMakeVisible(*overlay);setMainControlsEnabled(false);resized();overlay->toFront(true);
}
void GbaEditor::showSoundFonts(){
    if(dynamic_cast<SoundFontPanel*>(overlay.get())){closeOverlay();return;}
    overlay=std::make_unique<SoundFontPanel>(processor,[this]{closeOverlay();});addAndMakeVisible(*overlay);setMainControlsEnabled(false);resized();overlay->toFront(true);
}
void GbaEditor::closeOverlay() {overlay.reset();setMainControlsEnabled(true);grabKeyboardFocus();}
bool GbaEditor::isInterestedInFileDrag(const juce::StringArray& files) {
    return files.size()==1&&juce::File::isAbsolutePath(files[0])&&juce::File(files[0]).hasFileExtension("sf2");
}
void GbaEditor::fileDragEnter(const juce::StringArray& files,int,int) {soundFontDrag=isInterestedInFileDrag(files);repaint();}
void GbaEditor::fileDragExit(const juce::StringArray&) {soundFontDrag=false;repaint();}
void GbaEditor::filesDropped(const juce::StringArray& files,int,int) {
    soundFontDrag=false;repaint();
    if(!isInterestedInFileDrag(files))return;
    // Same bounded, asynchronous loader/cache as LOAD SF2. Invalid banks retain
    // the working instrument and report their error inside the ROM Hacks panel.
    processor.romHacks.loadFile(juce::File(files[0]));
    if(!dynamic_cast<SoundFontPanel*>(overlay.get()))showSoundFonts();
}
void GbaEditor::paintOverChildren(juce::Graphics& g) {
    if(!soundFontDrag)return;
    auto bounds=getLocalBounds().toFloat().reduced(10);
    g.setColour(juce::Colours::black.withAlpha(.68f));g.fillRoundedRectangle(bounds,18);
    g.setColour(uiColour(*this,0xffb792ff));g.drawRoundedRectangle(bounds.reduced(2),16,3);
    caption(g,"DROP SF2 TO LOAD ROM HACKS",bounds.withSizeKeepingCentre(bounds.getWidth()-30,50),float(getWidth())/42);
}
bool GbaEditor::keyPressed(const juce::KeyPress& key) {
    if(key==juce::KeyPress::escapeKey&&overlay){closeOverlay();return true;}
    if(key==juce::KeyPress::F1Key){showBrowser();return true;}if(key==juce::KeyPress::F2Key){showAdvanced();return true;}
    if(key==juce::KeyPress::F3Key){showSoundFonts();return true;}
    if(overlay)return false;
    if(key==juce::KeyPress::spaceKey){processor.preview();return true;}
    return dpad.keyPressed(key);
}
