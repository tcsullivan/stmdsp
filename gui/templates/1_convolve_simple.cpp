/**
 * 1_convolve_simple.cpp
 * Written by Clyne Sullivan.
 *
 * Computes a convolution in the simplest way possible. While the code is brief, it lacks many
 * possible optimizations. The convolution's result will not fill the output buffer either, as the
 * transient response is not calculated.
 */

adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
    // Define our output buffer. SIZE is the largest size of the 'samples' buffer.
    static adcsample_t buffer[SIZE];

    // Define our filter
    constexpr unsigned int filter_size = 3;
	float filter[filter_size] = {
        0.3333, 0.3333, 0.3333
    };

    // Begin convolving:
    for (int n = 0; n < size - (filter_size - 1); n++) {
        buffer[n] = 0;
        for (int k = 0; k < filter_size; k++)
            buffer[n] += samples[n + k] * filter[k];
    }

    return buffer;
}
