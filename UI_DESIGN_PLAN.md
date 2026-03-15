# OctobIR JUCE UI Design Plan
## Custom Shape-Drawn Controls — No Image Assets

**Plugin Window Size**: 700×760 pixels
**Rendering approach**: All visuals drawn via JUCE `Graphics`/`Path` API in a `LookAndFeel_V4` subclass and standalone `Component` subclasses.

---

## Table of Contents

1. [Control Inventory & Layout](#1-control-inventory--layout)
2. [Architecture](#2-architecture)
3. [New Files](#3-new-files)
4. [Phase 1 — OctobIRLookAndFeel](#4-phase-1--octobirlookandfeel)
5. [Phase 2 — LCDDisplay Component](#5-phase-2--lcddisplay-component)
6. [Phase 3 — PluginEditor Updates](#6-phase-3--plugineditor-updates)
7. [Phase 4 — CMakeLists.txt](#7-phase-4--cmakeliststxt)
8. [Implementation Order & Checklist](#8-implementation-order--checklist)
9. [Color Reference](#9-color-reference)
10. [Spacing Guidelines](#10-spacing-guidelines)

---

## 1. Control Inventory & Layout

### Control Summary

| Category | Count | Description |
|----------|-------|-------------|
| Rotary Knobs | 7 | Large (2) + Small (5) parameter controls |
| Toggle Buttons | 4 | LED-style (2) + Standard (2) |
| Standard Buttons | 3 | Text buttons for actions (Load×2, Swap) |
| LCD Displays | 2 | IR filename displays |
| Vertical Meters | 2 | Real-time visualization |
| Combo Box | 1 | Dropdown selection |
| Static Labels | 10 | Parameter names, titles |
| **Total Interactive** | **19** | User-controllable elements |
| **Total UI Elements** | **29** | Including labels |

### Section-by-Section Control List

#### Section 1: Title Area (Top) — 50px

1. **Title Label** — "OctobIR"
   - Type: Static text label
   - Font: 24pt bold, centered
   - Color: White (`#ffffff`)

---

#### Section 2: IR Loading Area — 110px total

##### IR A (Left Side)

2. **IR A Title Label** — "IR A (-1.0)"
   - Type: Static text label, 14pt bold, white
   - Width: ~50px

3. **Load IR A Button**
   - Type: Standard button
   - Text: "Load", Size: 70×30px
   - States: Normal / Hover / Pressed

4. **IR A Enable Toggle** (LED)
   - Type: LED-style toggle button
   - Visual: Green LED on / Dark off
   - Size: 30×30px LED area
   - Label: "Enable A" beside LED
   - Parameter: `irAEnable`

5. **IR A Path Display (LCD)**
   - Type: `LCDDisplay` component (replaces `juce::Label`)
   - Size: ~200×40px
   - Text color: Green (`#00ff00`) on near-black (`#0a0a0a`)
   - Default text: "No IR loaded"

##### IR B (Right Side)

6. **IR B Title Label** — "IR B (+1.0)"
   - Type: Static text label, 14pt bold, white

7. **Load IR B Button**
   - Type: Standard button, "Load", 70×30px

8. **IR B Enable Toggle** (LED)
   - Type: LED-style toggle button
   - Visual: Blue LED on / Dark off
   - Label: "Enable B"
   - Parameter: `irBEnable`

9. **IR B Path Display (LCD)**
   - Type: `LCDDisplay` component
   - Text color: Blue (`#00aaff`) on near-black (`#0a0a0a`)

##### Mode Controls Row (Below IR Section) — 30px

10. **Dynamic Mode Toggle**
    - Type: Standard toggle button (checkbox style)
    - Text: "Dynamic Mode", Size: ~140×30px
    - Parameter: `dynamicMode`

11. **Sidechain Enable Toggle**
    - Type: Standard toggle button (checkbox style)
    - Text: "Sidechain Enable", Size: ~140×30px
    - Parameter: `sidechainEnable`

12. **Swap IR Order Button**
    - Type: Standard button
    - Text: "Swap IR Order", Size: ~120×30px
    - Action: Swaps `lowBlend`/`highBlend` parameters

---

#### Section 3: Meters Area — 200px

13. **Input Level Meter**
    - Type: `VerticalMeter` component (12 LED circles, already implemented)
    - Size: 60×200px
    - Range: -96 dB to 0 dB
    - Colors: Green → Yellow → Red
    - Features: Optional threshold markers (orange/red)
    - Label: "Input Level" above meter

14. **Blend Meter**
    - Type: `VerticalMeter` component (already implemented)
    - Size: 60×200px
    - Range: -1.0 to +1.0
    - Colors: Cyan → Gray → Orange
    - Features: Optional blend range markers (blue)
    - Label: "Blend" above meter

---

#### Section 4: Main Parameter Controls — 120px

##### Large Rotary Knobs (100×100px)

15. **Blend Knob**
    - Type: Large rotary knob (`RotaryVerticalDrag`)
    - Size: 100×100px
    - Range: -1.0 to +1.0, Default: 0.0
    - Parameter: `blend`
    - Label: "Static Blend" above knob
    - Text box: Below knob, 2 decimal places
    - Sweep: 270° (7 o'clock to 5 o'clock)

16. **Output Gain Knob**
    - Type: Large rotary knob
    - Size: 100×100px
    - Range: -24 dB to +24 dB, Default: 0 dB
    - Parameter: `outputGain`
    - Label: "Output Gain" above knob
    - Text box: Below knob, "X.X dB"

---

#### Section 5: Dynamic Mode Parameters — 180px (2 rows)

##### Row 1: Threshold Controls (Small Knobs — 60×60px)

17. **Threshold Knob**
    - Range: -60 dB to 0 dB, Default: -30 dB
    - Parameter: `threshold`
    - Label: "Threshold" above, "X.X dB" below

18. **Range Knob**
    - Range: 1 dB to 60 dB, Default: 20 dB
    - Parameter: `rangeDb`
    - Label: "Range" above, "X.X dB" below

19. **Knee Width Knob**
    - Range: 0 dB to 20 dB, Default: 5 dB
    - Parameter: `kneeWidthDb`
    - Label: "Knee" above, "X.X dB" below

##### Row 2: Timing Controls (Small Knobs — 60×60px)

20. **Attack Time Knob**
    - Range: 1 ms to 500 ms, Default: 50 ms
    - Parameter: `attackTime`
    - Label: "Attack" above, "XXX ms" below

21. **Release Time Knob**
    - Range: 1 ms to 1000 ms, Default: 200 ms
    - Parameter: `releaseTime`
    - Label: "Release" above, "XXX ms" below

22. **Detection Mode Combo Box**
    - Type: `juce::ComboBox`
    - Size: ~150×30px
    - Options: "Peak" (default), "RMS"
    - Parameter: `detectionMode`
    - Label: "Detection Mode" above

---

#### Section 6: Status Display (Bottom) — 25px

23. **Latency Label**
    - Type: Static text label (read-only)
    - Style: Centered, color `#88ccff`
    - Text: "Latency: XXX samples (X.XX ms)"
    - Updates: Real-time when IR loaded

---

### Layout Diagram

```
700px
┌─────────────────────────────────────────────────────────────────────┐
│  [1] Title: "OctobIR"  (24pt bold, centered)                       │ 50px
├─────────────────────────────────────────────────────────────────────┤
│  [2] IR A (-1.0)  [3] Load  [4] LED-A  [5] LCD-A path display     │ 60px
│  [6] IR B (+1.0)  [7] Load  [8] LED-B  [9] LCD-B path display     │
├─────────────────────────────────────────────────────────────────────┤
│  [10] Dynamic Mode    [11] Sidechain Enable    [12] Swap IR Order  │ 30px
├─────────────────────────────────────────────────────────────────────┤
│  (spacing)                                                          │ 10px
├─────────────────────────────────────────────────────────────────────┤
│          [13] Input Level Meter    [14] Blend Meter                │ 200px
│           label above                label above                    │
│           60×200px                   60×200px                      │
├─────────────────────────────────────────────────────────────────────┤
│  (spacing)                                                          │ 15px
├─────────────────────────────────────────────────────────────────────┤
│    "Static Blend"          "Output Gain"                           │ 120px
│    [15] Blend Knob         [16] Output Gain Knob                   │
│    100×100px               100×100px                               │
│    value textbox           value textbox                           │
├─────────────────────────────────────────────────────────────────────┤
│  (spacing)                                                          │ 15px
├─────────────────────────────────────────────────────────────────────┤
│  "Threshold"   "Range"     "Knee"                                  │ 90px
│  [17] Thresh   [18] Range  [19] Knee   (small knobs, 60×60px)     │
│  value box     value box   value box                               │
├─────────────────────────────────────────────────────────────────────┤
│  "Attack"      "Release"   "Detection Mode"                        │ 90px
│  [20] Attack   [21] Release  [22] Detection ComboBox               │
│  value box     value box                                           │
├─────────────────────────────────────────────────────────────────────┤
│  (spacing)                                                          │ 15px
├─────────────────────────────────────────────────────────────────────┤
│  [23] Latency: XXX samples (X.XX ms)  (cyan, centered)            │ 25px
└─────────────────────────────────────────────────────────────────────┘
  Total: 700×760px
```

---

## 2. Architecture

### Core Principle

Subclass `juce::LookAndFeel_V4` — not `_V2` or `_V3`. `LookAndFeel_V4` has concrete implementations for all virtual methods, so only the overrides needed for this plugin must be written. `_V2`/`_V3` require implementing dozens of pure virtuals.

### LookAndFeel Lifetime Rule

The LookAndFeel instance **must be declared before all widget members** in the class. JUCE components hold a raw pointer to their LookAndFeel. If the LAF is destroyed before the components that reference it, a dangling pointer use-after-free occurs during component destruction. Member declaration order controls destruction order in C++.

```cpp
class OctobIREditor : public juce::AudioProcessorEditor {
    OctobIRLookAndFeel laf_;   // FIRST — destroyed LAST
    juce::Slider blendSlider_; // declared after — destroyed before laf_
    // ...
};
```

Additionally, `setLookAndFeel(nullptr)` must be called in the destructor before member destruction begins.

### Component Hierarchy

```
OctobIREditor
├── OctobIRLookAndFeel laf_        (not a Component, governs all rendering)
├── VerticalMeter inputLevelMeter_ (custom Component, already implemented)
├── VerticalMeter blendMeter_      (custom Component, already implemented)
├── LCDDisplay ir1LCDDisplay_      (new custom Component)
├── LCDDisplay ir2LCDDisplay_      (new custom Component)
├── juce::Slider blendSlider_      (RotaryVerticalDrag, styled via laf_)
├── juce::Slider outputGainSlider_ (RotaryVerticalDrag, styled via laf_)
├── juce::Slider thresholdSlider_  (RotaryVerticalDrag, styled via laf_)
├── juce::Slider rangeDbSlider_    (RotaryVerticalDrag, styled via laf_)
├── juce::Slider kneeWidthDbSlider_(RotaryVerticalDrag, styled via laf_)
├── juce::Slider attackTimeSlider_ (RotaryVerticalDrag, styled via laf_)
├── juce::Slider releaseTimeSlider_(RotaryVerticalDrag, styled via laf_)
├── juce::ToggleButton ir1EnableButton_ (LED style via laf_)
├── juce::ToggleButton ir2EnableButton_ (LED style via laf_)
├── juce::ToggleButton dynamicModeButton_   (standard toggle via laf_)
├── juce::ToggleButton sidechainEnableButton_(standard toggle via laf_)
├── juce::TextButton loadButton1_ / loadButton2_ / swapIROrderButton_
├── juce::ComboBox detectionModeCombo_
└── juce::Label (title, IR titles, parameter labels, latency)
```

---

## 3. New Files

| File | Purpose |
|------|---------|
| `Source/OctobIRLookAndFeel.h` | LookAndFeel class declaration |
| `Source/OctobIRLookAndFeel.cpp` | All draw method overrides |
| `Source/LCDDisplay.h` | LCD panel component (header-only) |

`plugins/juce-multiformat/CMakeLists.txt` must add `Source/OctobIRLookAndFeel.cpp` to `target_sources`.

---

## 4. Phase 1 — OctobIRLookAndFeel

### Class Declaration (`OctobIRLookAndFeel.h`)

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class OctobIRLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OctobIRLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height,
                      bool isButtonDown, int buttonX, int buttonY,
                      int buttonW, int buttonH, juce::ComboBox&) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon,
                           const juce::Colour* textColour) override;
};
```

### Constructor — Colour Registration

All component colours are set once in the constructor. Individual components then use `findColour()` to retrieve them, keeping the colour palette centralized.

```cpp
OctobIRLookAndFeel::OctobIRLookAndFeel()
{
    // Window background
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1a1a));

    // Rotary knob
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff333333)); // track bg
    setColour(juce::Slider::rotarySliderFillColourId,    juce::Colour(0xff00aaff)); // value arc
    setColour(juce::Slider::thumbColourId,               juce::Colours::white);
    setColour(juce::Slider::textBoxTextColourId,         juce::Colour(0xffaaaaaa));
    setColour(juce::Slider::textBoxBackgroundColourId,   juce::Colour(0xff111111));
    setColour(juce::Slider::textBoxOutlineColourId,      juce::Colour(0xff333333));

    // Text buttons
    setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a2a2a));
    setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff3a3a3a));
    setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    setColour(juce::TextButton::textColourOnId,  juce::Colours::white);

    // ComboBox
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff444444));
    setColour(juce::ComboBox::textColourId,       juce::Colours::white);
    setColour(juce::ComboBox::arrowColourId,      juce::Colour(0xff888888));

    // Popup menu
    setColour(juce::PopupMenu::backgroundColourId,
              juce::Colour(0xff1a1a1a));
    setColour(juce::PopupMenu::textColourId,      juce::Colours::white);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,
              juce::Colour(0xff00aaff));
    setColour(juce::PopupMenu::highlightedTextColourId,
              juce::Colours::white);

    // Labels
    setColour(juce::Label::textColourId, juce::Colours::white);
}
```

### `drawRotarySlider` — Design

The knob uses a three-layer rendering:

1. **Background arc track** — full sweep range, outline colour, 8px stroke, rounded caps
2. **Value arc** — from start angle to current angle, fill colour, same stroke weight
3. **Knob cap** — filled dark circle at center (radius 55% of total), with subtle radial gradient for depth
4. **Indicator dot** — small filled circle on the arc edge at the current value angle

**Angle convention:**
- `rotaryStartAngle = pi * 1.25` ≈ 7 o'clock position (set via `setRotaryParameters`)
- `rotaryEndAngle   = pi * 2.75` ≈ 5 o'clock position
- Current angle: `toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle)`
- Point on circle at angle `a`: `x = cx + r * std::sin(a)`, `y = cy - r * std::cos(a)`

```cpp
void OctobIRLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider& slider)
{
    auto bounds    = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
    auto radius    = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX   = bounds.getCentreX();
    auto centreY   = bounds.getCentreY();
    auto lineW     = juce::jmin(8.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;
    auto toAngle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background arc track
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
    g.strokePath(bgArc, juce::PathStrokeType(
        lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                            0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
    g.strokePath(valueArc, juce::PathStrokeType(
        lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob cap with radial gradient for depth
    auto capRadius = radius * 0.55f;
    juce::Point<float> centre(centreX, centreY);
    juce::ColourGradient capGradient(
        juce::Colour(0xff454545),
        centre.translated(-capRadius * 0.3f, -capRadius * 0.3f),
        juce::Colour(0xff1a1a1a),
        centre.translated(capRadius * 0.5f, capRadius * 0.5f),
        true);
    g.setGradientFill(capGradient);
    g.fillEllipse(centreX - capRadius, centreY - capRadius,
                  capRadius * 2.0f, capRadius * 2.0f);
    g.setColour(juce::Colour(0xff555555));
    g.drawEllipse(centreX - capRadius, centreY - capRadius,
                  capRadius * 2.0f, capRadius * 2.0f, 1.0f);

    // Indicator dot at arc position
    auto dotRadius = lineW * 0.9f;
    juce::Point<float> dotPt(
        centreX + arcRadius * std::sin(toAngle),
        centreY - arcRadius * std::cos(toAngle));
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    g.fillEllipse(juce::Rectangle<float>(dotRadius * 2.0f, dotRadius * 2.0f)
                      .withCentre(dotPt));
}
```

### `drawButtonBackground` — Standard and Action Buttons

Three visual states using `Colour::brighter()`/`darker()` to avoid hardcoded per-state colours.

```cpp
void OctobIRLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    const auto cornerSize = 4.0f;

    auto base = backgroundColour.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if (shouldDrawButtonAsDown)
        base = base.darker(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.1f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}
```

### `drawButtonText` — Centered with Press Offset

A 1px downward shift when pressed gives a tactile feel.

```cpp
void OctobIRLookAndFeel::drawButtonText(
    juce::Graphics& g, juce::TextButton& button,
    bool /*isHighlighted*/, bool isButtonDown)
{
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(button.findColour(button.getToggleState()
                    ? juce::TextButton::textColourOnId
                    : juce::TextButton::textColourOffId)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

    const int offset = isButtonDown ? 1 : 0;
    g.drawFittedText(button.getButtonText(),
                     offset, offset, button.getWidth(), button.getHeight(),
                     juce::Justification::centred, 1);
}
```

### `drawToggleButton` — LED vs Standard Dispatch

LED buttons are identified at runtime by a property set in the editor constructor (`button.getProperties()["isLED"] == true`). The method dispatches to one of two internal rendering paths.

#### Standard Toggle (Dynamic Mode, Sidechain Enable)

- Dark rounded rect background, same as action buttons
- When toggled ON: left-edge accent stripe (2px, colour `#00aaff`) + slightly brighter background
- Centered white text

#### LED Toggle (IR A Enable, IR B Enable)

The LED colour is read from `juce::ToggleButton::tickColourId`, set per-button in the editor:
- IR A: `#00ff00` (green)
- IR B: `#00aaff` (blue)

**ON state rendering:**
1. Glow halo: radial `ColourGradient` from `ledColour.withAlpha(0.5f)` at center to transparent at `radius * 1.8f`
2. LED body: radial gradient from `ledColour.brighter(0.5f)` (highlight, offset top-left) to `ledColour.darker(0.3f)` (shadow, offset bottom-right)
3. Rim: `juce::Colours::black.withAlpha(0.7f)` stroke

**OFF state rendering:**
1. No glow
2. LED body: very dark version (`ledColour.withSaturation(0.3f).withBrightness(0.1f)`)
3. Same rim stroke

**Text:** Rendered below the LED circle if `button.getButtonText()` is non-empty.

```cpp
void OctobIRLookAndFeel::drawToggleButton(
    juce::Graphics& g, juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    if (button.getProperties()["isLED"])
    {
        // LED rendering path — see detail above
        auto bounds  = button.getLocalBounds().toFloat();
        auto ledSize = juce::jmin(bounds.getHeight() * 0.6f, 24.0f);
        auto ledBounds = juce::Rectangle<float>(bounds.getX() + 4.0f,
                                                bounds.getCentreY() - ledSize * 0.5f,
                                                ledSize, ledSize);
        auto centre  = ledBounds.getCentre();
        auto radius  = ledSize * 0.5f;
        auto isOn    = button.getToggleState();
        auto ledColour = button.findColour(juce::ToggleButton::tickColourId);

        if (isOn)
        {
            auto glowR = radius * 1.8f;
            juce::ColourGradient glow(ledColour.withAlpha(0.5f), centre,
                                      ledColour.withAlpha(0.0f),
                                      centre.translated(glowR, 0.0f), true);
            g.setGradientFill(glow);
            g.fillEllipse(centre.x - glowR, centre.y - glowR, glowR * 2.0f, glowR * 2.0f);
        }

        auto innerColour = isOn ? ledColour.brighter(0.5f) : ledColour.withSaturation(0.3f).withBrightness(0.1f);
        auto outerColour = isOn ? ledColour.darker(0.3f)   : ledColour.withSaturation(0.2f).withBrightness(0.05f);
        juce::ColourGradient body(
            innerColour, centre.translated(-radius * 0.3f, -radius * 0.3f),
            outerColour, centre.translated(radius * 0.7f,   radius * 0.7f),
            true);
        g.setGradientFill(body);
        g.fillEllipse(ledBounds);

        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.drawEllipse(ledBounds, 1.0f);

        if (button.getButtonText().isNotEmpty())
        {
            g.setColour(juce::Colours::white.withAlpha(button.isEnabled() ? 0.9f : 0.5f));
            g.setFont(12.0f);
            auto textBounds = bounds.withLeft(ledBounds.getRight() + 4.0f);
            g.drawFittedText(button.getButtonText(), textBounds.toNearestInt(),
                             juce::Justification::centredLeft, 1);
        }
    }
    else
    {
        // Standard toggle rendering path
        auto bounds     = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
        const auto cornerSize = 4.0f;
        auto isOn       = button.getToggleState();
        auto base = button.findColour(juce::TextButton::buttonColourId)
                          .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

        if (isOn)
            base = base.brighter(0.12f);
        if (shouldDrawButtonAsHighlighted)
            base = base.brighter(0.08f);
        if (shouldDrawButtonAsDown)
            base = base.darker(0.12f);

        g.setColour(base);
        g.fillRoundedRectangle(bounds, cornerSize);

        // Accent stripe on left edge when toggled on
        if (isOn)
        {
            g.setColour(juce::Colour(0xff00aaff));
            g.fillRoundedRectangle(bounds.withWidth(2.0f), cornerSize);
        }

        g.setColour(juce::Colour(0xff444444));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(button.isEnabled() ? 0.9f : 0.5f));
        g.setFont(getTextButtonFont(button, button.getHeight()));
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                         juce::Justification::centred, 1);
    }
}
```

### `drawComboBox` — Dark Box with Chevron

```cpp
void OctobIRLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
    juce::ComboBox& box)
{
    const auto cornerSize = 3.0f;
    juce::Rectangle<float> boxBounds(0.0f, 0.0f, (float)width, (float)height);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(boxBounds, cornerSize);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(boxBounds.reduced(0.5f, 0.5f), cornerSize, 1.0f);

    // Chevron arrow
    juce::Rectangle<int> arrowZone(width - 30, 0, 20, height);
    juce::Path arrow;
    arrow.startNewSubPath((float)arrowZone.getX() + 3.0f,
                           (float)arrowZone.getCentreY() - 2.0f);
    arrow.lineTo((float)arrowZone.getCentreX(),
                 (float)arrowZone.getCentreY() + 3.0f);
    arrow.lineTo((float)arrowZone.getRight() - 3.0f,
                 (float)arrowZone.getCentreY() - 2.0f);

    g.setColour(box.findColour(juce::ComboBox::arrowColourId)
                    .withAlpha(box.isEnabled() ? 0.9f : 0.2f));
    g.strokePath(arrow, juce::PathStrokeType(2.0f));
}
```

### `drawPopupMenuItem` — Dark Menu with Accent Highlight

Override to match the dark theme and provide visible hover state using the blue accent colour.

---

## 5. Phase 2 — LCDDisplay Component

Header-only `juce::Component` subclass. Replaces `juce::Label` for the two IR path display fields.

### Design

- Rounded rect, near-black fill (`#0a0a0a`)
- Inner bevel: outer stroke in pure black, inner stroke in white at low opacity — creates the appearance of a recessed panel
- Monospace font, left-justified, vertically centered
- Text colour set per-instance (green for IR A, blue for IR B)
- Default text: "No IR loaded"

```cpp
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class LCDDisplay : public juce::Component
{
public:
    LCDDisplay()  { setOpaque(false); }

    void setText(const juce::String& text)
    {
        text_ = text;
        repaint();
    }

    void setTextColour(juce::Colour colour)
    {
        textColour_ = colour;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);

        // Recessed background
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRoundedRectangle(bounds, 3.0f);

        // Outer bevel (dark edge)
        g.setColour(juce::Colours::black.withAlpha(0.9f));
        g.drawRoundedRectangle(bounds, 3.0f, 1.5f);

        // Inner highlight (simulates depth)
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(bounds.reduced(1.5f), 2.0f, 1.0f);

        // Text
        g.setFont(juce::Font(
            juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));
        g.setColour(textColour_.withAlpha(isEnabled() ? 1.0f : 0.4f));
        g.drawFittedText(text_,
                         getLocalBounds().reduced(6, 4),
                         juce::Justification::centredLeft, 1, 0.9f);
    }

private:
    juce::String text_      = "No IR loaded";
    juce::Colour textColour_ = juce::Colour(0xff00ff00);
};
```

---

## 6. Phase 3 — PluginEditor Updates

### Header Changes (`PluginEditor.h`)

1. Add includes:
   ```cpp
   #include "OctobIRLookAndFeel.h"
   #include "LCDDisplay.h"
   ```

2. Declare `OctobIRLookAndFeel laf_` as the **first private member** (before all widget members).

3. Replace LCD path label declarations:
   ```cpp
   // Remove:
   juce::Label ir1PathLabel_;
   juce::Label ir2PathLabel_;

   // Add:
   LCDDisplay ir1LCDDisplay_;
   LCDDisplay ir2LCDDisplay_;
   ```

### Constructor Changes (`PluginEditor.cpp`)

**LookAndFeel — set immediately after construction:**
```cpp
setLookAndFeel(&laf_);
```

**Destructor — mandatory teardown:**
```cpp
OctobIREditor::~OctobIREditor()
{
    setLookAndFeel(nullptr);
}
```

**Slider style — all 7 sliders change from default horizontal to rotary:**
```cpp
auto setupRotarySlider = [](juce::Slider& s) {
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    s.setRotaryParameters(
        juce::MathConstants<float>::pi * 1.25f,
        juce::MathConstants<float>::pi * 2.75f, true);
};

setupRotarySlider(blendSlider_);
setupRotarySlider(outputGainSlider_);
setupRotarySlider(thresholdSlider_);
setupRotarySlider(rangeDbSlider_);
setupRotarySlider(kneeWidthDbSlider_);
setupRotarySlider(attackTimeSlider_);
setupRotarySlider(releaseTimeSlider_);
```

**LED button configuration:**
```cpp
ir1EnableButton_.getProperties().set("isLED", true);
ir1EnableButton_.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00ff00));
ir1EnableButton_.setButtonText("Enable A");

ir2EnableButton_.getProperties().set("isLED", true);
ir2EnableButton_.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00aaff));
ir2EnableButton_.setButtonText("Enable B");
```

**LCD display configuration:**
```cpp
ir1LCDDisplay_.setTextColour(juce::Colour(0xff00ff00)); // green
ir2LCDDisplay_.setTextColour(juce::Colour(0xff00aaff)); // blue

addAndMakeVisible(ir1LCDDisplay_);
addAndMakeVisible(ir2LCDDisplay_);
```

Replace all `ir1PathLabel_.setText(...)` → `ir1LCDDisplay_.setText(...)` and same for ir2.

**Editor `paint` — background fill:**
```cpp
void OctobIREditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}
```

### `resized()` Layout Update

Large knobs (100×100px) need ~120px vertical slots (label above + knob + text box). Small knobs (60×60px) need ~90px vertical slots. Use sequential `removeFromTop` slices:

```
Total height: 760px

Title area:          removeFromTop(50)
IR loading rows:     removeFromTop(110)  → 2× 50px rows + 10px padding
Mode buttons row:    removeFromTop(30)
Spacing:             removeFromTop(10)
Meters:              removeFromTop(200)
Spacing:             removeFromTop(15)
Large knobs:         removeFromTop(120)
Spacing:             removeFromTop(15)
Small knobs row 1:   removeFromTop(90)
Small knobs row 2:   removeFromTop(90)
Spacing:             removeFromTop(15)
Latency label:       remainder (~25px)
```

Within the large knob row, position each knob using horizontal splits:
```cpp
auto knobRow = area.removeFromTop(120);
// Center two knobs with spacing
auto blendArea = knobRow.removeFromLeft(knobRow.getWidth() / 2);
blendLabel_.setBounds(blendArea.removeFromTop(20));
blendSlider_.setBounds(blendArea.withSizeKeepingCentre(100, 100));
// repeat for outputGain
```

Within small knob rows (3 knobs each), divide width into thirds:
```cpp
auto row1 = area.removeFromTop(90);
auto w = row1.getWidth() / 3;
for each of (threshold, range, knee):
    auto cell = row1.removeFromLeft(w);
    label.setBounds(cell.removeFromTop(20));
    slider.setBounds(cell.withSizeKeepingCentre(60, 60));
```

---

## 7. Phase 4 — CMakeLists.txt

Add the new source file to the existing `target_sources` block in `plugins/juce-multiformat/CMakeLists.txt`:

```cmake
target_sources(OctobIR PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/OctobIRLookAndFeel.cpp)   # add this line
```

`LCDDisplay.h` is header-only and requires no CMake change.

---

## 8. Implementation Order & Checklist

Work in this order to allow incremental visual verification at each step.

### Step 1 — CMakeLists.txt
- [ ] Add `Source/OctobIRLookAndFeel.cpp` to `target_sources`

### Step 2 — LCDDisplay.h
- [ ] Create `Source/LCDDisplay.h`
- [ ] Implement `paint`, `setText`, `setTextColour`
- [ ] Verify no-text ("No IR loaded") and with-text states render correctly

### Step 3 — OctobIRLookAndFeel skeleton
- [ ] Create `Source/OctobIRLookAndFeel.h` with class declaration
- [ ] Create `Source/OctobIRLookAndFeel.cpp` with constructor colours only
- [ ] Run `make juce` — confirm clean build (no functional change yet)

### Step 4 — Integrate LookAndFeel into Editor
- [ ] Add `laf_` as first private member in `PluginEditor.h`
- [ ] `setLookAndFeel(&laf_)` in constructor
- [ ] `setLookAndFeel(nullptr)` in destructor
- [ ] Fill background in `paint()`
- [ ] Run `make juce` — confirm background colour changes

### Step 5 — Convert sliders to rotary knobs
- [ ] Apply `setupRotarySlider` to all 7 sliders in constructor
- [ ] Update `resized()` to use rotary knob layout dimensions
- [ ] Implement `drawRotarySlider` override
- [ ] Run `make juce` — verify knobs render with correct sweep and text boxes

### Step 6 — Standard buttons and toggles
- [ ] Implement `drawButtonBackground` and `drawButtonText`
- [ ] Implement standard toggle path in `drawToggleButton`
- [ ] Run `make juce` — verify buttons and Dynamic Mode / Sidechain toggles

### Step 7 — LED toggle buttons
- [ ] Set `isLED` property and `tickColourId` on `ir1EnableButton_` and `ir2EnableButton_`
- [ ] Implement LED path in `drawToggleButton`
- [ ] Run `make juce` — verify green and blue LED states, glow on/off

### Step 8 — LCD Displays
- [ ] Add `LCDDisplay` members to editor header
- [ ] Replace `ir1PathLabel_`/`ir2PathLabel_` with `LCDDisplay` instances
- [ ] Update all `setText` call sites
- [ ] Update `resized()` bounds for LCD components
- [ ] Run `make juce` — verify green/blue LCD text on dark background

### Step 9 — Combo Box
- [ ] Implement `drawComboBox`
- [ ] Implement `drawPopupMenuItem`
- [ ] Run `make juce` — verify dropdown appearance and selection

### Step 10 — Final pass
- [ ] Verify all WCAG 2.1 contrast ratios (minimum 4.5:1 for text)
- [ ] Test at plugin window size (700×760px) — not zoomed in
- [ ] Verify meter repaint performance (30 Hz, no visible stutter)
- [ ] Confirm `setLookAndFeel(nullptr)` destructor call with JUCE leak detector (no leaks reported)

---

## 9. Color Reference

### Backgrounds
| Element | Hex | Usage |
|---------|-----|-------|
| Window background | `#1a1a1a` | `paint()` fill |
| Panel / component bg | `#2a2a2a` | Buttons, combo box |
| LCD background | `#0a0a0a` | LCDDisplay fill |

### Text
| Element | Hex | Usage |
|---------|-----|-------|
| Primary text | `#ffffff` | Labels, button text |
| LCD text (IR A) | `#00ff00` | Green phosphor |
| LCD text (IR B) | `#00aaff` | Blue phosphor |
| Status / latency | `#88ccff` | Latency label |
| Slider value text | `#aaaaaa` | Text box below knob |

### Borders & Tracks
| Element | Hex | Usage |
|---------|-----|-------|
| Primary borders | `#444444` | Button outlines, combo |
| Knob arc track | `#333333` | Background sweep arc |
| Text box outline | `#333333` | Slider text box border |

### Accents & Indicators
| Element | Hex | Usage |
|---------|-----|-------|
| Value arc / IR B accent | `#00aaff` | Knob fill arc, toggle stripe |
| IR A / meter fill | `#00ff00` | Green LED, meter start |
| Meter yellow band | `#ffff00` | Meter mid-range |
| Threshold orange | `#ff6600` | Meter threshold low |
| Threshold red | `#ff0000` | Meter threshold high |
| Knob thumb dot | `#ffffff` | Indicator dot |

---

## 10. Spacing Guidelines

| Context | Value |
|---------|-------|
| Window margin | 15px from edge |
| Between major sections | 10–15px |
| Between adjacent controls | 10–20px |
| Between small knobs | 10px |
| Label to control | 5px |
| LCD internal padding | 6px horizontal, 4px vertical |
| Button corner radius | 4px |
| ComboBox corner radius | 3px |
| LED glow radius multiplier | 1.8× LED radius |
