#pragma once

/// @file search_attributes.h
/// @brief Search attribute keys and collections for workflow visibility.

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace temporalio::common {

/// Indexed value types for search attributes.
enum class IndexedValueType : int {
    kText = 1,
    kKeyword = 2,
    kInt = 3,
    kDouble = 4,
    kBool = 5,
    kDatetime = 6,
    kKeywordList = 7,
};

/// Base (type-erased) search attribute key. Holds name and value type.
class SearchAttributeKeyBase {
public:
    SearchAttributeKeyBase(std::string name, IndexedValueType value_type)
        : name_(std::move(name)), value_type_(value_type) {}

    virtual ~SearchAttributeKeyBase() = default;

    /// Key name.
    const std::string& name() const noexcept { return name_; }

    /// Key value type.
    IndexedValueType value_type() const noexcept { return value_type_; }

    bool operator==(const SearchAttributeKeyBase& other) const {
        return name_ == other.name_ && value_type_ == other.value_type_;
    }

    bool operator<(const SearchAttributeKeyBase& other) const {
        if (name_ != other.name_) return name_ < other.name_;
        return value_type_ < other.value_type_;
    }

private:
    std::string name_;
    IndexedValueType value_type_;
};

/// Typed search attribute key.
template <typename T>
class SearchAttributeKey : public SearchAttributeKeyBase {
public:
    SearchAttributeKey(std::string name, IndexedValueType value_type)
        : SearchAttributeKeyBase(std::move(name), value_type) {}
};

/// Factory functions for creating typed search attribute keys.
namespace search_attribute {

/// Create a "text" search attribute key.
inline SearchAttributeKey<std::string> create_text(std::string name) {
    return {std::move(name), IndexedValueType::kText};
}

/// Create a "keyword" search attribute key.
inline SearchAttributeKey<std::string> create_keyword(std::string name) {
    return {std::move(name), IndexedValueType::kKeyword};
}

/// Create an "int" (long) search attribute key.
inline SearchAttributeKey<int64_t> create_long(std::string name) {
    return {std::move(name), IndexedValueType::kInt};
}

/// Create a "double" search attribute key.
inline SearchAttributeKey<double> create_double(std::string name) {
    return {std::move(name), IndexedValueType::kDouble};
}

/// Create a "bool" search attribute key.
inline SearchAttributeKey<bool> create_bool(std::string name) {
    return {std::move(name), IndexedValueType::kBool};
}

/// Create a "datetime" search attribute key.
inline SearchAttributeKey<std::chrono::system_clock::time_point>
create_datetime(std::string name) {
    return {std::move(name), IndexedValueType::kDatetime};
}

/// Create a "keyword list" search attribute key.
inline SearchAttributeKey<std::vector<std::string>>
create_keyword_list(std::string name) {
    return {std::move(name), IndexedValueType::kKeywordList};
}

} // namespace search_attribute

/// Variant type for search attribute values.
using SearchAttributeValue =
    std::variant<std::string, int64_t, double, bool,
                 std::chrono::system_clock::time_point,
                 std::vector<std::string>>;

/// Collection of search attribute key-value pairs, ordered by key name.
class SearchAttributeCollection {
public:
    SearchAttributeCollection() = default;

    /// Set a typed value for a key.
    template <typename T>
    void set(const SearchAttributeKey<T>& key, T value) {
        entries_[key.name()] = SearchAttributeValue{std::move(value)};
    }

    /// Remove a key.
    void remove(const std::string& key_name) { entries_.erase(key_name); }

    /// Check if a key is present.
    bool contains(const std::string& key_name) const {
        return entries_.count(key_name) > 0;
    }

    /// Get the raw variant value for a key name.
    const SearchAttributeValue* get(const std::string& key_name) const {
        auto it = entries_.find(key_name);
        if (it == entries_.end()) return nullptr;
        return &it->second;
    }

    /// Get the typed value for a key. Returns nullptr if not found or wrong type.
    template <typename T>
    const T* get_typed(const SearchAttributeKey<T>& key) const {
        auto val = get(key.name());
        if (!val) return nullptr;
        return std::get_if<T>(val);
    }

    /// Number of entries.
    std::size_t size() const noexcept { return entries_.size(); }

    /// Whether collection is empty.
    bool empty() const noexcept { return entries_.empty(); }

    /// Access the underlying map.
    const std::map<std::string, SearchAttributeValue>& entries() const {
        return entries_;
    }

private:
    std::map<std::string, SearchAttributeValue> entries_;
};

} // namespace temporalio::common
