#ifndef SAMPLEBUFFER_HPP_
#define SAMPLEBUFFER_HPP_

#include <array>
#include <cstdint>

using Sample = uint16_t;

// gives 8000
constexpr unsigned int MAX_SAMPLE_BUFFER_BYTESIZE = 16384;
constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = MAX_SAMPLE_BUFFER_BYTESIZE / sizeof(Sample);

class SampleBuffer
{
public:
    SampleBuffer(Sample *buffer);

    void clear();

    void modify(Sample *data, unsigned int srcsize);
    void midmodify(Sample *data, unsigned int srcsize);
    Sample *modified();

    Sample *data();
    Sample *middata();
    uint8_t *bytedata();

    void setSize(unsigned int size);
    unsigned int size() const;
    unsigned int bytesize() const;

private:
    Sample *m_buffer = nullptr;
    unsigned int m_size = MAX_SAMPLE_BUFFER_SIZE;
    Sample *m_modified = nullptr;
};

#endif // SAMPLEBUFFER_HPP_
