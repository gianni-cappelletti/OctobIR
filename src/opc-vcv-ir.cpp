#include <string>
#include <vector>

#include "convoengine.h"
#include "plugin.hpp"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

struct OpcVcvIr final : Module {
    enum ParamId { GAIN_PARAM, PARAMS_LEN };
    enum InputId { INPUT_INPUT, INPUTS_LEN };
    enum OutputId { OUTPUT_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    std::vector<float> ir_samples_;
    std::vector<float> resampled_ir_;
    uint32_t ir_sample_rate_ = 0;
    uint32_t last_system_sample_rate_ = 0;
    size_t ir_length_ = 0;
    bool ir_loaded_ = false;
    bool needs_resampling_ = false;

    int sample_rate_check_counter_ = 0;
    static constexpr int kSampleRateCheckInterval = 4096;

    WDL_ConvolutionEngine convolver_;
    WDL_ImpulseBuffer impulse_buffer_;
    dsp::SampleRateConverter<1> ir_resampler_;

    OpcVcvIr() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        // We want to snap this to the closest 0.1dB. The easiest way to do this is multiply the
        // internal value by 10 to leverage VCV's snapEnabled parameter. When we fetch this value
        // later, we will need to divide it by 10.
        configParam(GAIN_PARAM, -200.f, 200.f, 0.f, "Gain", " dB", 0.f, 0.1f);
        paramQuantities[GAIN_PARAM]->snapEnabled = true;
        configInput(INPUT_INPUT, "Audio Input");
        configOutput(OUTPUT_OUTPUT, "Audio Output");

        last_system_sample_rate_ = static_cast<uint32_t>(APP->engine->getSampleRate());

        std::string home_dir = getenv("HOME") ? getenv("HOME") : "";
        std::string ir_path = home_dir + "/ir_sample.wav";
        LoadIR(ir_path);
    }

    void LoadIR(const std::string &file_path) {
        uint32_t channels;
        drwav_uint64 totalPCMFrameCount;

        float *sample_data = drwav_open_file_and_read_pcm_frames_f32(
            file_path.c_str(), &channels, &ir_sample_rate_, &totalPCMFrameCount, nullptr);

        if (sample_data == nullptr) {
            WARN("Failed to load IR file: %s", file_path.c_str());
            ir_loaded_ = false;
            return;
        }

        ir_length_ = static_cast<size_t>(totalPCMFrameCount);
        ir_samples_.clear();
        ir_samples_.resize(ir_length_);

        if (channels == 1) {
            for (size_t i = 0; i < ir_length_; i++) {
                ir_samples_[i] = sample_data[i];
            }
        } else if (channels == 2) {
            for (size_t i = 0; i < ir_length_; i++) {
                ir_samples_[i] = (sample_data[i * 2] + sample_data[i * 2 + 1]) * 0.5f;
            }
        } else {
            for (size_t i = 0; i < ir_length_; i++) {
                ir_samples_[i] = sample_data[i * channels];
            }
        }

        drwav_free(sample_data, nullptr);

        float ir_compensation_gain = dsp::dbToAmplitude(-17.0f);
        for (size_t i = 0; i < ir_length_; i++) {
            ir_samples_[i] *= ir_compensation_gain;
        }

        ResampleAndInitialize();
    }

    void ResampleAndInitialize() {
        if (ir_samples_.empty()) {
            ir_loaded_ = false;
            return;
        }

        auto system_sample_rate = static_cast<uint32_t>(APP->engine->getSampleRate());
        needs_resampling_ = (ir_sample_rate_ != system_sample_rate);
        last_system_sample_rate_ = system_sample_rate;

        if (needs_resampling_) {
            INFO("IR sample rate (%u Hz) differs from system (%u Hz), resampling...",
                 ir_sample_rate_, system_sample_rate);

            ir_resampler_.setChannels(1);
            ir_resampler_.setQuality(8);
            ir_resampler_.setRates(ir_sample_rate_, system_sample_rate);

            size_t out_frames =
                (ir_length_ * system_sample_rate + ir_sample_rate_ / 2) / ir_sample_rate_;
            resampled_ir_.resize(out_frames + 64);

            int in_frames = static_cast<int>(ir_length_);
            int actual_out_frames = static_cast<int>(out_frames);
            ir_resampler_.process(ir_samples_.data(), 1, &in_frames, resampled_ir_.data(), 1,
                                  &actual_out_frames);

            resampled_ir_.resize(actual_out_frames);

            int actual_length = impulse_buffer_.SetLength(actual_out_frames);
            if (actual_length <= 0) {
                ir_loaded_ = false;
                WARN("Failed to set impulse buffer length: requested %d, got %d", actual_out_frames,
                     actual_length);
                return;
            }

            impulse_buffer_.SetNumChannels(1);
            if (impulse_buffer_.GetNumChannels() != 1) {
                ir_loaded_ = false;
                WARN("Failed to set impulse buffer channels to 1, got %d",
                     impulse_buffer_.GetNumChannels());
                return;
            }

            impulse_buffer_.samplerate = system_sample_rate;

            WDL_FFT_REAL *ir_buffer = impulse_buffer_.impulses[0].Get();
            if (!ir_buffer) {
                ir_loaded_ = false;
                WARN("Failed to get impulse buffer data pointer");
                return;
            }

            for (int i = 0; i < actual_length; i++) {
                ir_buffer[i] = (i < actual_out_frames) ? resampled_ir_[i] : 0.0f;
            }

            int result = convolver_.SetImpulse(&impulse_buffer_);
            if (result > 0) {
                ir_loaded_ = true;
                INFO("Loaded and resampled IR file: (%zu→%d samples, %u→%u Hz, latency: %d)",
                     ir_length_, actual_length, ir_sample_rate_, system_sample_rate, result);
            } else {
                ir_loaded_ = false;
                WARN("Failed to initialize WDL convolver with resampled IR data, error code: %d",
                     result);
            }
        } else {
            int actual_length = impulse_buffer_.SetLength(static_cast<int>(ir_length_));
            if (actual_length <= 0) {
                ir_loaded_ = false;
                WARN("Failed to set impulse buffer length: requested %zu, got %d", ir_length_,
                     actual_length);
                return;
            }

            impulse_buffer_.SetNumChannels(1);
            if (impulse_buffer_.GetNumChannels() != 1) {
                ir_loaded_ = false;
                WARN("Failed to set impulse buffer channels to 1, got %d",
                     impulse_buffer_.GetNumChannels());
                return;
            }

            impulse_buffer_.samplerate = system_sample_rate;

            WDL_FFT_REAL *ir_buffer = impulse_buffer_.impulses[0].Get();
            if (!ir_buffer) {
                ir_loaded_ = false;
                WARN("Failed to get impulse buffer data pointer");
                return;
            }

            for (int i = 0; i < actual_length; i++) {
                ir_buffer[i] = (i < static_cast<int>(ir_length_)) ? ir_samples_[i] : 0.0f;
            }

            int result = convolver_.SetImpulse(&impulse_buffer_);
            if (result > 0) {
                ir_loaded_ = true;
                INFO("Using original IR: (%d samples, %u Hz, latency: %d)", actual_length,
                     ir_sample_rate_, result);
            } else {
                ir_loaded_ = false;
                WARN("Failed to initialize WDL convolver with IR data, error code: %d", result);
            }
        }
    }

    void process(const ProcessArgs &args) override {
        if (++sample_rate_check_counter_ >= kSampleRateCheckInterval) {
            sample_rate_check_counter_ = 0;

            if (ir_sample_rate_ > 0 && !ir_samples_.empty()) {
                auto current_system_sample_rate = static_cast<uint32_t>(args.sampleRate);
                if (current_system_sample_rate != last_system_sample_rate_) {
                    INFO("System sample rate changed from %u to %u Hz, reprocessing IR...",
                         last_system_sample_rate_, current_system_sample_rate);
                    ResampleAndInitialize();
                }
            }
        }
        float output = 0.0f;

        if (inputs[INPUT_INPUT].isConnected() && outputs[OUTPUT_OUTPUT].isConnected()) {
            // Get parameter values in dB and remember to divide it by 10! See discussion near the
            // constructor for more details on why we do this.
            float gain_db = params[GAIN_PARAM].getValue() * 0.1f;
            float gain = dsp::dbToAmplitude(gain_db);
            float input = inputs[INPUT_INPUT].getVoltage();
            float processed = input * gain;

            if (ir_loaded_) {
                WDL_FFT_REAL *input_ptr = &processed;
                convolver_.Add(&input_ptr, 1, 1);

                if (convolver_.Avail(1) >= 1) {
                    WDL_FFT_REAL **output_ptr = convolver_.Get();
                    processed = output_ptr[0][0];
                    convolver_.Advance(1);
                } else {
                    processed = 0.0f;
                }
            }

            output = processed;
        }

        outputs[OUTPUT_OUTPUT].setVoltage(output);
    }
};

struct OpcVcvIrWidget final : ModuleWidget {
    explicit OpcVcvIrWidget(OpcVcvIr *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(plugin_instance, "res/opc-vcv-ir-panel.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(
            createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(
            Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 40.64)), module,
                                                     OpcVcvIr::GAIN_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 111.76)), module,
                                                 OpcVcvIr::INPUT_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 111.76)), module,
                                                   OpcVcvIr::OUTPUT_OUTPUT));

        addChild(createWidget<Widget>(mm2px(Vec(5.08, 7.62))));
    }
};

Model *model_opc_vcv_ir = createModel<OpcVcvIr, OpcVcvIrWidget>("opc-vcv-ir");