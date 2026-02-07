# OctobIR UI Control Inventory
## Complete List for Figma Layout Design

**Plugin Window Size**: 700×760 pixels

---

## Control Summary

| Category | Count | Description |
|----------|-------|-------------|
| Rotary Knobs | 7 | Large (2) + Small (5) parameter controls |
| Toggle Buttons | 4 | LED-style (2) + Standard (2) |
| Standard Buttons | 3 | Text buttons for actions |
| LCD Displays | 2 | IR filename displays |
| Vertical Meters | 2 | Real-time visualization |
| Combo Box | 1 | Dropdown selection |
| Static Labels | 10 | Parameter names, titles |
| **Total Interactive** | **19** | User-controllable elements |
| **Total UI Elements** | **29** | Including labels |

---

## Detailed Control List

### Section 1: Title Area (Top)
**Height**: ~50px

1. **Title Label** - "OctobIR"
   - Type: Static text label
   - Style: Large, bold, centered
   - Size: 24pt font
   - Color: White (#ffffff)
   - Position: Centered, top of window

---

### Section 2: IR Loading Area (Top)
**Height**: ~110px

#### IR A (Left Side)

2. **IR A Title Label** - "IR A (-1.0)"
   - Type: Static text label
   - Style: Bold, 14pt
   - Color: White
   - Width: ~50px

3. **Load IR A Button**
   - Type: Standard button
   - Text: "Load"
   - Size: 70×30px
   - States: Normal, Hover, Pressed
   - Action: Opens file dialog for IR A

4. **IR A Enable Toggle**
   - Type: LED-style toggle button
   - Visual: Green LED (on) / Dark (off)
   - Size: 30×30px LED + text
   - Label: "Enable A"
   - Parameter: `irAEnable` (boolean)

5. **IR A Path Display (LCD)**
   - Type: LCD-style text display
   - Purpose: Shows loaded IR A filename
   - Size: ~200×40px
   - Style: Dark inset, monospace font
   - Text: Filename or "No IR loaded"
   - Color: Green text (#00ff00) on dark (#0a0a0a)

#### IR B (Right Side)

6. **IR B Title Label** - "IR B (+1.0)"
   - Type: Static text label
   - Style: Bold, 14pt
   - Color: White
   - Width: ~50px

7. **Load IR B Button**
   - Type: Standard button
   - Text: "Load"
   - Size: 70×30px
   - States: Normal, Hover, Pressed
   - Action: Opens file dialog for IR B

8. **IR B Enable Toggle**
   - Type: LED-style toggle button
   - Visual: Blue LED (on) / Dark (off)
   - Size: 30×30px LED + text
   - Label: "Enable B"
   - Parameter: `irBEnable` (boolean)

9. **IR B Path Display (LCD)**
   - Type: LCD-style text display
   - Purpose: Shows loaded IR B filename
   - Size: ~200×40px
   - Style: Dark inset, monospace font
   - Text: Filename or "No IR loaded"
   - Color: Blue text (#00aaff) on dark (#0a0a0a)

#### Mode Controls (Below IR Section)

10. **Dynamic Mode Toggle**
    - Type: Standard toggle button (checkbox style)
    - Text: "Dynamic Mode"
    - Size: ~140×30px
    - Parameter: `dynamicMode` (boolean)
    - Visual: Checked/unchecked state

11. **Sidechain Enable Toggle**
    - Type: Standard toggle button (checkbox style)
    - Text: "Sidechain Enable"
    - Size: ~140×30px
    - Parameter: `sidechainEnable` (boolean)
    - Visual: Checked/unchecked state

12. **Swap IR Order Button**
    - Type: Standard button
    - Text: "Swap IR Order"
    - Size: ~120×30px
    - States: Normal, Hover, Pressed
    - Action: Swaps lowBlend/highBlend parameters

---

### Section 3: Meters Area (Middle)
**Height**: ~200px

13. **Input Level Meter**
    - Type: Custom vertical meter component
    - Purpose: Shows input signal level
    - Size: 60×200px
    - Range: -96 dB to 0 dB
    - Style: Dark background, green-to-yellow gradient fill
    - Features: Optional threshold markers (orange/red)
    - Label: "Input Level" (above meter)

14. **Blend Meter**
    - Type: Custom vertical meter component
    - Purpose: Shows current blend position
    - Size: 60×200px
    - Range: -1.0 to +1.0
    - Style: Dark background, green-to-yellow gradient fill
    - Features: Optional blend range markers (blue)
    - Label: "Blend" (above meter)

---

### Section 4: Main Parameter Controls (Below Meters)
**Height**: ~120px

#### Large Rotary Knobs (Primary Controls)

15. **Blend Knob**
    - Type: Large rotary knob
    - Size: 100×100px
    - Range: -1.0 to +1.0
    - Default: 0.0
    - Parameter: `blend`
    - Label: "Static Blend" (above knob)
    - Text Box: Below knob, shows value (2 decimals)
    - Rotation: 270° (7:30 to 4:30 o'clock)

16. **Output Gain Knob**
    - Type: Large rotary knob
    - Size: 100×100px
    - Range: -24 dB to +24 dB
    - Default: 0 dB
    - Parameter: `outputGain`
    - Label: "Output Gain" (above knob)
    - Text Box: Below knob, shows "X.X dB"
    - Rotation: 270°

---

### Section 5: Dynamic Mode Parameters (Below Main Controls)
**Height**: ~180px (2 rows of small knobs)

#### Row 1: Threshold Controls

17. **Threshold Knob**
    - Type: Small rotary knob
    - Size: 60×60px
    - Range: -60 dB to 0 dB
    - Default: -30 dB
    - Parameter: `threshold`
    - Label: "Threshold" (above knob)
    - Text Box: Below knob, shows "X.X dB"
    - Rotation: 270°

18. **Range Knob**
    - Type: Small rotary knob
    - Size: 60×60px
    - Range: 1 dB to 60 dB
    - Default: 20 dB
    - Parameter: `rangeDb`
    - Label: "Range" (above knob)
    - Text Box: Below knob, shows "X.X dB"
    - Rotation: 270°

19. **Knee Width Knob**
    - Type: Small rotary knob
    - Size: 60×60px
    - Range: 0 dB to 20 dB
    - Default: 5 dB
    - Parameter: `kneeWidthDb`
    - Label: "Knee" (above knob)
    - Text Box: Below knob, shows "X.X dB"
    - Rotation: 270°

#### Row 2: Timing Controls

20. **Attack Time Knob**
    - Type: Small rotary knob
    - Size: 60×60px
    - Range: 1 ms to 500 ms
    - Default: 50 ms
    - Parameter: `attackTime`
    - Label: "Attack Time" (above knob)
    - Text Box: Below knob, shows "XXX ms" (integer)
    - Rotation: 270°

21. **Release Time Knob**
    - Type: Small rotary knob
    - Size: 60×60px
    - Range: 1 ms to 1000 ms
    - Default: 200 ms
    - Parameter: `releaseTime`
    - Label: "Release Time" (above knob)
    - Text Box: Below knob, shows "XXX ms" (integer)
    - Rotation: 270°

22. **Detection Mode Combo Box**
    - Type: Dropdown menu (combo box)
    - Size: ~150×30px
    - Options: "Peak" (default), "RMS"
    - Parameter: `detectionMode` (0 = Peak, 1 = RMS)
    - Label: "Detection Mode" (above combo box)
    - Style: Dark background, white text

---

### Section 6: Status Display (Bottom)
**Height**: ~25px

23. **Latency Label**
    - Type: Static text label (read-only display)
    - Purpose: Shows plugin latency
    - Style: Centered, cyan color (#88ccff)
    - Text: "Latency: XXX samples (X.XX ms)"
    - Size: Full width, 25px height
    - Updates: Real-time when IR loaded

---

## Layout Zones for Figma

### Recommended Grouping

```
┌─────────────────────────────────────────────────┐
│  [1] Title: "OctobIR"                          │  50px
├─────────────────────────────────────────────────┤
│  [2-5] IR A Section    │  [6-9] IR B Section   │  60px
├────────────────────────┴────────────────────────┤
│  [10] Dynamic  [11] Sidechain  [12] Swap       │  30px
├─────────────────────────────────────────────────┤
│  (spacing)                                      │  10px
├─────────────────────────────────────────────────┤
│  [13] Input Meter      [14] Blend Meter        │  200px
├─────────────────────────────────────────────────┤
│  (spacing)                                      │  15px
├─────────────────────────────────────────────────┤
│  [15] Blend Knob       [16] Gain Knob          │  120px
├─────────────────────────────────────────────────┤
│  (spacing)                                      │  15px
├─────────────────────────────────────────────────┤
│  [17] Thresh  [18] Range  [19] Knee            │  90px
├─────────────────────────────────────────────────┤
│  [20] Attack  [21] Release  [22] Detection     │  90px
├─────────────────────────────────────────────────┤
│  (spacing)                                      │  15px
├─────────────────────────────────────────────────┤
│  [23] Latency Display                          │  25px
└─────────────────────────────────────────────────┘
    Total: 700×760px
```

---

## Control Type Summary for Figma Components

### Create These Component Types:

1. **Large Rotary Knob** (100×100px)
   - Base knob graphic
   - Indicator at 12 o'clock
   - Label above
   - Value text box below
   - Used 2× (Blend, Output Gain)

2. **Small Rotary Knob** (60×60px)
   - Same design as large, scaled down
   - Label above
   - Value text box below
   - Used 5× (Threshold, Range, Knee, Attack, Release)

3. **LED Toggle Button** (30×30px + text)
   - Green LED version (IR A)
   - Blue LED version (IR B)
   - Dark off state
   - Label text beside
   - Used 2×

4. **Standard Toggle Button** (140×30px)
   - Checkbox style
   - Checked/unchecked states
   - Text inside button
   - Used 2× (Dynamic Mode, Sidechain Enable)

5. **Standard Button** (70-120×30px)
   - 3 states: Normal, Hover, Pressed
   - Text centered
   - Used 3× (Load A, Load B, Swap)

6. **LCD Display** (200-350×40px)
   - Dark inset background
   - Monospace text
   - Green or blue text color
   - Used 2× (IR A path, IR B path)

7. **Vertical Meter** (60×200px)
   - Dark background
   - Gradient fill (green to yellow)
   - Label above
   - Used 2× (Input Level, Blend)

8. **Combo Box** (150×30px)
   - Dropdown style
   - Label above
   - Dark background, white text
   - Used 1× (Detection Mode)

9. **Text Label** (various sizes)
   - Parameter names (12-14pt)
   - Title (24pt bold)
   - Status text (12pt)
   - Used 10×

---

## Spacing Guidelines

- **Vertical Sections**: 10-15px spacing between major sections
- **Horizontal Spacing**: 10-20px between adjacent controls
- **Knob Groups**: 10px spacing between small knobs
- **Label-to-Control**: 5px spacing from label to control
- **Margins**: 15px from window edge

---

## Color Reference for Figma

### Backgrounds
- Main background: `#1a1a1a`
- Panel/component background: `#2a2a2a`
- LCD background: `#0a0a0a`

### Text
- Primary text: `#ffffff` (white)
- LCD text (IR A): `#00ff00` (green)
- LCD text (IR B): `#00aaff` (blue)
- Status text: `#88ccff` (cyan)

### Borders
- Primary borders: `#444444`
- Dark borders: `#333333`
- Lighter borders: `#555555`

### Accents
- IR A accent: `#00ff00` (green)
- IR B accent: `#00aaff` (blue)
- Meter fill start: `#00ff00` (green)
- Meter fill end: `#ffff00` (yellow)
- Threshold low: `#ff6600` (orange)
- Threshold high: `#ff0000` (red)

---

## Interactive States to Design

### For Knobs:
- Resting state (indicator at default position)
- Show rotation range (270° arc)
- Highlight state (optional, for mouse-over)

### For Buttons:
- Normal (resting)
- Hover (mouse over)
- Pressed (clicked)
- Active/Checked (for toggles)

### For LEDs:
- Off (dark)
- On Green (glowing)
- On Blue (glowing)

### For LCD Displays:
- Empty state: "No IR loaded"
- With filename: "Long_Impulse_Response_Name.wav"
- Consider text overflow/truncation

---

## Notes for Designer

1. **Knob Indicator**: Must point straight up (12 o'clock) in base graphic
2. **Retina Support**: Design at @2x (1400×1520px) then scale down
3. **Consistency**: Use same design language across all knobs/buttons
4. **Readability**: Ensure text is legible at actual size (not just zoomed in)
5. **Accessibility**: Maintain 4.5:1 contrast ratio for text
6. **Meter Gradients**: Green → Yellow gradient for signal meters
7. **LED Glows**: Use Gaussian blur for authentic glow effect

---

This inventory covers all 29 UI elements in the plugin. Use this as your checklist when creating the Figma layout!