adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
	constexpr float alpha = 0.75;
	constexpr unsigned int D = 100;

	static adcsample_t output[SIZE];
	static adcsample_t prev[D]; // prev[0] = output[0 - D]

	// Do calculations with previous output
	for (unsigned int i = 0; i < D; i++)
		output[i] = samples[i] + alpha * (prev[i] - 2048);

	// Do calculations with current samples
	for (unsigned int i = D; i < size; i++)
		output[i] = samples[i] + alpha * (output[i - D] - 2048);

	// Save outputs for next computation
	for (unsigned int i = 0; i < D; i++)
		prev[i] = output[size - (D - i)];

	return output;
}
