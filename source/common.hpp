#include <array>
#include <cstdint>

constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = 6000;

using Sample = uint16_t;

class SampleBuffer
{
public:
    void clear() {
        m_buffer.fill(0);
    }
    void modify(Sample *data, unsigned int srcsize) {
        auto size = srcsize < m_size ? srcsize : m_size;
        std::copy(data, data + size, m_buffer.data());
        m_modified = m_buffer.data();
    }
    void midmodify(Sample *data, unsigned int srcsize) {
        auto size = srcsize < m_size / 2 ? srcsize : m_size / 2;
        std::copy(data, data + size, middata());
        m_modified = middata();
    }

    void setSize(unsigned int size) {
        m_size = size < MAX_SAMPLE_BUFFER_SIZE ? size : MAX_SAMPLE_BUFFER_SIZE;
    }

    Sample *data() /*const*/ {
        return m_buffer.data();
    }
    Sample *middata() /*const*/ {
        return m_buffer.data() + m_size / 2;
    }
    uint8_t *bytedata() /*const*/ {
        return reinterpret_cast<uint8_t *>(m_buffer.data());
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
    std::array<Sample, MAX_SAMPLE_BUFFER_SIZE> m_buffer;
    unsigned int m_size = MAX_SAMPLE_BUFFER_SIZE;
    Sample *m_modified = nullptr;
};

