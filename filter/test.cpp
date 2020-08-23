#include <cstdint>

using adcsample_t = uint16_t;
__attribute__((section(".process_data")))
void process_data(adcsample_t *samples, unsigned int size);
 
////////////////////////////////////////////////////////////////////////////////

void process_data(adcsample_t *samples, unsigned int size)
{
    for (unsigned int i = 0; i < size; i++)
        samples[i] /= 2;
}

