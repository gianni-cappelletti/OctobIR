#pragma once

#include "LCDSpectrumDisplay.h"

#include <array>
#include <cmath>
#include <functional>

class GraphicEQDisplay : public juce::Component
{
 public:
  static constexpr int kNumBands = LCDSpectrumDisplay::kNumBands;
  static constexpr float kMinGainDb = -12.0f;
  static constexpr float kMaxGainDb = 12.0f;

  GraphicEQDisplay()
  {
    eqGainsDb_.fill(0.0f);
    addAndMakeVisible(spectrumDisplay_);
  }

  void setTypeface(juce::Typeface::Ptr tf) { spectrumDisplay_.setTypeface(tf); }

  void setBandLevels(const float* levelsDb, int count)
  {
    spectrumDisplay_.setBandLevels(levelsDb, count);
  }

  void setCrossoverNormPosition(float normPos)
  {
    spectrumDisplay_.setCrossoverNormPosition(normPos);
  }

  void setEQBandGain(int band, float gainDb)
  {
    if (band < 0 || band >= kNumBands)
      return;

    gainDb = juce::jlimit(kMinGainDb, kMaxGainDb, gainDb);

    if (std::fabs(eqGainsDb_[static_cast<size_t>(band)] - gainDb) > 0.001f)
    {
      eqGainsDb_[static_cast<size_t>(band)] = gainDb;
      repaint();
    }
  }

  float getEQBandGain(int band) const
  {
    if (band < 0 || band >= kNumBands)
      return 0.0f;
    return eqGainsDb_[static_cast<size_t>(band)];
  }

  std::function<void(int band, float gainDb)> onBandGainChanged;

  void resized() override { spectrumDisplay_.setBounds(getLocalBounds()); }

  void paint(juce::Graphics&) override
  {
  }

  void paintOverChildren(juce::Graphics& g) override
  {
    auto barArea = getBarArea();
    float barAreaTop = static_cast<float>(barArea.getY());
    float barAreaHeight = static_cast<float>(barArea.getHeight());
    float barAreaLeft = static_cast<float>(barArea.getX());
    float barAreaWidth = static_cast<float>(barArea.getWidth());
    float barSlotWidth = barAreaWidth / static_cast<float>(kNumBands);
    float barWidth = barSlotWidth - static_cast<float>(LCDSpectrumDisplay::kBarGap);

    // Center line at 0dB gain
    float centerY = gainToY(0.0f, barAreaTop, barAreaHeight);
    g.setColour(juce::Colour(0xff1c1c30).withAlpha(0.3f));
    g.drawHorizontalLine(static_cast<int>(centerY), barAreaLeft,
                         barAreaLeft + barAreaWidth);

    // EQ curve polyline
    juce::Path eqPath;
    for (int i = 0; i < kNumBands; ++i)
    {
      float x = barAreaLeft + static_cast<float>(i) * barSlotWidth + barWidth / 2.0f;
      float y = gainToY(eqGainsDb_[static_cast<size_t>(i)], barAreaTop, barAreaHeight);

      if (i == 0)
        eqPath.startNewSubPath(x, y);
      else
        eqPath.lineTo(x, y);
    }

    g.setColour(juce::Colour(0xff1c1c30));
    g.strokePath(eqPath, juce::PathStrokeType(2.0f));

    // Draw drag handles at each band point
    for (int i = 0; i < kNumBands; ++i)
    {
      float x = barAreaLeft + static_cast<float>(i) * barSlotWidth + barWidth / 2.0f;
      float y = gainToY(eqGainsDb_[static_cast<size_t>(i)], barAreaTop, barAreaHeight);

      float radius = (i == hoverBand_ || i == dragBand_) ? 5.0f : 3.5f;
      float alpha = (i == hoverBand_ || i == dragBand_) ? 1.0f : 0.6f;

      g.setColour(juce::Colour(0xff1c1c30).withAlpha(alpha));
      g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);
    }
  }

  void mouseMove(const juce::MouseEvent& e) override
  {
    int band = hitTestBand(e.position);
    if (band != hoverBand_)
    {
      hoverBand_ = band;
      setMouseCursor(hoverBand_ >= 0 ? juce::MouseCursor::UpDownResizeCursor
                                     : juce::MouseCursor::NormalCursor);
      repaint();
    }
  }

  void mouseDown(const juce::MouseEvent& e) override
  {
    dragBand_ = hitTestBand(e.position);
    if (dragBand_ >= 0)
    {
      dragStartGain_ = eqGainsDb_[static_cast<size_t>(dragBand_)];
      dragStartY_ = e.position.y;
    }
  }

  void mouseDrag(const juce::MouseEvent& e) override
  {
    if (dragBand_ < 0)
      return;

    auto barArea = getBarArea();
    float barAreaHeight = static_cast<float>(barArea.getHeight());

    // Full bar height maps to the full gain range
    float gainRange = kMaxGainDb - kMinGainDb;
    float deltaY = dragStartY_ - e.position.y;
    float deltaGain = (deltaY / barAreaHeight) * gainRange;

    float newGain = juce::jlimit(kMinGainDb, kMaxGainDb, dragStartGain_ + deltaGain);

    eqGainsDb_[static_cast<size_t>(dragBand_)] = newGain;
    if (onBandGainChanged)
      onBandGainChanged(dragBand_, newGain);
    repaint();
  }

  void mouseUp(const juce::MouseEvent&) override
  {
    dragBand_ = -1;
  }

  void mouseDoubleClick(const juce::MouseEvent& e) override
  {
    int band = hitTestBand(e.position);
    if (band >= 0)
    {
      eqGainsDb_[static_cast<size_t>(band)] = 0.0f;
      if (onBandGainChanged)
        onBandGainChanged(band, 0.0f);
      repaint();
    }
  }

  void mouseExit(const juce::MouseEvent&) override
  {
    if (hoverBand_ >= 0)
    {
      hoverBand_ = -1;
      setMouseCursor(juce::MouseCursor::NormalCursor);
      repaint();
    }
  }

  LCDSpectrumDisplay& getSpectrumDisplay() { return spectrumDisplay_; }

 private:
  LCDSpectrumDisplay spectrumDisplay_;
  std::array<float, kNumBands> eqGainsDb_{};

  int dragBand_ = -1;
  float dragStartGain_ = 0.0f;
  float dragStartY_ = 0.0f;
  int hoverBand_ = -1;

  juce::Rectangle<int> getBarArea() const
  {
    auto content = getLocalBounds().reduced(LCDSpectrumDisplay::kPad);
    content.removeFromLeft(LCDSpectrumDisplay::kYAxisW);
    content.removeFromTop(LCDSpectrumDisplay::kTopPad);
    content.removeFromBottom(LCDSpectrumDisplay::kXAxisH);
    return content;
  }

  float gainToY(float gainDb, float barAreaTop, float barAreaHeight) const
  {
    float normalized = (gainDb - kMinGainDb) / (kMaxGainDb - kMinGainDb);
    return barAreaTop + barAreaHeight * (1.0f - normalized);
  }

  int hitTestBand(juce::Point<float> pos) const
  {
    auto barArea = getBarArea();
    float barAreaLeft = static_cast<float>(barArea.getX());
    float barAreaTop = static_cast<float>(barArea.getY());
    float barAreaWidth = static_cast<float>(barArea.getWidth());
    float barAreaHeight = static_cast<float>(barArea.getHeight());
    float barSlotWidth = barAreaWidth / static_cast<float>(kNumBands);
    float barWidth = barSlotWidth - static_cast<float>(LCDSpectrumDisplay::kBarGap);

    constexpr float kHitRadius = 10.0f;

    for (int i = 0; i < kNumBands; ++i)
    {
      float x = barAreaLeft + static_cast<float>(i) * barSlotWidth + barWidth / 2.0f;
      float y = gainToY(eqGainsDb_[static_cast<size_t>(i)], barAreaTop, barAreaHeight);

      float dx = pos.x - x;
      float dy = pos.y - y;
      if (dx * dx + dy * dy <= kHitRadius * kHitRadius)
        return i;
    }
    return -1;
  }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicEQDisplay)
};
