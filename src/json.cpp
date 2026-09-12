#include <blaze/json.h>

#include <boost/json/src.hpp>

#include <limits>

namespace blaze {

struct Json::Impl {
    boost::json::value value{nullptr};
};

namespace {

template<typename T>
T checked_integer(std::int64_t value, std::string_view type_name) {
    if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max()) {
        throw BadRequest("JSON number is out of range for " + std::string(type_name));
    }
    return static_cast<T>(value);
}

template<typename T>
T checked_unsigned(std::uint64_t value, std::string_view type_name) {
    if (value > std::numeric_limits<T>::max()) {
        throw BadRequest("JSON number is out of range for " + std::string(type_name));
    }
    return static_cast<T>(value);
}

[[noreturn]] void throw_type_error(std::string_view expected) {
    throw BadRequest("Expected JSON " + std::string(expected));
}

} // namespace

Json::Json() : impl_(std::make_unique<Impl>()) {}

Json::Json(std::nullptr_t) : Json() {}

Json::Json(bool value) : Json() {
    impl_->value = value;
}

Json::Json(int value) : Json(static_cast<std::int64_t>(value)) {}

Json::Json(unsigned int value) : Json(static_cast<std::uint64_t>(value)) {}

Json::Json(std::int64_t value) : Json() {
    impl_->value = value;
}

Json::Json(std::uint64_t value) : Json() {
    impl_->value = value;
}

Json::Json(double value) : Json() {
    impl_->value = value;
}

Json::Json(const char* value) : Json(std::string_view(value ? value : "")) {}

Json::Json(std::string value) : Json() {
    impl_->value = std::move(value);
}

Json::Json(std::string_view value) : Json(std::string(value)) {}

Json::Json(std::initializer_list<Member> members) : Json() {
    boost::json::object object;
    for (const auto& [key, value] : members) {
        object.emplace(key, value.impl_->value);
    }
    impl_->value = std::move(object);
}

Json::Json(const Json& other) : Json() {
    impl_->value = other.impl_->value;
}

Json::Json(Json&& other) : impl_(std::move(other.impl_)) {
    if (!impl_) {
        impl_ = std::make_unique<Impl>();
    }
    other.impl_ = std::make_unique<Impl>();
}

Json& Json::operator=(const Json& other) {
    if (this != &other) {
        impl_->value = other.impl_->value;
    }
    return *this;
}

Json& Json::operator=(Json&& other) {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        if (!impl_) {
            impl_ = std::make_unique<Impl>();
        }
        other.impl_ = std::make_unique<Impl>();
    }
    return *this;
}

Json::~Json() = default;

Json Json::object(std::initializer_list<Member> members) {
    return Json(members);
}

Json Json::array(std::initializer_list<Json> items) {
    boost::json::array array;
    array.reserve(items.size());
    for (const auto& item : items) {
        array.push_back(item.impl_->value);
    }

    Json json;
    json.impl_->value = std::move(array);
    return json;
}

std::expected<Json, util::ParseError> Json::parse(std::string_view text) {
    boost::system::error_code ec;
    auto value = boost::json::parse(text, ec);
    if (ec) {
        return std::unexpected(util::ParseError{
            util::ParseErrorCode::Invalid,
            "Invalid JSON: " + ec.message()
        });
    }

    Json json;
    json.impl_->value = std::move(value);
    return json;
}

Json::Type Json::type() const noexcept {
    const auto& value = impl_->value;
    if (value.is_null()) return Type::Null;
    if (value.is_bool()) return Type::Bool;
    if (value.is_int64()) return Type::Int;
    if (value.is_uint64()) return Type::UInt;
    if (value.is_double()) return Type::Double;
    if (value.is_string()) return Type::String;
    if (value.is_array()) return Type::Array;
    return Type::Object;
}

bool Json::is_null() const noexcept {
    return impl_->value.is_null();
}

bool Json::is_bool() const noexcept {
    return impl_->value.is_bool();
}

bool Json::is_number() const noexcept {
    return impl_->value.is_number();
}

bool Json::is_string() const noexcept {
    return impl_->value.is_string();
}

bool Json::is_array() const noexcept {
    return impl_->value.is_array();
}

bool Json::is_object() const noexcept {
    return impl_->value.is_object();
}

Json Json::operator[](size_t idx) const {
    if (!impl_->value.is_array()) {
        return Json();
    }

    const auto& array = impl_->value.as_array();
    if (idx >= array.size()) {
        return Json();
    }

    Json json;
    json.impl_->value = array[idx];
    return json;
}

Json Json::operator[](std::string_view key) const {
    if (!impl_->value.is_object()) {
        return Json();
    }

    const auto& object = impl_->value.as_object();
    const auto it = object.find(key);
    if (it == object.end()) {
        return Json();
    }

    Json json;
    json.impl_->value = it->value();
    return json;
}

bool Json::has(std::string_view key) const {
    return impl_->value.is_object() && impl_->value.as_object().contains(key);
}

size_t Json::size() const noexcept {
    if (impl_->value.is_array()) return impl_->value.as_array().size();
    if (impl_->value.is_object()) return impl_->value.as_object().size();
    return 0;
}

void Json::set(std::string_view key, Json value) {
    if (!impl_->value.is_object()) {
        impl_->value = boost::json::object{};
    }

    impl_->value.as_object().insert_or_assign(key, value.impl_->value);
}

template<>
bool Json::as<bool>() const {
    if (!impl_->value.is_bool()) {
        throw_type_error("bool");
    }
    return impl_->value.as_bool();
}

template<>
int Json::as<int>() const {
    if (impl_->value.is_int64()) {
        return checked_integer<int>(impl_->value.as_int64(), "int");
    }
    if (impl_->value.is_uint64()) {
        return checked_unsigned<int>(impl_->value.as_uint64(), "int");
    }
    throw_type_error("int");
}

template<>
unsigned int Json::as<unsigned int>() const {
    if (impl_->value.is_uint64()) {
        return checked_unsigned<unsigned int>(impl_->value.as_uint64(), "unsigned int");
    }
    if (impl_->value.is_int64()) {
        const auto value = impl_->value.as_int64();
        if (value < 0) {
            throw BadRequest("JSON number is out of range for unsigned int");
        }
        return checked_unsigned<unsigned int>(static_cast<std::uint64_t>(value), "unsigned int");
    }
    throw_type_error("unsigned int");
}

template<>
std::int64_t Json::as<std::int64_t>() const {
    if (!impl_->value.is_int64()) {
        throw_type_error("int");
    }
    return impl_->value.as_int64();
}

template<>
std::uint64_t Json::as<std::uint64_t>() const {
    if (impl_->value.is_uint64()) {
        return impl_->value.as_uint64();
    }
    if (impl_->value.is_int64()) {
        const auto value = impl_->value.as_int64();
        if (value < 0) {
            throw BadRequest("JSON number is out of range for uint64_t");
        }
        return static_cast<std::uint64_t>(value);
    }
    throw_type_error("uint");
}

template<>
double Json::as<double>() const {
    if (!impl_->value.is_number()) {
        throw_type_error("number");
    }
    return impl_->value.as_double();
}

template<>
std::string Json::as<std::string>() const {
    if (!impl_->value.is_string()) {
        throw_type_error("string");
    }
    return std::string(impl_->value.as_string());
}

template<>
Json Json::as<Json>() const {
    return *this;
}

Json::operator std::string() const {
    return as<std::string>();
}

Json::operator int() const {
    return as<int>();
}

std::string Json::dump() const {
    return boost::json::serialize(impl_->value);
}

} // namespace blaze
