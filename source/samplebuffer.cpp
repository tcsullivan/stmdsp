/**
 * @file samplebuffer.cpp
 * @brief Manages ADC/DAC buffer data.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "samplebuffer.hpp"

SampleBuffer::SampleBuffer(Sample *buffer) :
    m_buffer(buffer) {}

void SampleBuffer::clear() {
    std::fill(m_buffer, m_buffer + m_size, 2048);
}
__attribute__((section(".convcode")))
void SampleBuffer::modify(Sample *data, unsigned int srcsize) {
    auto size = srcsize < m_size ? srcsize : m_size;
    m_modified = m_buffer;
    for (Sample *d = m_buffer, *s = data; s != data + size;)
        *d++ = *s++;
}
__attribute__((section(".convcode")))
void SampleBuffer::midmodify(Sample *data, unsigned int srcsize) {
    auto size = srcsize < m_size / 2 ? srcsize : m_size / 2;
    m_modified = middata();
    for (Sample *d = middata(), *s = data; s != data + size;)
        *d++ = *s++;
}

void SampleBuffer::setModified() {
    m_modified = m_buffer;
}

void SampleBuffer::setMidmodified() {
    m_modified = middata();
}

void SampleBuffer::setSize(unsigned int size) {
    m_size = size < MAX_SAMPLE_BUFFER_SIZE ? size : MAX_SAMPLE_BUFFER_SIZE;
}

__attribute__((section(".convcode")))
Sample *SampleBuffer::data() {
    return m_buffer;
}
__attribute__((section(".convcode")))
Sample *SampleBuffer::middata() {
    return m_buffer + m_size / 2;
}
uint8_t *SampleBuffer::bytedata() {
    return reinterpret_cast<uint8_t *>(m_buffer);
}

Sample *SampleBuffer::modified() {
    auto m = m_modified;
    m_modified = nullptr;
    return m;
}
__attribute__((section(".convcode")))
unsigned int SampleBuffer::size() const {
    return m_size;
}
unsigned int SampleBuffer::bytesize() const {
    return m_size * sizeof(Sample);
}

