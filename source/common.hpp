#include <array>
#include <cstdint>

//#define ENABLE_SIGGEN

constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = 4000;

using Sample = uint16_t;

class SampleBuffer
{
public:
    SampleBuffer(Sample *buffer) :
        m_buffer(buffer) {}

    void clear() {
        /*static const Sample ref[21] = {
            100, 200, 400, 600, 800, 1000, 1200, 1400, 1600, 1800,
            2000, 2200, 2400, 2600, 2800, 3000, 3200, 3400, 3600, 3800, 4095
        };
        for (unsigned int i = 0; i < m_size; i++)
            m_buffer[i] = ref[i % 21];*/
        std::fill(m_buffer, m_buffer + m_size, 2048);
    }
    void modify(Sample *data, unsigned int srcsize) {
        auto size = srcsize < m_size ? srcsize : m_size;
        std::copy(data, data + size, m_buffer);
        m_modified = m_buffer;
    }
    void midmodify(Sample *data, unsigned int srcsize) {
        auto size = srcsize < m_size / 2 ? srcsize : m_size / 2;
        std::copy(data, data + size, middata());
        m_modified = middata();
    }

    void setSize(unsigned int size) {
        m_size = size < MAX_SAMPLE_BUFFER_SIZE ? size : MAX_SAMPLE_BUFFER_SIZE;
    }

    Sample *data() {
        return m_buffer;
    }
    Sample *middata() {
        return m_buffer + m_size / 2;
    }
    uint8_t *bytedata() {
        return reinterpret_cast<uint8_t *>(m_buffer);
    }

    Sample *modified() {
        auto m = m_modified;
        m_modified = nullptr;
        return m;
    }
    unsigned int size() const {
        return m_size;
    }
    unsigned int bytesize() const {
        return m_size * sizeof(Sample);
    }

private:
    //std::array<Sample, MAX_SAMPLE_BUFFER_SIZE> m_buffer;
    Sample *m_buffer = nullptr;
    unsigned int m_size = MAX_SAMPLE_BUFFER_SIZE;
    Sample *m_modified = nullptr;
};

