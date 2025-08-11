#include "convoengine.h"
#include "plugin.hpp"
#include <dsp/resampler.hpp>
#include <string>
#include <vector>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

struct Opc_vcv_ir final : Module {
    enum ParamId { GAIN_PARAM, PARAMS_LEN };
    enum InputId { INPUT_INPUT, INPUTS_LEN };
    enum OutputId { OUTPUT_OUTPUT, OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    std::vector<float> irSamples;
    std::vector<float> resampledIR;
    uint32_t irSampleRate = 0;
    uint32_t lastSystemSampleRate = 0;
    size_t irLength = 0;
    bool irLoaded = false;
    bool needsResampling = false;

    // Sample rate check optimization
    int sampleRateCheckCounter = 0;
    static constexpr int SAMPLE_RATE_CHECK_INTERVAL = 4096; // Check every ~93ms at 44.1kHz

    WDL_ConvolutionEngine convolver;
    WDL_ImpulseBuffer impulseBuffer;
    dsp::SampleRateConverter<1> irResampler;

    Opc_vcv_ir() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        // We want to snap this to the closest 0.1dB. The easiest way to do this is multiply the
        // internal value by 10 to leverage VCV's snapEnabled parameter. When we fetch this value
        // later, we will need to divide it by 10.
        configParam(GAIN_PARAM, -200.f, 200.f, 0.f, "Gain", " dB", 0.f, 0.1f);
        paramQuantities[GAIN_PARAM]->snapEnabled = true;
        configInput(INPUT_INPUT, "Audio Input");
        configOutput(OUTPUT_OUTPUT, "Audio Output");

        lastSystemSampleRate = static_cast<uint32_t>(APP->engine->getSampleRate());

        // Load IR file from home directory on startup
        std::string homeDir = getenv("HOME") ? getenv("HOME") : "";
        std::string irPath = homeDir + "/ir_sample.wav";
        loadIR(irPath);
    }

    void loadIR(const std::string &filePath) {
        uint32_t channels;
        drwav_uint64 totalPCMFrameCount;

        float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
            filePath.c_str(), &channels, &irSampleRate, &totalPCMFrameCount, nullptr);

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

        drwav_free(pSampleData, nullptr);

        // TODO: Investigate a better gain normalization method
        // I tried an RMS based normalization, but that did not work very well across different IRs.
        // For now, just going to reduce the gain by this value that seems to work moderately well
        // for different IRs. NAM does something similar, and if it's good enough for Steven
        // Atkinson it's good enough for me.
        float irCompensationGain = dsp::dbToAmplitude(-17.0f);
        for (size_t i = 0; i < irLength; i++) {
            irSamples[i] *= irCompensationGain;
        }

        // Perform resampling and convolver initialization
        resampleAndInitialize();
    }

    void resampleAndInitialize() {
        if (irSamples.empty()) {
            irLoaded = false;
            return;
        }

        // Check if sample rate conversion is needed
        auto systemSampleRate = static_cast<uint32_t>(APP->engine->getSampleRate());
        needsResampling = (irSampleRate != systemSampleRate);
        lastSystemSampleRate = systemSampleRate;

        if (needsResampling) {
            INFO("IR sample rate (%u Hz) differs from system (%u Hz), resampling...", irSampleRate,
                 systemSampleRate);

            // Configure resampler
            irResampler.setChannels(1);
            irResampler.setQuality(8); // High quality resampling
            irResampler.setRates(irSampleRate, systemSampleRate);

            // Calculate output length with proper rounding
            size_t outFrames = (irLength * systemSampleRate + irSampleRate / 2) / irSampleRate;

            // Allocate output buffer with extra padding for safety
            resampledIR.resize(outFrames + 64);

            // Perform resampling
            int inFrames = static_cast<int>(irLength);
            int actualOutFrames = static_cast<int>(outFrames);
            irResampler.process(irSamples.data(), 1, &inFrames, resampledIR.data(), 1,
                                &actualOutFrames);

            resampledIR.resize(actualOutFrames);

            int actualLength = impulseBuffer.SetLength(actualOutFrames);
            if (actualLength <= 0) {
                irLoaded = false;
                WARN("Failed to set impulse buffer length: requested %d, got %d", actualOutFrames,
                     actualLength);
                return;
            }

            impulseBuffer.SetNumChannels(1);
            if (impulseBuffer.GetNumChannels() != 1) {
                irLoaded = false;
                WARN("Failed to set impulse buffer channels to 1, got %d",
                     impulseBuffer.GetNumChannels());
                return;
            }

            impulseBuffer.samplerate = systemSampleRate;

            WDL_FFT_REAL *irBuffer = impulseBuffer.impulses[0].Get();
            if (!irBuffer) {
                irLoaded = false;
                WARN("Failed to get impulse buffer data pointer");
                return;
            }

            for (int i = 0; i < actualLength; i++) {
                irBuffer[i] = (i < actualOutFrames) ? resampledIR[i] : 0.0f;
            }

            int result = convolver.SetImpulse(&impulseBuffer);
            if (result > 0) {
                irLoaded = true;
                INFO("Loaded and resampled IR file: (%zu→%d samples, %u→%u Hz, latency: %d)",
                     irLength, actualLength, irSampleRate, systemSampleRate, result);
            } else {
                irLoaded = false;
                WARN("Failed to initialize WDL convolver with resampled IR data, error code: %d",
                     result);
            }
        } else {
            int actualLength = impulseBuffer.SetLength(static_cast<int>(irLength));
            if (actualLength <= 0) {
                irLoaded = false;
                WARN("Failed to set impulse buffer length: requested %zu, got %d", irLength,
                     actualLength);
                return;
            }

            impulseBuffer.SetNumChannels(1);
            if (impulseBuffer.GetNumChannels() != 1) {
                irLoaded = false;
                WARN("Failed to set impulse buffer channels to 1, got %d",
                     impulseBuffer.GetNumChannels());
                return;
            }

            impulseBuffer.samplerate = systemSampleRate;

            // Copy original IR data to WDL impulse buffer
            WDL_FFT_REAL *irBuffer = impulseBuffer.impulses[0].Get();
            if (!irBuffer) {
                irLoaded = false;
                WARN("Failed to get impulse buffer data pointer");
                return;
            }

            for (int i = 0; i < actualLength; i++) {
                irBuffer[i] = (i < static_cast<int>(irLength)) ? irSamples[i] : 0.0f;
            }

            int result = convolver.SetImpulse(&impulseBuffer);
            if (result > 0) {
                irLoaded = true;
                INFO("Using original IR: (%d samples, %u Hz, latency: %d)", actualLength,
                     irSampleRate, result);
            } else {
                irLoaded = false;
                WARN("Failed to initialize WDL convolver with IR data, error code: %d", result);
            }
        }
    }

    void process(const ProcessArgs &args) override {
        // Only check sample rate occasionally to reduce CPU overhead
        if (++sampleRateCheckCounter >= SAMPLE_RATE_CHECK_INTERVAL) {
            sampleRateCheckCounter = 0;

            if (irSampleRate > 0 && !irSamples.empty()) {
                auto currentSystemSampleRate = static_cast<uint32_t>(args.sampleRate);
                if (currentSystemSampleRate != lastSystemSampleRate) {
                    INFO("System sample rate changed from %u to %u Hz, reprocessing IR...",
                         lastSystemSampleRate, currentSystemSampleRate);
                    resampleAndInitialize();
                }
            }
        }
        // Always set output to prevent floating values
        float output = 0.0f;

        if (inputs[INPUT_INPUT].isConnected() && outputs[OUTPUT_OUTPUT].isConnected()) {
            // Get parameter values in dB and remember to divide it by 10! See discussion near the
            // top of this file for more details on why we do this
            float gainDb = params[GAIN_PARAM].getValue() * 0.1f;

            float gain = dsp::dbToAmplitude(gainDb);

            float input = inputs[INPUT_INPUT].getVoltage();

            float processed = input * gain;

            if (irLoaded) {
                WDL_FFT_REAL *inputPtr = &processed;
                convolver.Add(&inputPtr, 1, 1);

                if (convolver.Avail(1) >= 1) {
                    WDL_FFT_REAL **outputPtr = convolver.Get();
                    processed = outputPtr[0][0];
                    convolver.Advance(1);
                } else {
                    processed = 0.0f;
                }
            }

            output = processed;
        }

        outputs[OUTPUT_OUTPUT].setVoltage(output);
    }
};

struct Opc_vcv_irWidget final : ModuleWidget {
    explicit Opc_vcv_irWidget(Opc_vcv_ir *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/opc-vcv-ir-panel.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(
            createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(
            Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 40.64)), module,
                                                     Opc_vcv_ir::GAIN_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16, 111.76)), module,
                                                 Opc_vcv_ir::INPUT_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.48, 111.76)), module,
                                                   Opc_vcv_ir::OUTPUT_OUTPUT));

        // mm2px(Vec(30.48, 20.32))
        addChild(createWidget<Widget>(mm2px(Vec(5.08, 7.62))));
    }
};

Model *modelOpc_vcv_ir = createModel<Opc_vcv_ir, Opc_vcv_irWidget>("opc-vcv-ir");