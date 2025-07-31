#include "plugin.hpp"

struct Opc_vcv_ir : Module {
	enum ParamId {
		INPUT_GAIN_PARAM,
		OUTPUT_GAIN_PARAM,
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

	Opc_vcv_ir() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(INPUT_GAIN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(OUTPUT_GAIN_PARAM, 0.f, 1.f, 0.f, "");
		configInput(INPUT_INPUT, "");
		configOutput(OUTPUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct Opc_vcv_irWidget : ModuleWidget {
	Opc_vcv_irWidget(Opc_vcv_ir* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/opc-vcv-ir.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 40.64)), module, Opc_vcv_ir::INPUT_GAIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 63.698)), module, Opc_vcv_ir::OUTPUT_GAIN_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 111.76)), module, Opc_vcv_ir::INPUT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 111.76)), module, Opc_vcv_ir::OUTPUT_OUTPUT));

		// mm2px(Vec(30.48, 20.32))
		addChild(createWidget<Widget>(mm2px(Vec(5.08, 7.62))));
	}
};

Model* modelOpc_vcv_ir = createModel<Opc_vcv_ir, Opc_vcv_irWidget>("opc-vcv-ir");