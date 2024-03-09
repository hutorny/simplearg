/*
 * Copyright (C) 2024 Eugene Hutorny <eugene@hutorny.in.ua>
 *
 * arguments.h - a simple parameterized argument parser for C++17
 *
 * Licensed under MIT License, see full text in LICENSE
 * or visit page https://opensource.org/license/mit/
 */

#pragma once
#include <array>
#include <charconv>
#include <type_traits>
#include <string>
#include <string_view>
#include <cstring>
#include <limits>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <tuple>

namespace simplearg {

class Arguments;

template<class Class>
class Parameter {
public:
    using dispatcher_type = bool(Class::*)(std::string_view, Arguments&);
    constexpr Parameter(dispatcher_type dispatcher, const char name[], const char description[], const char aliases[])
      : dispatcher_ { dispatcher }, name_{name}, description_{description}, aliases_{aliases}{}
    Parameter(Parameter&&) = default;
    Parameter(const Parameter&) = default;
    Parameter& operator=(Parameter&&) = default;
    Parameter& operator=(const Parameter&) = default;
    constexpr auto name() const noexcept { return name_; }
    constexpr auto description() const noexcept { return description_; }
    constexpr auto aliases() const noexcept { return aliases_; }
    constexpr auto dispatcher() const noexcept { return dispatcher_; }
    constexpr operator bool() const noexcept { return !(name_ == nullptr || name_[0] == '\0' || dispatcher_ == nullptr); }
private:
    dispatcher_type dispatcher_;
    const char* name_;
    const char* description_;
    const char* aliases_;
};

template<class Class, std::size_t Size>
using Parameters = std::array<Parameter<Class>, Size>;

class Arguments {
public:
    Arguments(int argc, char** argv) : count_ {argc}, values_{argv} {}
    Arguments(Arguments&&) = default;
    Arguments(const Arguments&) = default;
    Arguments& operator=(Arguments&&) = default;
    Arguments& operator=(const Arguments&) = default;
    constexpr operator bool() const noexcept { return count_ > 0; }
    constexpr bool empty() const noexcept { return count_ <= 0; }
    Arguments& operator++() noexcept { count_--; values_++; return *this; }
    template<typename T> std::enable_if_t<std::is_integral_v<T>, bool>
    get(T& value) {
        static_assert(std::is_integral_v<T>);
        using l=std::numeric_limits<T>;
        if (count_ <= 0) return false;
        const char* end = *values_ + std::strlen(*values_);
        auto [ptr, ec] = std::from_chars(*values_, end, value);
        if (ec == std::errc::invalid_argument) {
            message("expects number in place of '", *values_, '\'');
            return false;
        }
        if (ec == std::errc::result_out_of_range) {
            message("expects number in range [", std::to_string(l::lowest()), "..",
                          std::to_string(l::max()), "] in place of '", *values_, '\'');
            return false;
        }
        count_--;
        values_++;
        return true;
    }
    bool get(double& value) {
        if (count_ <= 0) return false;
        const char* end = *values_ + std::strlen(*values_);
        auto [ptr, ec] = std::from_chars(*values_, end, value);
        if (ec == std::errc::invalid_argument) {
            message("expects floating point value in place of '", *values_, '\'');
            return false;
        }
        if (ec == std::errc::result_out_of_range) {
            message("expects number in double range in place of '", *values_, '\'');
            return false;
        }
        count_--;
        values_++;
        return true;
    }
    bool get(std::string& value) {
        if (count_ <= 0) return false;
        value = *values_++;
        count_--;
        return true;
    }
    std::string_view get() {
        if (count_ <= 0) return {};
        count_--;
        return *values_++;
    }
    template<typename ... T>
    bool getall(T& ... values) {
        if (count_ < 0) return false;
        if(static_cast<int>(sizeof...(T)) > count_) {
            message("expects ", std::to_string(sizeof...(T)), " parameters, got only ", std::to_string(count_));
            return false;
        }
        return (get(values) && ...);
    }
    const std::string& errors() const noexcept { return errors_; }
    std::string errors(std::string&& initial) {
        std::string result { std::move(errors_) };
        errors_ = std::move(initial);
        return result;
    }
    bool contains(const char value[]) const noexcept {
        for(int i = 0; i < count_; i++)
            if( strcmp(value, values_[i]) == 0) return true;
        return false;
    }
    template<class Class, std::size_t Size>
    bool parse(Class& obj, const Parameters<Class, Size>& params) {
        if (count_ <= 0) return false;
        std::unordered_map<std::string_view, typename Parameter<Class>::dispatcher_type> dispatchers{};
        for(auto p : params) {
            if (!p) continue;
            dispatchers[p.name()] = p.dispatcher();
            fillaliases(p.aliases(), [&dispatchers, &p](std::string_view alias) mutable {
                if (!alias.empty()) dispatchers[alias] = p.dispatcher();
            });
        }
        for(std::string_view param = get(); count_ >= 0 && ! param.empty(); param = get()) {
            const auto eq = param.find('=');
            if (eq != param.npos) {
                param = param.substr(0, eq + 1);
            }
            auto p = dispatchers.find(param);
            if (p == dispatchers.end()) {
                message("Unknown verb '", param, "' expected one of:");
                for(auto p : params) message(' ', p.name());
                return false;
            }
            const auto saved = eq != param.npos ? unget(eq + 1) : nothing;
            if (! (obj.*p->second)(param, *this) ) {
                revert(saved);
                return false;
            }
            revert(saved);
        }

        return true;
    }
private:
    static constexpr inline std::pair<char**, char*> nothing {};
    std::pair<char**, char*> unget(int pos) noexcept {
        if (count_ < 0) return {};
        count_++;
        values_--;
        std::pair<char**, char*> result { values_, *values_ };
        values_[0] += pos;
        return result;
    }
    static void revert(std::pair<char**, char*> v) noexcept {
        if (v.first != nullptr && v.second != nullptr) {
            *v.first = v.second;
        }
    }
    template<typename ... T>
    void message(T ... str) {
        count_= -1;
        ((errors_ += str), ...);
    }
    void fillaliases(const char* aliases, std::function<void(std::string_view)> put) {
        if (aliases == nullptr || aliases[0] == '\0' ) return;
        std::string_view current { aliases };
        while(!current.empty()) {
            while(!current.empty() && current[0] == ' ') current.remove_prefix(1);
            auto space = current.find(' ');
            if (space == current.npos) {
                put(current);
                break;
            } else {
                put(current.substr(0, space));
                current.remove_prefix(space+1);
            }
        }
    }
    int count_;
    char** values_;
    std::string errors_;
};

template<class Stream, class Class, std::size_t Size>
Stream& print(Stream& out, const Parameters<Class, Size>& params, std::string_view bullet = " - ", std::string_view alias_label = "Aliases: ") {
    std::size_t width {alias_label.size()};
    for(auto p : params) width = std::max(width, strlen(p.name()));
    for(auto p : params) {
        out.width(width + 1);
        out << std::left << p.name() << bullet << p.description() << '\n';
        if (p.aliases() != 0 && p.aliases()[0] != '\0') {
            out.width(width + 1 + bullet.length());
            out << std::right << alias_label << p.aliases() << '\n';
        }
    }
    return out;
}

} // namespace simplearg
