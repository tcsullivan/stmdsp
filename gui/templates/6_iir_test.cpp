adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
	constexpr float alpha = 0.7;

	static adcsample_t prev = 2048;

	samples[0] = (1 - alpha) * samples[0] + alpha * prev;
	for (unsigned int i = 1; i < size; i++)
		samples[i] = (1 - alpha) * samples[i] + alpha * samples[i - 1];
	prev = samples[size - 1];

	return samples;
}
