#include "plugin.hpp"
#include "convolution.h"
#include <string>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"


struct Opc_vcv_ir final : Module {
	enum ParamId {
		GAIN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		INPUT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	std::vector<float> irSamples;
	uint32_t irSampleRate = 0;
	size_t irLength = 0;
	bool irLoaded = false;
	
	Convolution convolver;

	Opc_vcv_ir() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// We want to snap this to the closest 0.1dB. The easiest way to do this is multiply the internal value by 10 to
		// leverage VCV's snapEnabled parameter. When we fetch this value later, we will need to divide it by 10.
		configParam(GAIN_PARAM, -200.f, 200.f, 0.f, "Gain", " dB", 0.f, 0.1f);
		paramQuantities[GAIN_PARAM]->snapEnabled = true;
		configInput(INPUT_INPUT, "Audio Input");
		configOutput(OUTPUT_OUTPUT, "Audio Output");
		
		// Load IR file from home directory on startup
		std::string homeDir = getenv("HOME") ? getenv("HOME") : "";
		if (!homeDir.empty()) {
			loadIR(homeDir + "/ir_sample.wav");
		}
		else {
			WARN("Could not find home directory!");
		}
	}

	void loadIR(const std::string& filePath) {
		uint32_t channels;
		drwav_uint64 totalPCMFrameCount;
		
		float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(filePath.c_str(), &channels, &irSampleRate, &totalPCMFrameCount, NULL);
		
		if (pSampleData == nullptr) {
			WARN("Failed to load IR file: %s", filePath.c_str());
			irLoaded = false;
			return;
		}
		
		irLength = static_cast<size_t>(totalPCMFrameCount);
		irSamples.clear();
		irSamples.resize(irLength);
		
		if (channels == 1) {
			// Mono - copy directly
			for (size_t i = 0; i < irLength; i++) {
				irSamples[i] = pSampleData[i];
			}
		} else if (channels == 2) {
			// Stereo - mix to mono
			for (size_t i = 0; i < irLength; i++) {
				irSamples[i] = (pSampleData[i * 2] + pSampleData[i * 2 + 1]) * 0.5f;
			}
		} else {
			// Multi-channel - take first channel
			for (size_t i = 0; i < irLength; i++) {
				irSamples[i] = pSampleData[i * channels];
			}
		}
		
		drwav_free(pSampleData, NULL);
		
		// TODO: Investigate a better gain normalization method
		// I tried an RMS based normalization, but that did not work very well across different IRs.
		// For now, just going to reduce the gain by this value that seems to work moderately well for different IRs
		float irCompensationGain = dsp::dbToAmplitude(-17.0f);
		for (size_t i = 0; i < irLength; i++) {
			irSamples[i] *= irCompensationGain;
		}
		
		// Initialize convolution with the loaded IR data
		if (convolver.initialize(irSamples.data(), irLength)) {
			irLoaded = true;
			INFO("Loaded IR file: %s (%zu samples, %u Hz)", filePath.c_str(), irLength, irSampleRate);
		} else {
			irLoaded = false;
			WARN("Failed to initialize convolver with IR data");
		}
	}

	void process(const ProcessArgs& args) override {
		// Always set output to prevent floating values
		float output = 0.0f;
		
		if (inputs[INPUT_INPUT].isConnected() && outputs[OUTPUT_OUTPUT].isConnected()) {
			// Get parameter values in dB and remember to divide it by 10! See discussion near the top of this file
			// for more details on why we do this
			float gainDb = params[GAIN_PARAM].getValue() * 0.1f;

			// Convert dB to linear amplitude using VCV Rack utility function
			float gain = dsp::dbToAmplitude(gainDb);

			// Get input sample
			float input = inputs[INPUT_INPUT].getVoltage();
			
			// Apply input gain
			float processed = input * gain;

			// Apply convolution if IR is loaded
			if (irLoaded) {
				processed = convolver.process(processed);
			}
			
			// Set the processed result as output
			output = processed;
		}
		
		outputs[OUTPUT_OUTPUT].setVoltage(output);
	}
};


struct Opc_vcv_irWidget final : ModuleWidget {
	explicit Opc_vcv_irWidget(Opc_vcv_ir* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/opc-vcv-ir-panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 40.64)), module, Opc_vcv_ir::GAIN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 111.76)), module, Opc_vcv_ir::INPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 111.76)), module, Opc_vcv_ir::OUTPUT_OUTPUT));

		// mm2px(Vec(30.48, 20.32))
		addChild(createWidget<Widget>(mm2px(Vec(5.08, 7.62))));
	}
};

Model* modelOpc_vcv_ir = createModel<Opc_vcv_ir, Opc_vcv_irWidget>("opc-vcv-ir");