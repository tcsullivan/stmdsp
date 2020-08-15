#include <cstdint>

using adcsample_t = uint16_t;
__attribute__((section(".process_data")))
void process_data(adcsample_t *samples, unsigned int size);
 
////////////////////////////////////////////////////////////////////////////////

void process_data(adcsample_t *samples, unsigned int size)
{
    size--;
    for (unsigned int i = 0; i < size; i++)
        samples[i] = samples[i] * 80 / 100  + samples[i + 1] * 20 / 100;
}

