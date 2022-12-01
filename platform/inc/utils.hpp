#pragma once

template<typename T, typename... Args>
static constexpr inline bool is_type() {
    return (std::is_same<T, Args>::value && ...);
}
