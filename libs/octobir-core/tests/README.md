# OctobIR Core Library Tests

Unit tests for the `octobir-core` library. These tests focus on testing OUR logic, not third-party libraries (WDL, dr_wav, etc.).

## Running Tests

```bash
# From repository root
make test
```

## Test Coverage

### IRProcessorTests.cpp
Tests for IRProcessor public interface and behavior:
- **Parameter Clamping**: Verifies all parameters (blend, thresholds, attack/release times, output gain) are clamped to their valid ranges
- **Output Gain**: Verifies dB to linear conversion and gain application
- **State Management**: Verifies initial state and dynamic mode state transitions
- **Passthrough**: Verifies audio passes through unchanged when no IRs are loaded

### IRProcessorLogicTests.cpp
Tests for IRProcessor gain calculation logic:
- **Blend Gain Calculation**: Verifies equal-power crossfade math (sqrt-based)
- **Equal Power Verification**: Ensures power sum equals 1.0 across blend range

### IRLoaderTests.cpp
Tests for IRLoader logic:
- **Initial State**: Verifies clean initial state
- **Error Handling**: Verifies proper error handling for invalid files
- **Compensation Gain**: Verifies the -17dB compensation gain calculation

## What's NOT Tested

Following best practices, we do NOT test:
- **WDL ConvolutionEngine**: Third-party library, assumed correct
- **WDL Resampler**: Third-party library, assumed correct
- **dr_wav file parsing**: Third-party library, assumed correct
- **FFT correctness**: Third-party library, assumed correct

## Adding New Tests

When adding new logic to the core library:

1. Test parameter validation/clamping
2. Test mathematical calculations (gain, blend, etc.)
3. Test state transitions
4. Test error handling paths

Do NOT test implementation details (private methods). Test PUBLIC behavior only.
