/**
 * 5_iir_differentiator.cpp
 * Written by Clyne Sullivan.
 *
 * Does an IIR differentiation on the incoming signal, so that the output is representative of the
 * rate of change of the input.
 * A scaling factor is applied so that the output's form is more clearly visible.
 */

adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
    constexpr int scaling_factor = 4;
    static adcsample_t prev = 2048;

    // Compute the first output value using the saved sample.
    samples[0] = 2048 + ((samples[0] - prev) * scaling_factor;
    // Save the last sample for the next iteration.
    prev = samples[size - 1];

	for (unsigned int i = 1; i < size; i++) {
        // Take the rate of change and scale it.
        // 2048 is added as the output should be centered in the voltage range.
		samples[i] = 2048 + ((samples[i] - samples[i - 1]) * scaling_factor);
    }

    return samples;
}

