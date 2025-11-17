#pragma once

#include <cstddef>
#include <cstring>
#include <type_traits>

class Page {
public:
    static constexpr std::size_t PAGE_SIZE = 128;

    Page() {
        clear();
    }

    char* getData() {
        return data;
    }

    const char* getData() const {
        return data;
    }

    void clear() {
        std::memset(data, 0, PAGE_SIZE);
    }

    template <typename T>
    bool serialize(const T& value) {
        static_assert(std::is_trivially_copyable<T>::value, "Page::serialize requires trivially copyable type");
        if (sizeof(T) > PAGE_SIZE) {
            return false;
        }
        clear();
        std::memcpy(data, &value, sizeof(T));
        return true;
    }

    template <typename T>
    bool deserialize(T& value) const {
        static_assert(std::is_trivially_copyable<T>::value, "Page::deserialize requires trivially copyable type");
        if (sizeof(T) > PAGE_SIZE) {
            return false;
        }
        std::memcpy(&value, data, sizeof(T));
        return true;
    }

private:
    char data[PAGE_SIZE];
};
