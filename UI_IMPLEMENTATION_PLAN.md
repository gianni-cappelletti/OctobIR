# OctobIR UI Enhancement - Implementation Plan

**Status**: Ready for Implementation
**Last Updated**: 2026-02-02
**Estimated Time**: 22-31 hours total

---

## Executive Summary

This plan outlines the complete implementation of custom graphics and UI enhancements for the OctobIR JUCE plugin, including:
- IR enable/disable LED toggles (green for A, blue for B)
- Professional rotary knobs for all 7 parameters
- LCD-style displays for IR filenames
- Custom background graphic
- Unified visual theme via CustomLookAndFeel

---

## Design Decisions (Confirmed)

✅ **Knob Style**: Rotation-based (single image rotated in code)
✅ **Slider Conversion**: All 7 parameters converted to rotary knobs
✅ **LCD Implementation**: LookAndFeel styling (no custom component needed)
✅ **Background**: Custom graphic (700×760px background image)

---

## Asset Requirements

See `DESIGNER_BRIEF.md` for complete designer specifications.

**Summary**: 20 asset files total
- LED toggles: 6 files (off, on-green, on-blue × @1x/@2x)
- Buttons: 6 files (normal, hover, pressed × @1x/@2x)
- LCD: 2 files (background × @1x/@2x)
- Knobs: 4 files (large, small × @1x/@2x)
- Background: 2 files (plugin background × @1x/@2x)

---

## Phase 1: Infrastructure Setup (6-8 hours)

### 1.1 Create Directory Structure
```bash
cd /Users/giannitestprojects/Code/OctobIR/plugins/juce-multiformat
mkdir -p Resources/{Toggles,Buttons,LCD,Knobs,Background}
```

### 1.2 Place Assets
- Receive assets from designer (20 PNG files)
- Organize into appropriate subdirectories
- Verify naming conventions match CMakeLists.txt expectations

### 1.3 Update CMakeLists.txt

**File**: `plugins/juce-multiformat/CMakeLists.txt`

Add after `juce_add_plugin()` call:
```cmake
# Create binary data target for embedded assets
juce_add_binary_data(OctobIRBinaryData
    SOURCES
        # LED Toggle indicators
        Resources/Toggles/led_off.png
        Resources/Toggles/led_off@2x.png
        Resources/Toggles/led_on_green.png
        Resources/Toggles/led_on_green@2x.png
        Resources/Toggles/led_on_blue.png
        Resources/Toggles/led_on_blue@2x.png

        # Load buttons
        Resources/Buttons/button_normal.png
        Resources/Buttons/button_normal@2x.png
        Resources/Buttons/button_hover.png
        Resources/Buttons/button_hover@2x.png
        Resources/Buttons/button_pressed.png
        Resources/Buttons/button_pressed@2x.png

        # LCD display background
        Resources/LCD/lcd_background.png
        Resources/LCD/lcd_background@2x.png

        # Rotary knobs
        Resources/Knobs/knob_large.png
        Resources/Knobs/knob_large@2x.png
        Resources/Knobs/knob_small.png
        Resources/Knobs/knob_small@2x.png

        # Plugin background
        Resources/Background/plugin_background.png
        Resources/Background/plugin_background@2x.png
)

# Link binary data to plugin target
target_link_libraries(OctobIR
    PRIVATE
        OctobIRBinaryData  # ADD THIS LINE
        octobir-core
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

### 1.4 Test Build
```bash
cd /Users/giannitestprojects/Code/OctobIR
make juce
```

**Verify**:
- Build succeeds with no errors
- `BinaryData.h` generated in build directory
- All asset references present in `BinaryData` namespace

### 1.5 Checklist
- [ ] Directory structure created
- [ ] Assets placed in correct folders
- [ ] CMakeLists.txt updated
- [ ] Build successful
- [ ] BinaryData.h generated

---

## Phase 2: CustomLookAndFeel Implementation (10-14 hours)

### 2.1 Create CustomLookAndFeel Class

**New File**: `plugins/juce-multiformat/Source/CustomLookAndFeel.h`

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace octob
{

class OctobIRLookAndFeel : public juce::LookAndFeel_V4
{
 public:
  OctobIRLookAndFeel();
  ~OctobIRLookAndFeel() override = default;

  // Toggle button rendering (LED indicators)
  void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

  // Standard button rendering (Load buttons)
  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

  // Rotary slider rendering (knobs)
  void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider& slider) override;

  // Label rendering (LCD displays)
  void drawLabel(juce::Graphics& g, juce::Label& label) override;

  // Color palette
  juce::Colour getBackgroundColor() const { return juce::Colour(0xff1a1a1a); }
  juce::Colour getPanelColor() const { return juce::Colour(0xff2a2a2a); }
  juce::Colour getTextColor() const { return juce::Colours::white; }
  juce::Colour getAccentGreen() const { return juce::Colour(0xff00ff00); }
  juce::Colour getAccentBlue() const { return juce::Colour(0xff00aaff); }

 private:
  // Asset images
  juce::Image ledOff_;
  juce::Image ledOnGreen_;
  juce::Image ledOnBlue_;
  juce::Image buttonNormal_;
  juce::Image buttonHover_;
  juce::Image buttonPressed_;
  juce::Image lcdBackground_;
  juce::Image knobLarge_;
  juce::Image knobSmall_;

  // Fonts
  juce::Font lcdFont_;

  // Helper methods
  void loadAssets();
  bool isIREnableToggle(const juce::ToggleButton& button) const;
  bool isIRAToggle(const juce::ToggleButton& button) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIRLookAndFeel)
};

}  // namespace octob
```

**New File**: `plugins/juce-multiformat/Source/CustomLookAndFeel.cpp`

See full implementation in detailed plan file (`~/.claude/plans/concurrent-bubbling-steele.md` section 2.1).

Key methods to implement:
- `loadAssets()` - Load all images from BinaryData
- `drawToggleButton()` - Render LED indicators
- `drawButtonBackground()` - Render Load buttons with states
- `drawRotarySlider()` - Render knobs with rotation
- `drawLabel()` - Render LCD display styling

### 2.2 Integrate LookAndFeel into PluginEditor

**File**: `plugins/juce-multiformat/Source/PluginEditor.h`

Add include and member:
```cpp
#include "CustomLookAndFeel.h"

class OctobIREditor : public juce::AudioProcessorEditor, private juce::Timer
{
 private:
  octob::OctobIRLookAndFeel customLookAndFeel_;  // ADD
  // ... existing members
};
```

**File**: `plugins/juce-multiformat/Source/PluginEditor.cpp`

In constructor (FIRST LINE):
```cpp
OctobIREditor::OctobIREditor(OctobIRProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      inputLevelMeter_("Input Level", -96.0f, 0.0f),
      blendMeter_("Blend", -1.0f, 1.0f)
{
  setLookAndFeel(&customLookAndFeel_);  // ADD THIS FIRST

  // Set component names for LookAndFeel identification
  irPathLabel_.setName("irAPathLCD");
  ir2PathLabel_.setName("irBPathLCD");

  // ... rest of constructor
}
```

In destructor (FIRST LINE):
```cpp
OctobIREditor::~OctobIREditor()
{
  setLookAndFeel(nullptr);  // ADD THIS FIRST
  stopTimer();
}
```

### 2.3 Add IR Enable Toggle Buttons

**File**: `plugins/juce-multiformat/Source/PluginEditor.h`

Add members:
```cpp
juce::ToggleButton irAEnableButton_;
std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> irAEnableAttachment_;

juce::ToggleButton irBEnableButton_;
std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> irBEnableAttachment_;
```

**File**: `plugins/juce-multiformat/Source/PluginEditor.cpp`

In constructor (after existing buttons):
```cpp
addAndMakeVisible(irAEnableButton_);
irAEnableButton_.setButtonText("Enable A");
irAEnableButton_.setName("irAEnableToggle");
irAEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
    audioProcessor.getAPVTS(), "irAEnable", irAEnableButton_);

addAndMakeVisible(irBEnableButton_);
irBEnableButton_.setButtonText("Enable B");
irBEnableButton_.setName("irBEnableToggle");
irBEnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
    audioProcessor.getAPVTS(), "irBEnable", irBEnableButton_);
```

In `resized()` method, update IR sections:
```cpp
// IR A section
auto ir1Section = irRow.removeFromLeft(getWidth() / 2 - 20);
ir1TitleLabel_.setBounds(ir1Section.removeFromLeft(50));
loadButton_.setBounds(ir1Section.removeFromLeft(70).reduced(2));
irAEnableButton_.setBounds(ir1Section.removeFromLeft(80).reduced(2));  // NEW
irPathLabel_.setBounds(ir1Section.reduced(2));

// IR B section
auto ir2Section = irRow;
ir2TitleLabel_.setBounds(ir2Section.removeFromLeft(50));
loadButton2_.setBounds(ir2Section.removeFromLeft(70).reduced(2));
irBEnableButton_.setBounds(ir2Section.removeFromLeft(80).reduced(2));  // NEW
ir2PathLabel_.setBounds(ir2Section.reduced(2));
```

### 2.4 Test Phase 2
```bash
make juce
```

**Visual Tests**:
- [ ] IR A Enable toggle shows green LED when on
- [ ] IR B Enable toggle shows blue LED when on
- [ ] Both toggles show dark LED when off
- [ ] Load buttons show hover/pressed states
- [ ] LCD displays have dark inset appearance
- [ ] All colors match design spec

**Functional Tests**:
- [ ] Toggling IR A Enable affects audio processing
- [ ] Toggling IR B Enable affects audio processing
- [ ] Buttons control parameters correctly

---

## Phase 3: Rotary Slider Conversion (3-4 hours)

### 3.1 Convert Slider Styles

**File**: `plugins/juce-multiformat/Source/PluginEditor.cpp`

For each of the 7 sliders, change from LinearHorizontal to Rotary.

**Example (Blend slider)**:
```cpp
// OLD:
blendSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
blendSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);

// NEW:
blendSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
blendSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
blendSlider_.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                  juce::MathConstants<float>::pi * 2.75f,
                                  true);  // 270° rotation range
blendSlider_.setName("blendKnob");  // For LookAndFeel sizing
```

**Apply to all 7 sliders**:
1. `blendSlider_` → Large knob
2. `outputGainSlider_` → Large knob
3. `thresholdSlider_` → Small knob
4. `rangeDbSlider_` → Small knob
5. `kneeWidthDbSlider_` → Small knob
6. `attackTimeSlider_` → Small knob
7. `releaseTimeSlider_` → Small knob

### 3.2 Update CustomLookAndFeel Knob Rendering

**File**: `plugins/juce-multiformat/Source/CustomLookAndFeel.cpp`

Modify `drawRotarySlider()`:
```cpp
void OctobIRLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
  // Determine knob size based on slider name
  bool useLargeKnob = (slider.getName().contains("blend") ||
                       slider.getName().contains("outputGain"));

  juce::Image& knobImage = useLargeKnob ? knobLarge_ : knobSmall_;

  if (knobImage.isValid())
  {
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto centerPoint = bounds.getCentre();

    // Create rotation transform around center
    juce::AffineTransform transform =
        juce::AffineTransform::rotation(toAngle, centerPoint.x, centerPoint.y);

    g.drawImageTransformed(knobImage, transform);
  }
  else
  {
    // Fallback to JUCE default
    LookAndFeel_V4::drawRotarySlider(g, x, y, width, height, sliderPosProportional,
                                     rotaryStartAngle, rotaryEndAngle, slider);
  }
}
```

### 3.3 Redesign Parameter Layout

**File**: `plugins/juce-multiformat/Source/PluginEditor.cpp`

Update `resized()` method to accommodate rotary knobs:

```cpp
// After meters section
bounds.removeFromTop(15);

// Large knobs row (Blend, Output Gain)
auto largeKnobRow = bounds.removeFromTop(120);
auto blendArea = largeKnobRow.removeFromLeft(150);
blendLabel_.setBounds(blendArea.removeFromTop(20));
blendSlider_.setBounds(blendArea);

largeKnobRow.removeFromLeft(20);  // Spacer

auto gainArea = largeKnobRow.removeFromLeft(150);
outputGainLabel_.setBounds(gainArea.removeFromTop(20));
outputGainSlider_.setBounds(gainArea);

bounds.removeFromTop(15);

// Small knobs row 1 (Threshold, Range, Knee)
auto smallKnobRow1 = bounds.removeFromTop(90);
auto thresholdArea = smallKnobRow1.removeFromLeft(100);
thresholdLabel_.setBounds(thresholdArea.removeFromTop(20));
thresholdSlider_.setBounds(thresholdArea);

smallKnobRow1.removeFromLeft(10);

auto rangeArea = smallKnobRow1.removeFromLeft(100);
rangeDbLabel_.setBounds(rangeArea.removeFromTop(20));
rangeDbSlider_.setBounds(rangeArea);

smallKnobRow1.removeFromLeft(10);

auto kneeArea = smallKnobRow1.removeFromLeft(100);
kneeWidthDbLabel_.setBounds(kneeArea.removeFromTop(20));
kneeWidthDbSlider_.setBounds(kneeArea);

bounds.removeFromTop(10);

// Small knobs row 2 (Attack, Release) + Detection ComboBox
auto smallKnobRow2 = bounds.removeFromTop(90);
auto attackArea = smallKnobRow2.removeFromLeft(100);
attackTimeLabel_.setBounds(attackArea.removeFromTop(20));
attackTimeSlider_.setBounds(attackArea);

smallKnobRow2.removeFromLeft(10);

auto releaseArea = smallKnobRow2.removeFromLeft(100);
releaseTimeLabel_.setBounds(releaseArea.removeFromTop(20));
releaseTimeSlider_.setBounds(releaseArea);

smallKnobRow2.removeFromLeft(20);

auto detectionArea = smallKnobRow2.removeFromLeft(150);
detectionModeLabel_.setBounds(detectionArea.removeFromTop(20));
detectionModeCombo_.setBounds(detectionArea.removeFromTop(30));

bounds.removeFromTop(15);

// Latency label at bottom
latencyLabel_.setBounds(bounds.removeFromTop(25));
```

### 3.4 Test Phase 3
```bash
make juce
```

**Visual Tests**:
- [ ] All 7 sliders now display as rotary knobs
- [ ] Large knobs (Blend, Output Gain) are 100×100px
- [ ] Small knobs (others) are 60×60px
- [ ] Knobs rotate smoothly when dragged
- [ ] Knob indicators point correctly at all positions
- [ ] Text boxes show values below knobs

**Functional Tests**:
- [ ] All knobs control their parameters correctly
- [ ] Parameter ranges correct (e.g., Blend -1 to +1)
- [ ] Value displays update in real-time
- [ ] Knobs respond to mouse drag (vertical/horizontal)

---

## Phase 4: Background Image Integration (1-2 hours)

### 4.1 Load Background Image

**File**: `plugins/juce-multiformat/Source/PluginEditor.h`

Add member variable:
```cpp
juce::Image backgroundImage_;
```

**File**: `plugins/juce-multiformat/Source/PluginEditor.cpp`

In constructor (after setLookAndFeel):
```cpp
// Load background image
backgroundImage_ = juce::ImageCache::getFromMemory(
    BinaryData::plugin_background_png,
    BinaryData::plugin_background_pngSize);
```

### 4.2 Render Background

Update `paint()` method:
```cpp
void OctobIREditor::paint(juce::Graphics& g)
{
  // Draw background image if available
  if (backgroundImage_.isValid())
  {
    g.drawImage(backgroundImage_, getLocalBounds().toFloat());
  }
  else
  {
    // Fallback to solid color
    g.fillAll(juce::Colour(0xff1a1a1a));
  }

  // Draw title on top of background
  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
  g.drawText("OctobIR", getLocalBounds().removeFromTop(50),
             juce::Justification::centred, true);
}
```

### 4.3 Test Phase 4
```bash
make juce
```

**Visual Tests**:
- [ ] Background image renders at full plugin size
- [ ] Title text visible on top of background
- [ ] Colors/textures complement controls
- [ ] Retina version loads on HiDPI displays
- [ ] No visual artifacts or stretching

---

## Phase 5: Testing & Refinement (2-3 hours)

### 5.1 Build Verification

**Clean build**:
```bash
cd /Users/giannitestprojects/Code/OctobIR
rm -rf build/dev-juce
make juce
```

**Expected output**:
- Zero compilation errors
- Zero compilation warnings
- All plugin formats build: VST3, AU, Standalone

### 5.2 Visual Inspection Checklist

- [ ] IR A Enable: Green LED glows when on, dark when off
- [ ] IR B Enable: Blue LED glows when on, dark when off
- [ ] Load buttons: Show normal/hover/pressed states clearly
- [ ] LCD displays: Dark inset with monospace text
- [ ] All 7 knobs: Render correctly and rotate smoothly
- [ ] Background: Loads and displays properly
- [ ] Color scheme: Consistent dark theme throughout
- [ ] Layout: No overlapping components
- [ ] Meters: Still functioning correctly
- [ ] Text: All labels readable and aligned

### 5.3 Functional Testing

**Toggle Behavior**:
1. Click IR A Enable → LED lights green, IR A processing enabled
2. Click IR B Enable → LED lights blue, IR B processing enabled
3. Disable both → Verify pass-through or silence
4. Enable one, disable other → Verify solo behavior

**Knob Controls**:
1. Drag each knob → Verify parameter changes
2. Check parameter ranges (Blend: -1 to +1, Gain: -24 to +24, etc.)
3. Verify text boxes update in real-time
4. Test double-click → Reset to default value
5. Test all 7 knobs individually

**Button Interactions**:
1. Hover Load button → Highlight effect visible
2. Click Load button → Pressed state, file dialog opens
3. Load IR file → LCD displays filename
4. Load long filename → Text readable (may truncate)

**Parameter Synchronization**:
1. Adjust knob → Verify APVTS parameter updates
2. Automate parameter (if host supports) → Knob follows
3. Save/reload plugin state → All parameters restored

### 5.4 Performance Testing

- [ ] Plugin loads within 2 seconds
- [ ] UI repaints smoothly (30Hz meter updates)
- [ ] Knob rotation smooth, no stuttering
- [ ] Asset loading doesn't delay initialization
- [ ] No CPU spikes during UI interaction

### 5.5 Cross-Platform Testing (macOS)

- [ ] VST3: Builds and runs in DAW
- [ ] AU: Validates with auval, loads in Logic/GarageBand
- [ ] Standalone: Launches independently
- [ ] Retina display: @2x assets load correctly, no blurriness
- [ ] Standard display: @1x assets load correctly

---

## Success Criteria

### Phase 1 Success
✅ Assets embedded in binary
✅ Build succeeds with no errors
✅ BinaryData.h generated with all asset references

### Phase 2 Success
✅ CustomLookAndFeel renders all components correctly
✅ IR enable toggles show LED indicators (green/blue)
✅ Load buttons show state changes
✅ LCD displays styled correctly
✅ No functional regressions

### Phase 3 Success
✅ All 7 sliders converted to rotary knobs
✅ Knobs rotate smoothly and accurately
✅ Large/small knobs render at correct sizes
✅ Parameters controllable via knobs
✅ Text boxes positioned below knobs

### Phase 4 Success
✅ Background image loads and renders
✅ Full plugin window covered
✅ Retina support working
✅ No performance impact

### Phase 5 Success
✅ All visual tests pass
✅ All functional tests pass
✅ Performance tests pass
✅ Cross-platform tests pass
✅ Zero bugs or crashes

### Overall Success
✅ Professional, cohesive UI
✅ Improved usability (clear states)
✅ WCAG 2.1 accessibility maintained
✅ No performance degradation
✅ Designer assets integrated perfectly

---

## Rollback Plan

If critical issues arise, phases can be rolled back:

**Phase 4 Rollback** (Remove background):
- Comment out background image loading in constructor
- Revert `paint()` to solid color fill

**Phase 3 Rollback** (Revert to linear sliders):
- Change all sliders back to `LinearHorizontal` style
- Revert `resized()` to original layout

**Phase 2 Rollback** (Remove CustomLookAndFeel):
- Comment out `setLookAndFeel(&customLookAndFeel_);`
- Comment out IR enable button creation
- Plugin reverts to default JUCE styling

**Phase 1 Rollback** (Remove asset embedding):
- Comment out `juce_add_binary_data()` in CMakeLists.txt
- Remove link to `OctobIRBinaryData`
- Clean rebuild

---

## Timeline Summary

| Phase | Description | Time | Cumulative |
|-------|-------------|------|------------|
| 1 | Infrastructure & assets | 6-8h | 6-8h |
| 2 | CustomLookAndFeel implementation | 10-14h | 16-22h |
| 3 | Rotary slider conversion | 3-4h | 19-26h |
| 4 | Background integration | 1-2h | 20-28h |
| 5 | Testing & refinement | 2-3h | 22-31h |

**Critical Path**: Sequential - each phase depends on previous completion

---

## File Manifest

### New Files Created
- `plugins/juce-multiformat/Source/CustomLookAndFeel.h`
- `plugins/juce-multiformat/Source/CustomLookAndFeel.cpp`
- `plugins/juce-multiformat/Resources/` directory tree with 20 asset files

### Modified Files
- `plugins/juce-multiformat/CMakeLists.txt`
- `plugins/juce-multiformat/Source/PluginEditor.h`
- `plugins/juce-multiformat/Source/PluginEditor.cpp`

### No Changes Required
- `plugins/juce-multiformat/Source/PluginProcessor.h` (already has irAEnable/irBEnable params)
- `plugins/juce-multiformat/Source/PluginProcessor.cpp`
- Core library files (untouched)

---

## Notes

- This plan assumes designer assets are available before implementation
- All code follows .clang-format rules (Google C++ Style)
- All changes maintain C++14 compatibility
- WCAG 2.1 accessibility considered in all design decisions
- VCV Rack plugin UI is separate and not addressed in this plan

---

**Ready to Begin**: Once designer delivers assets to `Resources/` directories