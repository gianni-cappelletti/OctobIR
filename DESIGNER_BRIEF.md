# OctobIR Plugin - Asset Design Brief

**Project**: OctobIR Audio Plugin UI Graphics
**Date**: 2026-02-02
**Deliverable**: Custom UI assets for JUCE audio plugin

---

## Project Overview

OctobIR is a professional audio impulse response (IR) convolution processor plugin. We need custom graphics for a dark-themed, hardware-inspired user interface that maintains a modern, minimal aesthetic.

**Plugin Window Size**: 700×760 pixels
**Target Audience**: Professional audio engineers, music producers
**Style**: Dark theme, hardware-inspired, clean and functional

---

## Design Style Guide

### Color Palette

| Element | Color | Hex Code | Usage |
|---------|-------|----------|-------|
| Background | Very Dark Gray | `#1a1a1a` | Main plugin background |
| Panels | Dark Gray | `#2a2a2a` | LCD displays, component backgrounds |
| Borders | Medium Gray | `#444444` | Outlines, separators |
| Text | White | `#ffffff` | All text labels |
| Accent Green | Bright Green | `#00ff00` | IR A indicators, LCD text |
| Accent Blue | Cyan Blue | `#00aaff` | IR B indicators |
| Off State | Very Dark Gray | `#0a0a0a` | Inactive LEDs, LCD background |

### Typography

- **Primary Font**: Sans-serif, clean and modern
- **LCD Font**: Monospace (Inconsolata, Consolas, or system monospace)
- **Sizes**: 12pt-14pt for controls, 24pt for title

### Design Principles

1. **Contrast**: Maintain WCAG 2.1 compliance (4.5:1 minimum contrast ratio)
2. **Consistency**: Use same design language across all assets
3. **Clarity**: Controls should be immediately recognizable
4. **Subtle Effects**: Glows, shadows, and textures should enhance, not distract
5. **Retina Ready**: All assets must be exported at @1x and @2x resolutions

---

## Asset Requirements

### Total Deliverables: 20 Files
- 14 unique assets
- Each exported at @1x and @2x resolutions for retina displays
- All with proper transparency (PNG-24 with alpha channel)

---

## 1. LED Toggle Indicators (IR Enable Buttons)

**Purpose**: Visual indicators for IR A and IR B enable/disable states

**Deliverables**: 6 files (3 assets × 2 resolutions)

### Asset 1: LED Off State
- **Filename**: `led_off.png`, `led_off@2x.png`
- **Size**: 30×30px @1x, 60×60px @2x
- **Style**: Dark circular LED indicator
- **Colors**: Base `#0a0a0a`, subtle inner shadow for depth
- **Details**:
  - Circular shape, centered
  - Matte finish, no glow
  - Subtle gradient (darker at edges)
  - Inner shadow: 2px, 50% opacity, black

### Asset 2: LED On (Green)
- **Filename**: `led_on_green.png`, `led_on_green@2x.png`
- **Size**: 30×30px @1x, 60×60px @2x
- **Style**: Bright glowing green LED
- **Colors**: Center `#00ff00`, glow `#00ff00` at 30-50% opacity
- **Details**:
  - Circular shape, centered
  - Gaussian blur glow: 4-6px radius
  - Radial gradient: bright center → darker edges
  - Outer glow extending 2-3px beyond circle
  - Should look "lit up" and active

### Asset 3: LED On (Blue)
- **Filename**: `led_on_blue.png`, `led_on_blue@2x.png`
- **Size**: 30×30px @1x, 60×60px @2x
- **Style**: Bright glowing blue LED
- **Colors**: Center `#00aaff`, glow `#00aaff` at 30-50% opacity
- **Details**: Same as green LED but with blue color

**Reference Examples**:
- Hardware rack unit power indicators
- Studio equipment status LEDs
- Modern synthesizer indicator lights

---

## 2. Load Buttons (Standard Rectangular Buttons)

**Purpose**: Buttons for loading IR files

**Deliverables**: 6 files (3 states × 2 resolutions)

### Asset 4: Button Normal State
- **Filename**: `button_normal.png`, `button_normal@2x.png`
- **Size**: 80×30px @1x, 160×60px @2x
- **Style**: Rectangular button, dark theme
- **Colors**: Base `#2a2a2a`, border `#444444`
- **Details**:
  - Subtle top-to-bottom gradient (`#2e2e2e` → `#262626`)
  - 1px border around entire button
  - Slight rounded corners (2px radius) optional
  - Inner space for "Load" text (centered, ~14pt)
  - No text in graphic - will be added by code

### Asset 5: Button Hover State
- **Filename**: `button_hover.png`, `button_hover@2x.png`
- **Size**: 80×30px @1x, 160×60px @2x
- **Style**: Slightly lighter/highlighted version
- **Colors**: Base `#353535`, border `#555555`
- **Details**:
  - Slightly brighter than normal state
  - Optional: subtle glow on border
  - Same gradient pattern, just lighter

### Asset 6: Button Pressed State
- **Filename**: `button_pressed.png`, `button_pressed@2x.png`
- **Size**: 80×30px @1x, 160×60px @2x
- **Style**: Darker, "pushed in" appearance
- **Colors**: Base `#1a1a1a`, darker border `#333333`
- **Details**:
  - Darker than normal state
  - Inner shadow effect (inset, 2px)
  - Reversed gradient (lighter at top)
  - Visually appears pressed/depressed

---

## 3. LCD Display Background

**Purpose**: Background for IR filename display areas

**Deliverables**: 2 files (1 asset × 2 resolutions)

### Asset 7: LCD Background
- **Filename**: `lcd_background.png`, `lcd_background@2x.png`
- **Size**: 350×40px @1x, 700×80px @2x
- **Style**: Inset LCD screen panel
- **Colors**: Base `#0a0a0a`, border `#333333`
- **Details**:
  - Very dark background (almost black)
  - Beveled inset border (2-3px depth)
  - Subtle glass reflection on top third (white, 5% opacity gradient)
  - Optional: Scanline texture overlay (1px horizontal lines, 10% opacity, every 2px)
  - Optional: Rounded corners (2-3px radius)
  - Should look like a recessed LCD screen

**Reference Examples**:
- Digital audio interface displays
- Hardware reverb unit screens
- Calculator LCD screens

---

## 4. Rotary Knobs

**Purpose**: Knob controls for all 7 parameters

**Deliverables**: 4 files (2 sizes × 2 resolutions)

### Asset 8: Large Knob
- **Filename**: `knob_large.png`, `knob_large@2x.png`
- **Size**: 100×100px @1x, 200×200px @2x
- **Used For**: Blend and Output Gain controls (2 main parameters)
- **Style**: Circular knob with indicator
- **Details**:
  - **CRITICAL**: Center point must be mathematically exact (pixel 50,50 for @1x)
  - **CRITICAL**: Indicator (line, dot, or notch) must point straight up (12 o'clock / 0°)
  - Circular knob body: `#2a2a2a` to `#3a3a3a` gradient
  - Indicator: Bright line, dot, or notch (`#00ff00` or `#00aaff` or white)
  - Subtle drop shadow: Baked into transparent background, 2-3px offset
  - Can be flat design or subtle 3D (your choice)
  - Should look tactile and rotatable

### Asset 9: Small Knob
- **Filename**: `knob_small.png`, `knob_small@2x.png`
- **Size**: 60×60px @1x, 120×120px @2x
- **Used For**: Threshold, Range, Knee, Attack, Release (5 secondary parameters)
- **Details**:
  - **CRITICAL**: Center point must be mathematically exact (pixel 30,30 for @1x)
  - **CRITICAL**: Indicator must point straight up (12 o'clock / 0°)
  - Same design as large knob, just scaled down
  - Maintain all proportions and effects

**Design Notes**:
- These knobs will be **rotated by code** using transforms
- The indicator pointing up (12 o'clock) is the minimum value position
- Rotation range: ~270° (from 7:30 o'clock to 4:30 o'clock)
- Do NOT create multiple frames - single image only
- Ensure knob looks good at all angles when rotated

**Reference Examples**:
- Analog synthesizer knobs
- Studio outboard gear controls
- Modern plugin interfaces (FabFilter, Soundtoys)

---

## 5. Plugin Background

**Purpose**: Full-window background texture/gradient

**Deliverables**: 2 files (1 asset × 2 resolutions)

### Asset 10: Plugin Background
- **Filename**: `plugin_background.png`, `plugin_background@2x.png`
- **Size**: 700×760px @1x, 1400×1520px @2x
- **Style**: Dark textured/gradient background
- **Colors**: Base `#1a1a1a`, gradient to `#141414`
- **Details**:
  - Subtle dark gradient (top-to-bottom or radial)
  - Optional textures: Carbon fiber, brushed metal, subtle noise, wood grain
  - Panel sections: Slightly lighter areas (`#2a2a2a`) for visual grouping
    - Top section (IR loading): ~110px height
    - Middle section (meters): ~200px height
    - Bottom section (parameters): remaining space
  - Keep textures VERY subtle - controls should stand out
  - Optional: Subtle vignette (darker at edges)
  - Can use PNG-24 if transparency needed, or JPG if solid

**Visual Hierarchy**:
- Background should recede visually
- Allow controls and meters to be the focus
- Maintain professional studio aesthetic

**Reference Examples**:
- Professional plugin interfaces (Native Instruments, UAD, Waves)
- Physical rack unit faceplates
- Studio equipment enclosures

---

## Technical Requirements

### File Format
- **Format**: PNG-24 with alpha channel (except background, which can be JPG)
- **Color Mode**: RGB
- **Bit Depth**: 8-bit per channel
- **Compression**: Standard PNG compression (lossless)

### Resolution Requirements
- **@1x**: Standard resolution (e.g., 30×30px)
- **@2x**: Retina resolution (exactly 2× dimensions, e.g., 60×60px)
- **DPI**: 72 DPI for @1x, 144 DPI for @2x
- **Naming**: Use "@2x" suffix for retina files (e.g., `led_off@2x.png`)

### Critical Requirements for Knobs
1. **Pixel-Perfect Centering**: The mathematical center of the image must be the rotation point
   - Large knob: Exactly 50×50 pixels from top-left corner (@1x)
   - Small knob: Exactly 30×30 pixels from top-left corner (@1x)
2. **Indicator Alignment**: Indicator MUST point straight up (0°, 12 o'clock position)
3. **Transparency**: Use alpha channel for drop shadow, not baked-in background
4. **Square Canvas**: Width and height must be identical for proper rotation

### Delivery Structure
```
OctobIR_Assets/
├── Toggles/
│   ├── led_off.png
│   ├── led_off@2x.png
│   ├── led_on_green.png
│   ├── led_on_green@2x.png
│   ├── led_on_blue.png
│   └── led_on_blue@2x.png
├── Buttons/
│   ├── button_normal.png
│   ├── button_normal@2x.png
│   ├── button_hover.png
│   ├── button_hover@2x.png
│   ├── button_pressed.png
│   └── button_pressed@2x.png
├── LCD/
│   ├── lcd_background.png
│   └── lcd_background@2x.png
├── Knobs/
│   ├── knob_large.png
│   ├── knob_large@2x.png
│   ├── knob_small.png
│   └── knob_small@2x.png
└── Background/
    ├── plugin_background.png
    └── plugin_background@2x.png
```

---

## Design Process

### Recommended Workflow
1. **Create style frames** in Figma/Sketch/Photoshop at @1x size first
2. **Get approval** on design direction before creating all assets
3. **Export @1x versions** for initial testing
4. **Export @2x versions** once @1x is approved
5. **Provide source files** (.psd, .sketch, .fig) for future modifications

### Testing
- View assets at actual size (100% zoom)
- Test on retina displays if possible
- Check contrast ratios using WCAG tools
- Verify centering for knobs using measurement tools

---

## Reference Materials

### Inspiration Examples
- **Hardware**: Universal Audio LA-2A, Neve 1073, SSL compressors
- **Plugins**: FabFilter Pro-Q3, Soundtoys EchoBoy, Valhalla VintageVerb
- **Modern UI**: Clean, functional, studio-grade aesthetic

### Accessibility
- Text on backgrounds: Minimum 4.5:1 contrast ratio
- LED indicators: Must be clearly distinguishable when on vs off
- Button states: Obvious visual differences between normal/hover/pressed

---

## Timeline & Deliverables

**Estimated Design Time**: 7-11 hours
- LED toggles: 1-2 hours
- Buttons: 1-2 hours
- LCD background: 1 hour
- Knobs: 2-3 hours
- Background: 2-3 hours

**Delivery Format**: ZIP file containing all 20 PNG files organized in folders

---

## Questions or Clarifications?

If anything is unclear or you need reference images, please reach out. We're aiming for a professional, hardware-inspired look that audio engineers will recognize and appreciate.

Thank you!