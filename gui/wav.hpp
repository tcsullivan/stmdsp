#ifndef WAV_HPP_
#define WAV_HPP_

#include <cstdint>
#include <cstring>
#include <fstream>

namespace wav
{
    struct header {
        char riff[4];      // "RIFF"
        uint32_t filesize; // Total file size minus eight bytes
        char wave[4];      // "WAVE"

        bool valid() const {
            return strncmp(riff, "RIFF", 4) == 0 && filesize > 8 && strncmp(wave, "WAVE", 4) == 0;
        }
    } __attribute__ ((packed));

    struct format {
        char fmt_[4]; // "fmt "
        uint32_t size;
        uint16_t type;
        uint16_t channelcount;
        uint32_t samplerate;
        uint32_t byterate;
        uint16_t unused;
        uint16_t bps;

        bool valid() const {
            return strncmp(fmt_, "fmt ", 4) == 0;
        }
    } __attribute__ ((packed));

    struct data {
        char data[4]; // "data"
        uint32_t size;

        bool valid() const {
            return strncmp(data, "data", 4) == 0;
        }
    } __attribute__ ((packed));

    class clip {
    public:
        clip(const char *path) {
            std::ifstream file (path);
            if (!file.good())
                return;
            {
                header h;
                file.read(reinterpret_cast<char *>(&h), sizeof(header));
                if (!h.valid())
                    return;
            }
            {
                format f;
                file.read(reinterpret_cast<char *>(&f), sizeof(format));
                if (!f.valid() || f.type != 1) // ensure PCM
                    return;
            }
            {
                wav::data d;
                file.read(reinterpret_cast<char *>(&d), sizeof(wav::data));
                if (!d.valid())
                    return;
                m_data = new char[d.size + 4096 - (d.size % 4096)];
                m_size = d.size;
                file.read(m_data, d.size);
            }
        }

        bool valid() const {
            return m_data != nullptr && m_size > 0;
        }
        auto data() const {
            return m_data;
        }
        auto next(unsigned int chunksize = 3000) {
            if (m_pos == m_size) {
                m_pos = 0;
            }
            auto ret = m_data + m_pos;
            m_pos = std::min(m_pos + chunksize, m_size);
            return ret;
        }
    private:
        char *m_data = nullptr;
        uint32_t m_size = 0;
        uint32_t m_pos = 0;
    };
}

#endif // WAV_HPP_

