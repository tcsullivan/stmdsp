#include <array>

constexpr unsigned int MAX_ERROR_QUEUE_SIZE = 8;

enum class Error : char
{
    None = 0,
    BadParam,
    BadParamSize,
    BadUserCodeLoad,
    BadUserCodeSize,
    NotIdle,
    ConversionAborted
};

class ErrorManager
{
public:
    void add(Error error) {
        if (m_index < m_queue.size())
            m_queue[m_index++] = error;
    }

    bool assert(bool condition, Error error) {
        if (!condition)
            add(error);
        return condition;
    }

    bool hasError() {
        return m_index > 0;
    }

    Error pop() {
        return m_index == 0 ? Error::None : m_queue[--m_index];
    }

private:
    std::array<Error, MAX_ERROR_QUEUE_SIZE> m_queue;
    unsigned int m_index = 0;
};

