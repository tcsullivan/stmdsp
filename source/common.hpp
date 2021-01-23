#include <array>
#include <cstdint>

using Sample = uint16_t;

// gives 8000
constexpr unsigned int MAX_SAMPLE_BUFFER_BYTESIZE = 16384;
constexpr unsigned int MAX_SAMPLE_BUFFER_SIZE = MAX_SAMPLE_BUFFER_BYTESIZE / sizeof(Sample);

class SampleBuffer
{
public:
    SampleBuffer(Sample *buffer) :
        m_buffer(buffer) {}

    void clear() {
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
    Sample *m_buffer = nullptr;
    unsigned int m_size = MAX_SAMPLE_BUFFER_SIZE;
    Sample *m_modified = nullptr;
};

