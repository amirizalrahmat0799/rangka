#pragma once
// A value or an error message. The core is built without exceptions (so it compiles to WebAssembly with
// -fno-exceptions), so every function that can fail returns one of these instead of throwing.

#include <string>
#include <utility>
#include <variant>

namespace rangka {

struct Error {
    std::string message;
};

template <typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error error) : data_(std::move(error)) {}

    bool ok() const { return data_.index() == 0; }
    explicit operator bool() const { return ok(); }

    T& value() { return std::get<0>(data_); }
    const T& value() const { return std::get<0>(data_); }
    T& operator*() { return value(); }
    const T& operator*() const { return value(); }
    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }

    const std::string& error() const { return std::get<1>(data_).message; }

private:
    std::variant<T, Error> data_;
};

inline Error fail(std::string message) { return Error{std::move(message)}; }

}  // namespace rangka
