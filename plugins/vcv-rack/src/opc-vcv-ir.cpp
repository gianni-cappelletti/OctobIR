#include <osdialog.h>

#include <octobir-core/IRProcessor.hpp>
#include <string>

#include "plugin.hpp"

struct OpcVcvIr final : Module {
  enum ParamId { GAIN_PARAM, PARAMS_LEN };
  enum InputId { INPUT_L, INPUT_R, INPUTS_LEN };
  enum OutputId { OUTPUT_L, OUTPUT_R, OUTPUTS_LEN };
  enum LightId { LIGHTS_LEN };

  octob::IRProcessor irProcessor_;
  std::string loaded_file_path_;

  int sample_rate_check_counter_ = 0;
  static constexpr int kSampleRateCheckInterval = 4096;
  uint32_t last_system_sample_rate_ = 0;

  OpcVcvIr() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    configParam(GAIN_PARAM, -200.f, 200.f, 0.f, "Gain", " dB", 0.f, 0.1f);
    paramQuantities[GAIN_PARAM]->snapEnabled = true;
    configInput(INPUT_L, "Audio Input L");
    configInput(INPUT_R, "Audio Input R");
    configOutput(OUTPUT_L, "Audio Output L");
    configOutput(OUTPUT_R, "Audio Output R");

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
      INFO("Loaded IR: %s (%zu samples, %.0f Hz)", file_path.c_str(),
           irProcessor_.getIRNumSamples(), irProcessor_.getIRSampleRate());
    } else {
      WARN("Failed to load IR file %s: %s", file_path.c_str(), error.c_str());
      loaded_file_path_.clear();
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

    bool leftInputConnected = inputs[INPUT_L].isConnected();
    bool rightInputConnected = inputs[INPUT_R].isConnected();

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

  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "filePath", json_string(loaded_file_path_.c_str()));
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
  }
};

struct IrFileDisplay : app::LedDisplayChoice {
  OpcVcvIr* module;

  IrFileDisplay() : module(nullptr) {
    fontPath = asset::plugin(plugin_instance, "res/font/Inconsolata_Condensed-Bold.ttf");
    color = nvgRGB(0x12, 0x12, 0x12);
    // bgColor = nvgRGB(0xe0, 0x9e, 0x6e);
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
      module->LoadIR(std::string(path));
      updateDisplayText();
      std::free(path);
    }

    osdialog_filters_free(filters);
  }

  void updateDisplayText() {
    if (!module || module->loaded_file_path_.empty()) {
      text = "<No IR selected>";
    } else {
      size_t pos = module->loaded_file_path_.find_last_of("/\\");
      std::string filename = (pos != std::string::npos) ? module->loaded_file_path_.substr(pos + 1)
                                                        : module->loaded_file_path_;

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

    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 40.64)), module,
                                                 OpcVcvIr::GAIN_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 105.0)), module, OpcVcvIr::INPUT_L));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 118.0)), module, OpcVcvIr::INPUT_R));

    addOutput(
        createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 105.0)), module, OpcVcvIr::OUTPUT_L));
    addOutput(
        createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 118.0)), module, OpcVcvIr::OUTPUT_R));

    auto* fileDisplay = createWidget<IrFileDisplay>(mm2px(Vec(5.08, 12.0)));
    fileDisplay->box.size = mm2px(Vec(30.48, 17.0));
    fileDisplay->module = module;
    addChild(fileDisplay);
  }
};

Model* model_opc_vcv_ir = createModel<OpcVcvIr, OpcVcvIrWidget>("opc-vcv-ir");