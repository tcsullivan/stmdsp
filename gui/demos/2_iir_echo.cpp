Sample *process_data(Samples samples)
{
	constexpr unsigned int size = samples.size();
	constexpr unsigned int D = 2000;

	float alpha = readalt() / 4095.;

	static Sample output[size];
	static Sample prev[D]; // prev[0] = output[0 - D]

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
