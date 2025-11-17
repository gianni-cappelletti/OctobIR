#include <osdialog.h>

#include <octobir-core/IRProcessor.hpp>
#include <string>

#include "plugin.hpp"

struct OpcVcvIr final : Module {
  enum ParamId {
    GAIN_PARAM,
    BLEND_PARAM,
    LOW_BLEND_PARAM,
    HIGH_BLEND_PARAM,
    LOW_THRESHOLD_PARAM,
    HIGH_THRESHOLD_PARAM,
    ATTACK_TIME_PARAM,
    RELEASE_TIME_PARAM,
    OUTPUT_GAIN_PARAM,
    PARAMS_LEN
  };
  enum InputId { INPUT_L, INPUT_R, SIDECHAIN_L, SIDECHAIN_R, INPUTS_LEN };
  enum OutputId { OUTPUT_L, OUTPUT_R, OUTPUTS_LEN };
  enum LightId { DYNAMIC_MODE_LIGHT, SIDECHAIN_LIGHT, LIGHTS_LEN };

  octob::IRProcessor irProcessor_;
  std::string loaded_file_path_;
  std::string loaded_file_path2_;

  bool dynamicModeEnabled_ = false;
  bool sidechainEnabled_ = false;

  int sample_rate_check_counter_ = 0;
  static constexpr int kSampleRateCheckInterval = 4096;
  uint32_t last_system_sample_rate_ = 0;

  OpcVcvIr() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(GAIN_PARAM, -200.f, 200.f, 0.f, "Gain", " dB", 0.f, 0.1f);
    paramQuantities[GAIN_PARAM]->snapEnabled = true;

    configParam(BLEND_PARAM, -1.f, 1.f, 0.f, "Static Blend");
    configParam(LOW_BLEND_PARAM, -1.f, 1.f, -1.f, "Low Blend");
    configParam(HIGH_BLEND_PARAM, -1.f, 1.f, 1.f, "High Blend");
    configParam(LOW_THRESHOLD_PARAM, -60.f, 0.f, -40.f, "Low Threshold", " dB");
    configParam(HIGH_THRESHOLD_PARAM, -60.f, 0.f, -10.f, "High Threshold", " dB");
    configParam(ATTACK_TIME_PARAM, 1.f, 1000.f, 50.f, "Attack Time", " ms");
    configParam(RELEASE_TIME_PARAM, 1.f, 1000.f, 200.f, "Release Time", " ms");
    configParam(OUTPUT_GAIN_PARAM, -24.f, 24.f, 0.f, "Output Gain", " dB");

    configInput(INPUT_L, "Audio Input L");
    configInput(INPUT_R, "Audio Input R");
    configInput(SIDECHAIN_L, "Sidechain Input L");
    configInput(SIDECHAIN_R, "Sidechain Input R");
    configOutput(OUTPUT_L, "Audio Output L");
    configOutput(OUTPUT_R, "Audio Output R");

    configLight(DYNAMIC_MODE_LIGHT, "Dynamic Mode Active");
    configLight(SIDECHAIN_LIGHT, "Sidechain Active");

    last_system_sample_rate_ = static_cast<uint32_t>(APP->engine->getSampleRate());
    irProcessor_.setSampleRate(APP->engine->getSampleRate());

    std::string home_dir = getenv("HOME") ? getenv("HOME") : "";
    std::string ir_path = home_dir + "/ir_sample.wav";
    LoadIR(ir_path);
  }

  void LoadIR(const std::string& file_path) {
    std::string error;
    if (irProcessor_.loadImpulseResponse(file_path, error)) {
      loaded_file_path_ = file_path;
      INFO("Loaded IR A: %s (%zu samples, %.0f Hz)", file_path.c_str(),
           irProcessor_.getIRNumSamples(), irProcessor_.getIRSampleRate());
    } else {
      WARN("Failed to load IR A file %s: %s", file_path.c_str(), error.c_str());
      loaded_file_path_.clear();
    }
  }

  void LoadIR2(const std::string& file_path) {
    std::string error;
    if (irProcessor_.loadImpulseResponse2(file_path, error)) {
      loaded_file_path2_ = file_path;
      INFO("Loaded IR B: %s (%zu samples, %.0f Hz)", file_path.c_str(),
           irProcessor_.getIR2NumSamples(), irProcessor_.getIR2SampleRate());
    } else {
      WARN("Failed to load IR B file %s: %s", file_path.c_str(), error.c_str());
      loaded_file_path2_.clear();
    }
  }

  void process(const ProcessArgs& args) override {
    if (++sample_rate_check_counter_ >= kSampleRateCheckInterval) {
      sample_rate_check_counter_ = 0;

      if (irProcessor_.isIRLoaded()) {
        auto current_system_sample_rate = static_cast<uint32_t>(args.sampleRate);
        if (current_system_sample_rate != last_system_sample_rate_) {
          INFO("System sample rate changed from %u to %u Hz, updating IR processor...",
               last_system_sample_rate_, current_system_sample_rate);
          last_system_sample_rate_ = current_system_sample_rate;
          irProcessor_.setSampleRate(args.sampleRate);
        }
      }
    }

    float gain_db = params[GAIN_PARAM].getValue() * 0.1f;
    float gain = dsp::dbToAmplitude(gain_db);

    irProcessor_.setDynamicModeEnabled(dynamicModeEnabled_);
    irProcessor_.setSidechainEnabled(sidechainEnabled_);
    irProcessor_.setBlend(params[BLEND_PARAM].getValue());
    irProcessor_.setLowBlend(params[LOW_BLEND_PARAM].getValue());
    irProcessor_.setHighBlend(params[HIGH_BLEND_PARAM].getValue());
    irProcessor_.setLowThreshold(params[LOW_THRESHOLD_PARAM].getValue());
    irProcessor_.setHighThreshold(params[HIGH_THRESHOLD_PARAM].getValue());
    irProcessor_.setAttackTime(params[ATTACK_TIME_PARAM].getValue());
    irProcessor_.setReleaseTime(params[RELEASE_TIME_PARAM].getValue());
    irProcessor_.setOutputGain(params[OUTPUT_GAIN_PARAM].getValue());

    lights[DYNAMIC_MODE_LIGHT].setBrightness(dynamicModeEnabled_ ? 1.0f : 0.0f);
    lights[SIDECHAIN_LIGHT].setBrightness(sidechainEnabled_ ? 1.0f : 0.0f);

    bool leftInputConnected = inputs[INPUT_L].isConnected();
    bool rightInputConnected = inputs[INPUT_R].isConnected();
    bool sidechainLConnected = inputs[SIDECHAIN_L].isConnected();
    bool sidechainRConnected = inputs[SIDECHAIN_R].isConnected();
    bool hasSidechain = sidechainLConnected || sidechainRConnected;

    if (dynamicModeEnabled_ && sidechainEnabled_ && hasSidechain) {
      if (leftInputConnected && rightInputConnected) {
        float inputL = inputs[INPUT_L].getVoltage() * gain;
        float inputR = inputs[INPUT_R].getVoltage() * gain;
        float sidechainL = sidechainLConnected ? inputs[SIDECHAIN_L].getVoltage() * gain : inputL;
        float sidechainR = sidechainRConnected ? inputs[SIDECHAIN_R].getVoltage() * gain
                                               : (sidechainLConnected ? sidechainL : inputR);
        float outputL = 0.0f;
        float outputR = 0.0f;

        irProcessor_.processStereoWithSidechain(&inputL, &inputR, &sidechainL, &sidechainR,
                                                &outputL, &outputR, 1);

        outputs[OUTPUT_L].setVoltage(outputL);
        outputs[OUTPUT_R].setVoltage(outputR);
      } else if (leftInputConnected) {
        float input = inputs[INPUT_L].getVoltage() * gain;
        float sidechain = sidechainLConnected ? inputs[SIDECHAIN_L].getVoltage() * gain : input;
        float output = 0.0f;

        irProcessor_.processMonoWithSidechain(&input, &sidechain, &output, 1);

        outputs[OUTPUT_L].setVoltage(output);
        outputs[OUTPUT_R].setVoltage(output);
      } else if (rightInputConnected) {
        float input = inputs[INPUT_R].getVoltage() * gain;
        float sidechain = sidechainRConnected ? inputs[SIDECHAIN_R].getVoltage() * gain : input;
        float output = 0.0f;

        irProcessor_.processMonoWithSidechain(&input, &sidechain, &output, 1);

        outputs[OUTPUT_L].setVoltage(output);
        outputs[OUTPUT_R].setVoltage(output);
      } else {
        outputs[OUTPUT_L].setVoltage(0.0f);
        outputs[OUTPUT_R].setVoltage(0.0f);
      }
    } else {
      if (leftInputConnected && rightInputConnected) {
        float inputL = inputs[INPUT_L].getVoltage() * gain;
        float inputR = inputs[INPUT_R].getVoltage() * gain;
        float outputL = 0.0f;
        float outputR = 0.0f;

        irProcessor_.processStereo(&inputL, &inputR, &outputL, &outputR, 1);

        outputs[OUTPUT_L].setVoltage(outputL);
        outputs[OUTPUT_R].setVoltage(outputR);
      } else if (leftInputConnected) {
        float input = inputs[INPUT_L].getVoltage() * gain;
        float output = 0.0f;

        irProcessor_.processMono(&input, &output, 1);

        outputs[OUTPUT_L].setVoltage(output);
        outputs[OUTPUT_R].setVoltage(output);
      } else if (rightInputConnected) {
        float input = inputs[INPUT_R].getVoltage() * gain;
        float output = 0.0f;

        irProcessor_.processMono(&input, &output, 1);

        outputs[OUTPUT_L].setVoltage(output);
        outputs[OUTPUT_R].setVoltage(output);
      } else {
        outputs[OUTPUT_L].setVoltage(0.0f);
        outputs[OUTPUT_R].setVoltage(0.0f);
      }
    }
  }

  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "filePath", json_string(loaded_file_path_.c_str()));
    json_object_set_new(rootJ, "filePath2", json_string(loaded_file_path2_.c_str()));
    json_object_set_new(rootJ, "dynamicModeEnabled", json_boolean(dynamicModeEnabled_));
    json_object_set_new(rootJ, "sidechainEnabled", json_boolean(sidechainEnabled_));
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    json_t* filePathJ = json_object_get(rootJ, "filePath");
    if (filePathJ) {
      std::string path = json_string_value(filePathJ);
      if (!path.empty()) {
        LoadIR(path);
      }
    }

    json_t* filePath2J = json_object_get(rootJ, "filePath2");
    if (filePath2J) {
      std::string path = json_string_value(filePath2J);
      if (!path.empty()) {
        LoadIR2(path);
      }
    }

    json_t* dynamicModeJ = json_object_get(rootJ, "dynamicModeEnabled");
    if (dynamicModeJ) {
      dynamicModeEnabled_ = json_boolean_value(dynamicModeJ);
    }

    json_t* sidechainJ = json_object_get(rootJ, "sidechainEnabled");
    if (sidechainJ) {
      sidechainEnabled_ = json_boolean_value(sidechainJ);
    }
  }
};

struct IrFileDisplay : app::LedDisplayChoice {
  OpcVcvIr* module;
  bool isIR2;

  IrFileDisplay(bool ir2 = false) : module(nullptr), isIR2(ir2) {
    fontPath = asset::plugin(plugin_instance, "res/font/Inconsolata_Condensed-Bold.ttf");
    color = nvgRGB(0x12, 0x12, 0x12);
    bgColor = nvgRGB(0xd9, 0x81, 0x29);
    text = "<No IR selected>";
  }

  void onButton(const widget::Widget::ButtonEvent& e) override {
    if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT && module) {
      openFileDialog();
    }
  }

  void openFileDialog() {
    osdialog_filters* filters = osdialog_filters_parse("Audio files:wav,aiff,flac;All files:*");
    char* path = osdialog_file(OSDIALOG_OPEN, nullptr, nullptr, filters);

    if (path) {
      if (isIR2) {
        module->LoadIR2(std::string(path));
      } else {
        module->LoadIR(std::string(path));
      }
      updateDisplayText();
      std::free(path);
    }

    osdialog_filters_free(filters);
  }

  void updateDisplayText() {
    if (!module) {
      text = "<No IR selected>";
      return;
    }

    std::string& path = isIR2 ? module->loaded_file_path2_ : module->loaded_file_path_;
    if (path.empty()) {
      text = "<No IR selected>";
    } else {
      size_t pos = path.find_last_of("/\\");
      std::string filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;

      if (filename.length() > 20) {
        filename = filename.substr(0, 17) + "...";
      }
      text = filename;
    }
  }

  void step() override {
    updateDisplayText();
    app::LedDisplayChoice::step();
  }
};

struct OpcVcvIrWidget final : ModuleWidget {
  explicit OpcVcvIrWidget(OpcVcvIr* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(plugin_instance, "res/opc-vcv-ir-panel.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(
        Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    auto* fileDisplay = createWidget<IrFileDisplay>(mm2px(Vec(5.08, 12.0)));
    fileDisplay->box.size = mm2px(Vec(30.48, 10.0));
    fileDisplay->module = module;
    fileDisplay->isIR2 = false;
    addChild(fileDisplay);

    auto* fileDisplay2 = createWidget<IrFileDisplay>(mm2px(Vec(5.08, 23.0)));
    fileDisplay2->box.size = mm2px(Vec(30.48, 10.0));
    fileDisplay2->module = module;
    fileDisplay2->isIR2 = true;
    addChild(fileDisplay2);

    addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(5.08, 38.0)), module,
                                                        OpcVcvIr::DYNAMIC_MODE_LIGHT));
    addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(35.56, 38.0)), module,
                                                         OpcVcvIr::SIDECHAIN_LIGHT));

    addParam(
        createParamCentered<RoundBlackKnob>(mm2px(Vec(10.16, 50.0)), module, OpcVcvIr::GAIN_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48, 50.0)), module,
                                                 OpcVcvIr::BLEND_PARAM));

    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16, 65.0)), module,
                                                      OpcVcvIr::LOW_BLEND_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(30.48, 65.0)), module,
                                                      OpcVcvIr::HIGH_BLEND_PARAM));

    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16, 78.0)), module,
                                                      OpcVcvIr::LOW_THRESHOLD_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(30.48, 78.0)), module,
                                                      OpcVcvIr::HIGH_THRESHOLD_PARAM));

    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16, 91.0)), module,
                                                      OpcVcvIr::ATTACK_TIME_PARAM));
    addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(30.48, 91.0)), module,
                                                      OpcVcvIr::RELEASE_TIME_PARAM));

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 103.0)), module,
                                                 OpcVcvIr::OUTPUT_GAIN_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.62, 115.0)), module, OpcVcvIr::INPUT_L));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.24, 115.0)), module, OpcVcvIr::INPUT_R));
    addInput(
        createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 115.0)), module, OpcVcvIr::SIDECHAIN_L));
    addInput(
        createInputCentered<PJ301MPort>(mm2px(Vec(33.02, 115.0)), module, OpcVcvIr::SIDECHAIN_R));

    addOutput(
        createOutputCentered<PJ301MPort>(mm2px(Vec(11.43, 125.0)), module, OpcVcvIr::OUTPUT_L));
    addOutput(
        createOutputCentered<PJ301MPort>(mm2px(Vec(29.21, 125.0)), module, OpcVcvIr::OUTPUT_R));
  }

  void appendContextMenu(Menu* menu) override {
    OpcVcvIr* module = dynamic_cast<OpcVcvIr*>(this->module);
    if (!module) {
      return;
    }

    menu->addChild(new MenuSeparator);

    menu->addChild(createBoolPtrMenuItem("Dynamic Mode", "", &module->dynamicModeEnabled_));
    menu->addChild(createBoolPtrMenuItem("Sidechain Enable", "", &module->sidechainEnabled_));
  }
};

Model* model_opc_vcv_ir = createModel<OpcVcvIr, OpcVcvIrWidget>("opc-vcv-ir");