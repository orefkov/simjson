/*
 * (c) Проект "SimJson", Александр Орефков orefkov@gmail.com
 * ver. 1.0
 * Классы для работы с JSON
 * (c) Project "SimJson", Aleksandr Orefkov orefkov@gmail.com
 * ver. 1.0
 * Classes for working with JSON
 */

#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <simstr/sstring.h>
#include <cassert>
#include <type_traits>
#include <utility>

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#ifdef SIMJSON_IN_SHARED
    #if defined(_MSC_VER) || (defined(__clang__) && __has_declspec_attribute(dllexport))
        #ifdef SIMJSON_EXPORT
            #define SIMJSON_API __declspec(dllexport)
        #else
            #define SIMJSON_API __declspec(dllimport)
        #endif
    #elif (defined(__GNUC__) || defined(__GNUG__)) && defined(SIMSTR_EXPORT)
        #define SIMJSON_API __attribute__((visibility("default")))
    #else
        #define SIMJSON_API
    #endif
#else
    #define SIMJSON_API
#endif

/*!
 * @ru @brief Пространство имён для объектов библиотеки
 * @en @brief Library namespace
 */
namespace simjson {
using namespace simstr;
using namespace simstr::literals;

class Json {
public:
    enum Type {
        Undefined,
        Null,
        Boolean,
        Text,
        Integer,
        Real,
        Object,
        Array
    };
    struct null_t {};
    struct emptyString_t {};
    struct emptyObject_t {};
    struct emptyArray_t {};

    inline static const null_t null;
    inline static const emptyString_t emptyString;
    inline static const emptyObject_t emptyObject;
    inline static const emptyArray_t emptyArray;
};

enum class JsonParseResult {
    Success,
    Pending,
    NoNeedMore,
    Error,
};

struct StreamedJsonParserBase {

    unsigned line_{};
    unsigned col_{};

protected:
    int state_ {};
    u16s currentUnicode_[2]{};
    int idxUnicode_{};
};

namespace jt {

template<typename K>
using KeyType = StoreType<K, strhash<K>>;

template<typename T, typename K>
concept JsonType = std::is_integral_v<std::remove_cvref_t<T>> ||
    std::is_floating_point_v<std::remove_cvref_t<T>> ||
    std::is_constructible_v<sstring<K>, T>;

template<typename T, typename K>
concept JsonKeyType = std::convertible_to<T, simple_str<K>> ||
    std::same_as<std::remove_cvref_t<T>, KeyType<K>>;

template<typename T, typename K>
concept JsonArraySource = requires(const T& t) {
    { t.empty() } -> std::same_as<bool>;
    { t.size() } -> std::same_as<size_t>;
    { *t.begin() } -> JsonType<K>;
    { *t.end() } -> JsonType<K>;
};

template<typename T, typename K>
concept JsonObjectSource = requires(const T& t) {
    { t.empty() } -> std::same_as<bool>;
    { t.size() } -> std::same_as<size_t>;
    { t.begin()->first } -> JsonKeyType<K>;
    { t.end()->first } -> JsonKeyType<K>;
    { t.begin()->second } -> JsonType<K>;
    { t.end()->second } -> JsonType<K>;
};

} // namespace jt

/*!
 * @brief Класс для представления json значения.
 * @tparam K - тип символов.
 * @en @brief Class to represent a json value.
 * @tparam K - character type.
 */
template<typename K>
class JsonValueTempl : public Json {
public:
    using strType = sstring<K>;
    using ssType = simple_str<K>;

    using json_value = JsonValueTempl<K>;
    using obj_type = hashStrMap<K, JsonValueTempl<K>>;
    using arr_type = std::vector<JsonValueTempl<K>>;
    using json_object = std::shared_ptr<obj_type>;
    using json_array = std::shared_ptr<arr_type>;

    /// @ru Создает пустой объект с типом Undefined.
    /// @en Creates an empty object of type Undefined.
    JsonValueTempl() : type_(Undefined) {}
    /// @ru Конструктор копирования. Объекты и массивы копируются по ссылке
    /// @en Copy constructor. Objects and arrays are copied by reference.
    SIMJSON_API JsonValueTempl(const JsonValueTempl& other);
    /// @ru Конструктор перемещения.
    /// @en Move constructor.
    JsonValueTempl(JsonValueTempl&& other) noexcept {
        type_ = other.type_;
        switch (other.type_) {
        case Json::Text:
            new (&val_.text) strType(std::move(other.val_.text));
            break;
        case Json::Object:
            new (&val_.object) json_object(std::move(other.val_.object));
            break;
        case Json::Array:
            new (&val_.array) json_array(std::move(other.val_.array));
            break;
        default:
            memcpy(&val_, &other.val_, sizeof(val_));
        }
        other.type_ = Undefined;
    }
    /// @ru Деструктор.
    /// @en Destructor.
    SIMJSON_API ~JsonValueTempl();

    /// @ru Конструктор из int8_t.
    /// @en Constructor from int8_t.
    JsonValueTempl(int8_t v) : type_(Integer) { val_.integer = v; }
    /// @ru Конструктор из int16_t.
    /// @en Constructor from int16_t.
    JsonValueTempl(int16_t v) : type_(Integer) { val_.integer = v; }
    /// @ru Конструктор из int32_t.
    /// @en Constructor from int32_t.
    JsonValueTempl(int32_t v) : type_(Integer) { val_.integer = v; }
    /// @ru Конструктор из int64_t.
    /// @en Constructor from int64_t.
    JsonValueTempl(int64_t v) : type_(Integer) { val_.integer = v; }
    /// @ru Конструктор из bool.
    /// @en Constructor from bool.
    JsonValueTempl(bool v) : type_(Boolean) { val_.boolean = v; }
    /// @ru Конструктор из double.
    /// @en Constructor from double.
    JsonValueTempl(double v) : type_(Real) { val_.real = v; }

    /// @ru Конструктор из sstring.
    /// @en Constructor from sstring.
    JsonValueTempl(strType t) : type_(Text) {
        new (&val_.text) strType(std::move(t));
    }
    /// @ru Конструктор из остальных строк.
    /// @en Constructor from the other strings.
    JsonValueTempl(ssType t) : type_(Text) {
        new (&val_.text) strType(t);
    }
    /// @ru Конструктор из строковых выражений.
    /// @en Constructor from string expressions.
    template<StrExprForType<K> A>
    JsonValueTempl(const A& e) : type_(Text) {
        new (&val_.text) strType(e);
    }
    /// @ru Конструктор из строковых литералов.
    /// @en Constructor from string literals.
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    JsonValueTempl(T&& str) : type_(Text) {
        new (&val_.text) strType(std::forward<T>(str));
    }
    /// @ru Конструктор для создания json::null.
    /// @en Constructor for creating json::null.
    JsonValueTempl(const null_t&) : type_(Null) {}
    /// @ru Конструктор для создания пустой строки.
    /// @en Constructor for creating an empty string.
    JsonValueTempl(const emptyString_t&) : type_(Text) { new (&val_.text) strType; }
    /// @ru Конструктор для создания json::object.
    /// @en Constructor for creating json::object.
    JsonValueTempl(const emptyObject_t&) : type_(Object) {
        new (&val_.object) json_object(std::make_shared<obj_type>());
    }
    /// @ru Конструктор для создания json::array.
    /// @en Constructor for creating json::array.
    JsonValueTempl(const emptyArray_t&) : type_(Array) {
        new (&val_.array) json_array(std::make_shared<arr_type>());
    }
    /// @ru Конструктор для создания дефолтного значения с типом type.
    /// @en Constructor for creating a default value with type type.
    SIMJSON_API JsonValueTempl(Type type);

    struct KeyInit : std::pair<const jt::KeyType<K>, json_value> {
        using base = std::pair<const jt::KeyType<K>, json_value>;
        KeyInit(const jt::KeyType<K>& k, const json_value& v) : base(k, v){}
        KeyInit(ssType k, const json_value& v) : base(obj_type::toStoreType(k), v){}
    };
    using ObjectInit = std::initializer_list<KeyInit>;
    /*!
     * @ru @brief Конструктор из инициализатора для создания json::object.
     * @en @brief Constructor from initializer to create json::object.
     * @~ `JsonValue v = {{"key1"_h, 1}, {"key2", "text"}, {"key3"_h, false}}; -> {"key1": 10, "key2": "text", "key3": false}`
     */
    JsonValueTempl(ObjectInit&& init) : type_(Object) {
        new (&val_.object) json_object(std::make_shared<obj_type>(std::move(reinterpret_cast<std::initializer_list<std::pair<const jt::KeyType<K>, json_value>>&>(init))));
    }
    using ArrayInit = std::initializer_list<json_value>;
    /*!
     * @ru @brief Конструктор из инициализатора для создания json::array.
     * @en @brief Constructor from initializer to create json::array.
     * @~ `JsonValue json = {"key1", 12, false, Json::null, 1.25, true}; -> ["key1", 12, false, null, 1.25, true]`
     */
    JsonValueTempl(ArrayInit&& init) : type_(Array) {
        new (&val_.array) json_array(std::make_shared<arr_type>(std::move(init)));
    }
    /*!
     * @ru @brief Конструктор из любого стандартного контейнера-массива, содержащего значения JSON.
     * @param obj - объект.
     * @en @brief Constructor from any standard array container containing JSON values.
     * @param obj is the object.
     */
    JsonValueTempl(jt::JsonArraySource<K> auto const& obj) : JsonValueTempl(emptyArray) {
        if (!obj.empty()) {
            auto& arr = *as_array();
            arr.reserve(obj.size());
            for (const auto& e : obj) {
                arr.emplace_back(e);
            }
        }
    }
    /*!
     * @ru @brief Конструктор из любого стандартного контейнера с парами, содержащими строковый ключ в first и значение JSON в second.
     * @param obj - объект.
     * @en @brief Constructor from any standard container with pairs containing a string key in first and a JSON value in second.
     * @param obj is the object.
     */
    JsonValueTempl(jt::JsonObjectSource<K> auto const& obj) : JsonValueTempl(emptyObject) {
        if (!obj.empty()) {
            auto& me = *as_object();
            for (const auto& [f, s] : obj) {
                me.emplace(f, s);
            }
        }
    }

    struct Clone {
        const json_value& from;
    };
    /*!
     * @ru @brief Конструктор клонирования. В этом случае для объектов и массивов создаются "глубокие" копии.
     * @param clone - клонируемый объект.
     * @return копию json-значения.
     * @en @brief Clone constructor. In this case, "deep" copies are created for objects and arrays.
     * @param clone - cloned object.
     * @return a copy of the json value.
     */
    SIMJSON_API JsonValueTempl(const Clone& clone);
    /// @ru Получить тип значения.
    /// @en Get value type.
    Type type() const { return type_; }
    /// @ru Клонировать значение.
    /// @en Clone value.
    json_value clone() const {
        return Clone{*this};
    }

    // Проверка типов
    // Type checking

    /// @ru Значение undefined.
    /// @en The value is undefined.
    bool is_undefined() const { return type_ == Undefined; };
    /// @ru Значение null.
    /// @en The value is null.
    bool is_null() const { return type_ == Null; };
    /// @ru Значение boolean.
    /// @en The value is boolean.
    bool is_boolean() const { return type_ == Boolean; }
    /// @ru Значение integer.
    /// @en The value is integer.
    bool is_integer() const { return type_ == Integer; }
    /// @ru Значение real (double).
    /// @en The value is real (double).
    bool is_real() const { return type_ == Real; }
    /// @ru Значение text.
    /// @en The value is text.
    bool is_text() const { return type_ == Text; }
    /// @ru Значение array.
    /// @en The value is array.
    bool is_array() const { return type_ == Array; }
    /// @ru Значение object.
    /// @en The value is object.
    bool is_object() const { return type_ == Object; }

    // -----------------------------   Boolean -----------------------------------------------

    /*!
     * @ru @brief Получить значение как boolean. В отладочной версии проверяется, что значение действительно boolean.
     * @en @brief Get the value as boolean. The debug version checks that the value is indeed boolean.
     */
    bool as_boolean() const {
        assert(type_ == Boolean);
        return val_.boolean;
    }
    /*!
     * @ru @brief Получить boolean, если хранится boolean, или ничего.
     * @details Пример:
     * @en @brief Get a boolean if a boolean is stored, or nothing.
     * @details Example:
     * @~   `bool val = json.boolean().value_or(false);`
     */
    std::optional<bool> boolean() const {
        if (type_ == Boolean) {
            return val_.boolean;
        }
        return {};
    }
    /// @ru Получить boolean, если хранится boolean, или выкинуть исключение. Пример: bool val = json.boolean_or_throw<std::runtime_error>("Need boolean value");
    /// @en Get boolean if boolean is stored, or throw an exception. Example: bool val = json.boolean_or_throw<std::runtime_error>("Need boolean value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    bool boolean_or_throw(Args&&...args) const {
        if (type_ == Boolean) {
            return val_.boolean;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// @ru Получить значение, конвертированное в boolean. Логика работы как в javascript `!!val`. Пример: bool val = json.to_boolean();
    /// @en Get the value converted to boolean. The logic of operation is the same as in javascript `!!val`. Example: bool val = json.to_boolean();
    SIMJSON_API bool to_boolean() const;

    // ---------------------------------------- Integer -------------------------------------
    /// @ru Получить значение как integer. В отладочной версии проверяется, что значение действительно integer
    /// @en Get value as integer. The debug version checks that the value is really an integer
    int64_t as_integer() const {
        assert(type_ == Integer);
        return val_.integer;
    }
    /// @ru Получить integer, если хранится integer, или ничего. Пример: auto val = json.integer().value_or(10);
    /// @en Get an integer if the stored value is an integer, or nothing. Example: auto val = json.integer().value_or(10);
    std::optional<int64_t> integer() const {
        if (type_ == Integer) {
            return val_.integer;
        }
        return {};
    }
    /// @ru Получить integer, если хранится integer, или выкинуть исключение. Пример: int val = json.integer_or_throw<std::runtime_error>("Need integer value");
    /// @en Get an integer if an integer is stored, or throw an exception. Example: int val = json.integer_or_throw<std::runtime_error>("Need integer value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t integer_or_throw(Args&&...args) const {
        if (type_ == Integer) {
            return val_.integer;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Получить значение, конвертированное в integer, или ничего. Логика работы как в javascript `1 * val`.
     * Если в результате NaN или Inf, то ничего. Нецелые числа - переводятся в целое.
     * Пример: int val = json.to_integer().value_or(10);
     * @en @brief Get the value converted to an integer or nothing. The logic is the same as in javascript `1 * val`.
     * If the result is NaN or Inf, then nothing. Non-integer numbers are converted to an integer.
     * Example: `int val = json.to_integer().value_or(10);`
     */
    SIMJSON_API std::optional<int64_t> to_integer() const;
    /// @ru Получить значение из to_integer, если оно есть, или выкинуть исключение.
    /// @en Get the value from to_integer if it exists, or throw an exception.
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t to_integer_or_throw(Args&&...args) const {
        if (auto val = to_integer()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    // -------------------------------------- Real --------------------------------------------
    /// @ru Получить значение как double. В отладочной версии проверяется, что значение действительно double
    /// @en Get the value as double. The debug version checks that the value is really double
    double as_real() const {
        assert(type_ == Real);
        return val_.real;
    }
    /// @ru Получить double, если хранится double, или ничего. Пример: auto val = json.real().value_or(10.0);
    /// @en Get double if double is stored, or nothing. Example: auto val = json.real().value_or(10.0);
    std::optional<double> real() const {
        if (type_ == Real) {
            return val_.real;
        }
        return {};
    }
    /// @ru Получить double, если хранится double, или выкинуть исключение. Пример: double val = json.real_or_throw<std::runtime_error>("Need double value");
    /// @en Get double if double is stored, or throw an exception. Example: double val = json.real_or_throw<std::runtime_error>("Need double value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    double real_or_throw(Args&&...args) {
        if (type_ == Real) {
            return val_.real;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    /// @ru Получить значение, конвертированное в double. Логика работы как в javascript `1 * val`. Для "не чисел" возвращает NaN.
    /// @en Get the value converted to double. The logic is the same as in javascript `1 * val`. For "non-numbers" returns NaN.
    SIMJSON_API double to_real() const;
    // --------------------------------------- Number -----------------------------
    // В нашей реализации в отличии от чистого json числа могут быть int64_t или double.
    // Поэтому добавим ещё логику для работы только с обоими вариантами чисел.
    // In our implementation, unlike pure json, numbers can be int64_t or double.
    // Therefore, let's add some more logic to work only with both variants of numbers.

    /// @ru Возвращает int64_t, если хранится int64_t или double влезающий в int64_t, либо ничего.
    /// @en Returns int64_t if stored int64_t or double that fits into int64_t, or nothing.
    std::optional<int64_t> number_int() const;
    /// @ru Получить int64_t, если хранится int64_t или double влезающий в int64_t, либо выбросить исключение.
    /// @en Get int64_t if stored int64_t or double that fits into int64_t, or throw an exception.
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t number_int_or_throw(Args&&...args) const {
        if (auto val = number_int()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// @ru Возвращает double, если хранится double или int64_t, либо ничего.
    /// @en Returns double if double or int64_t is stored, or nothing.
    SIMJSON_API std::optional<double> number_real() const;
    /// @ru Получить double, если хранится double или int64_t, либо выбросить исключение.
    /// @en Get double if double or int64_t is stored, or throw an exception.
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    double number_real_or_throw(Args&&...args) const {
        if (auto val = number_real()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    // --------------------------------------- Text -----------------------------

    /// @ru Получить значение как текст. В отладочной версии проверяется, что значение действительно текст.
    /// @en Get the value as text. The debug version checks that the value is actually text.
    const strType& as_text() const {
        assert(type_ == Text);
        return val_.text;
    }

    /// @ru Получить текст, если хранится текст, или ничего. Пример: auto val = json.text().value_or("default value");
    /// @en Get text if text is stored, or nothing. Example: auto val = json.text().value_or("default value");
    std::optional<strType> text() const {
        if (type_ == Text) {
            return val_.text;
        }
        return {};
    }
    /// @ru Получить текст, если хранится текст, или выкинуть исключение. Пример: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    /// @en Get the text if text is stored, or throw an exception. Example: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    const strType& text_or_throw(Args&&...args) const {
        if (type_ == Text) {
            return val_.text;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// @ru Получить текст, если хранится текст и он не пустой, или выкинуть исключение. Пример: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    /// @en Get the text if the text is stored and it is not empty, or throw an exception. Example: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    const strType& not_empty_text_or_throw(Args&&...args) const {
        if (type_ == Text && !val_.text.template trimmed<ssType>().is_empty()) {
            return val_.text;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    /// @ru Получить значение, конвертированное в текст. Логика работы как в javascript `"" + val`.
    /// @en Get the value converted to text. The logic is the same as in javascript `"" + val`.
    SIMJSON_API strType to_text() const;

    /// @ru Получить значение как json Object. В отладочной версии проверяется, что значение действительно Object.
    /// @en Get value as json Object. The debug version checks that the value is really Object.
    json_object& as_object() {
        assert(type_ == Object);
        return val_.object;
    }
    /// @ru Получить значение как json Object. В отладочной версии проверяется, что значение действительно Object.
    /// @en Get value as json Object. The debug version checks that the value is indeed Object.
    const json_object& as_object() const {
        assert(type_ == Object);
        return val_.object;
    }
    /// @ru Получить значение как json Array. В отладочной версии проверяется, что значение действительно Array.
    /// @en Get value as json Array. The debug version checks that the value is indeed an Array.
    json_array& as_array() {
        assert(type_ == Array);
        return val_.array;
    }
    /// @ru Получить значение как json Array. В отладочной версии проверяется, что значение действительно Array.
    /// @en Get value as json Array. The debug version checks that the value is indeed an Array.
    const json_array& as_array() const {
        assert(type_ == Array);
        return val_.array;
    }
    /// @ru Обменять значения.
    /// @en Exchange values.
    void swap(json_value& other) noexcept {
        char tmp[sizeof(*this)];
        memcpy(tmp, this, sizeof(*this));
        memcpy((void*)this, &other, sizeof(*this));
        memcpy((void*)&other, tmp, sizeof(*this));
    }

    /// @ru Присвоение.
    /// @en Assignment.
    JsonValueTempl& operator=(json_value t) noexcept {
        swap(t);
        return *this;
    }
    /*!
     * @ru @brief Обращение к свойству константного объекта по ключу.
     * @tparam T - типа ключа.
     * @param key - ключ.
     * @return const json_value& - ссылка на значение ключа.
     *   Если значение не объект, или ключа нет, не создаёт новую пару, а возвращает ссылку на UNDEFINED.
     * @en @brief Access to a property of a constant object by key.
     * @tparam T - key type.
     * @param key - key.
     * @return const json_value& - reference to the key value.
     * If the value is not an object, or there is no key, does not create a new pair, but returns a reference to UNDEFINED.
     */
    template<jt::JsonKeyType<K> T>
    const json_value& at(T&& key) const {
        if (type_ == Object) {
            if (auto it = as_object()->find(std::forward<T>(key)); it != as_object()->end())
                return it->second;
        }
        return UNDEFINED;
    }
    /*!
     * @ru @brief Обращение к свойству константного объекта по ключу.
     * @tparam T - типа ключа.
     * @param key - ключ.
     * @return const json_value& - ссылка на значение ключа.
     *   Если значение не объект, или ключа нет, не создаёт новую пару, а возвращает ссылку на UNDEFINED.
     * @en @brief Access to a property of a constant object by key.
     * @tparam T - key type.
     * @param key - key.
     * @return const json_value& - reference to the key value.
     * If the value is not an object, or there is no key, does not create a new pair, but returns a reference to UNDEFINED.
     */
    template<jt::JsonKeyType<K> T>
    const json_value& operator[](T&& key) const {
        return at(std::forward<T>(key));
    }
    /*!
     * @ru @brief Обращение к свойству константного объекта по набору ключей.
     * @details Функция последовательно переходит от значения к значению по заданным ключам.
     *  Как только указанный ключ не будет найден, перебор останавливается.
     *  Например: config("a", "b", "c") для объекта {"a": {"b": {"c": 10}}} вернёт ссылку на 10.
     * @tparam T - типа ключа.
     * @param key - ключ.
     * @param ...args - остальные ключи.
     * @return const json_value& - ссылка на значение ключа.
     *   Если значение не объект, или указанного ключа нет, не создаёт новую пару, а возвращает ссылку на UNDEFINED.
     * @en @brief Access to a property of a constant object by a set of keys.
     * @details The function moves sequentially from value to value based on the given keys.
     * As soon as the specified key is not found, the search stops.
     * For example: config("a", "b", "c") for object {"a": {"b": {"c": 10}}} will return a reference to 10.
     * @tparam T - key type.
     * @param key - key.
     * @param ...args - other keys.
     * @return const json_value& - reference to the key value.
     * If the value is not an object, or the specified key does not exist, does not create a new pair, but returns a reference to UNDEFINED.
     */
    template<jt::JsonKeyType<K> T, typename...Args>
    const json_value& operator()(T&& key, Args&&...args) const {
        const json_value& res = at(std::forward<T>(key));
        if constexpr (sizeof...(Args) == 0) {
            return res;
        } else {
            return res.is_undefined() ? res : res(std::forward<Args>(args)...);
        }
    }
    /*!
     * @ru @brief Обращение к свойству объекта по ключу.
     * @tparam T - тип ключа.
     * @param key - ключ.
     * @return json_value& - ссылку на значение ключа.
     * @details Если значение не json-объект, то "превращает" его в json-объект.
     *  Если указанного ключа нет - добавляет в json-объект этот ключ с пустым значением.
     * @en @brief Access to an object property by key.
     * @tparam T - type of the key.
     * @param key - key.
     * @return json_value& - a reference to the key value.
     * @details If the value is not a json object, then "turns" it into a json object.
     * If the specified key does not exist, adds this key with an empty value to the json object.
     */
    template<jt::JsonKeyType<K> T>
    json_value& operator[](T&& key) {
        if (type_ != Object) {
            assert(this != &UNDEFINED);
            *this = emptyObject;
        }
        return as_object()->try_emplace(std::forward<T>(key)).first->second;
    }
    /*!
     * @ru @brief Установка значения свойству json-объекта по ключу.
     * @tparam Key - тип ключа.
     * @tparam Args - типы аргументов для создания json-значения по указанному ключу.
     * @param key  - ключ.
     * @param args - аргументы  для создания json-значения по указанному ключу.
     * @return json_value& - ссылку на json-значение по указанному ключу.
     * @details Если значение не json-объект, то "превращает" его в json-объект.
     *  Если указанного ключа нет - добавляет в json-объект этот ключ с заданными аргументами,
     *  иначе присваивает ему аргументы.
     * @en @brief Setting the value of a json object property by key.
     * @tparam Key - the type of the key.
     * @tparam Args - types of arguments to create a json value for the specified key.
     * @param key - key.
     * @param args - arguments for creating a json value for the specified key.
     * @return json_value& - a link to the json value for the specified key.
     * @details If the value is not a json object, then "turns" it into a json object.
     * If the specified key does not exist, adds this key with the given arguments to the json object,
     * otherwise assigns arguments to it.
     */
    template<jt::JsonKeyType<K> Key, typename ... Args>
    json_value& set(Key&& key, Args&& ... args) {
        if (type_ != Object) {
            assert(this != &UNDEFINED);
            *this = emptyObject;
        }
        return as_object()->emplace(std::forward<Key>(key), std::forward<Args>(args)...).first->second;
    }
    /// @ru Обращение к элементу константного массива по индексу. Если это не json-массив или индекс за границами массива - возвращает ссылку на UNDEFINED.
    /// @en Accessing a constant array element by index. If this is not a json array or an index outside the bounds of the array, it returns a reference to UNDEFINED.
    const json_value& at(size_t idx) const {
        if (type_ == Array) {
            const auto& arr = *as_array();
            if (idx < arr.size()) {
                return arr[idx];
            }
        }
        return UNDEFINED;
    }
    /// @ru Обращение к элементу константного массива по индексу. Если это не json-массив или индекс за границами массива - возвращает ссылку на UNDEFINED.
    /// @en Accessing a constant array element by index. If this is not a json array or an index outside the bounds of the array, it returns a reference to UNDEFINED.
    const json_value& operator[](size_t idx) const {
        return at(idx);
    }
    /*!
     * @ru @brief Обращение к элементу массива по индексу.
     * @param idx - индекс элемента.
     * @return json_value& - ссылку на указанный элемент массива.
     * @details Если это значение не json-массив - "превращает" его в массив.
     *      Если индекс == -1 - добавляет в массив ещё один элемент.
     *      Если индекс больше длины массива - увеличивает массив до заданного индекса.
     * @en @brief Access an array element by index.
     * @param idx - element index.
     * @return json_value& - a reference to the specified array element.
     * @details If this value is not a json array, "turns" it into an array.
     * If index == -1 - adds one more element to the array.
     * If the index is greater than the length of the array, increases the array to the specified index.
     */
    json_value& operator[](size_t idx) {
        if (type_ != Array) {
            assert(this != &UNDEFINED);
            *this = emptyArray;
        }
        auto& arr = *as_array();
        if (idx == -1) {
            idx = arr.size();
        }
        if (idx >= arr.size()) {
            arr.resize(idx + 1);
        }
        return arr[idx];
    }
    /// @ru Количество элементов json-массива или ключей json-объекта.
    /// @en The number of elements of a json array or keys of a json object.
    size_t size() const {
        if (type_ == Array) {
            return as_array()->size();
        } else if (type_ == Object) {
            return as_object()->size();
        }
        return 0;
    }
    /*!
     * @ru @brief Слияние с другим JSON.
     * @param other - другое json-значение.
     * @param replace - заменять другим при слиянии.
     * @param append_arrays - при слиянии json-массивов объединять их.
     * @details Если replace == true, то другой JSON имеет приоритет и для простых типов, если он не undefined, он заменит текущий.
     *      Если оба объекта json-массивы, то при append_arrays = true другой массив дописывается к этому массиву,
     *      иначе если replace == true, то заменит текущий массив.
     *      Если оба JSONа объекты - то сливаются по ключам. Ключи из другого объекта, которых нет в текущем - добавятся в текущий
     *      объект. Которые есть - при replace == true будут заменены.
     * @en @brief Merge with other JSON.
     * @param other - another json value.
     * @param replace - replace with something else when merging.
     * @param append_arrays - when merging json arrays, combine them.
     * @details If replace == true then another JSON takes precedence and for simple types, if it is not undefined it will replace the current one.
     * If both objects are json arrays, then when append_arrays = true, another array is appended to this array,
     * otherwise if replace == true, it will replace the current array.
     * If both are JSON objects, then they are merged by keys. Keys from another object that are not in the current one will be added to the current one
     * object. Which exist - with replace == true they will be replaced.
     */
    SIMJSON_API void merge(const json_value& other, bool replace = true, bool append_arrays = false);
    /*!
     * @ru @brief Распарсить текст в json.
     * @param jsonString - строка текста, которую надо распарсить.
     * @return std::tuple<json_value, JsonParseResult, unsigned, unsigned> - tuple, содержащую:
     *  json_value - получившееся значение, если парсинг успешный, или UNDEFINED, в случае ошибок;
     *  JsonParseResult - код ошибки парсинга, Success в случае успеха;
     *  unsigned line, unsigned col - в случае ошибки это номера строки/колонки возникновения ошибки.
     * @en @brief Parse text to json.
     * @param jsonString - the text string to be parsed.
     * @return std::tuple<json_value, JsonParseResult, unsigned, unsigned> - tuple, содержащую:
     * json_value - the resulting value if the parsing is successful, or UNDEFINED in case of errors;
     * JsonParseResult - parsing error code, Success if successful;
     * unsigned line, unsigned col - in case of an error, these are the line/column numbers where the error occurred.
     */
    static std::tuple<json_value, JsonParseResult, unsigned, unsigned> parse(ssType jsonString);
    /*!
     * @ru @brief Сериализовать json-значение в строку.
     * @param stream - строка, в которую сохранять.
     * @param prettify - "украшать", в случае true в строке будут добавляться переносы строк и отступы.
     * @param order_keys - упорядочить ключи json-объектов. По стандарту порядок ключей в JSON не задаётся и не влияет
     *          на валидность, однако часто требуется для повторяемости результатов придерживаться одного и того же
     *          порядка вывода при разных запусках. В этом случае вывод ключей будет осуществляться упорядоченно
     *          "побайтовым сравнением".
     * @param indent_symbol - при "украшении" задаёт символ для отступов, по умолчанию пробел.
     * @param indent_count - количество символов отступа на один уровень, по умолчанию 2.
     * @en @brief Serialize json value to string.
     * @param stream - the string to save into.
     * @param prettify - "decorate", if true, line breaks and indentations will be added to the text.
     * @param order_keys - order the keys of json objects. According to the standard, the order of keys in JSON is not specified and does not affect
     * for validity, but often it is required to repeat the results to stick to the same
     * output order for different runs. In this case, the keys will be output in an orderly manner
     * "byte-by-byte comparison".
     * @param indent_symbol - when "decorating" sets the symbol for indentation, default is space.
     * @param indent_count - number of indentation characters per level, default 2.
     */
    SIMJSON_API void store(lstring<K, 0, true>& stream, bool prettify = false, bool order_keys = false, K indent_symbol = ' ', unsigned indent_count = 2) const;
    /*!
     * @ru @brief Сериализовать json-значение в строку.
     * @param prettify - "украшать", в случае true в строке будут добавляться переносы строк и отступы.
     * @param order_keys - упорядочить ключи json-объектов. По стандарту порядок ключей в JSON не задаётся и не влияет
     *          на валидность, однако часто требуется для повторяемости результатов придерживаться одного и того же
     *          порядка вывода при разных запусках. В этом случае вывод ключей будет осуществляться упорядоченно
     *          "побайтовым сравнением".
     * @param indent_symbol - при "украшении" задаёт символ для отступов, по умолчанию пробел.
     * @param indent_count - количество символов отступа на один уровень, по умолчанию 2.
     * @return строку с JSON.
     * @en @brief Serialize a json value to a string.
     * @param prettify - "decorate", if true, line breaks and indentations will be added to the line.
     * @param order_keys - order the keys of json objects. According to the standard, the order of keys in JSON is not specified and does not affect
     * for validity, but often it is required to repeat the results to stick to the same
     * output order for different runs. In this case, the keys will be output in an orderly manner
     * "byte-by-byte comparison".
     * @param indent_symbol - when "decorating" sets the symbol for indentation, default is space.
     * @param indent_count - number of indentation characters per level, default 2.
     * @return a string containing JSON.
     */
    lstring<K, 0, true> store(bool prettify = false, bool order_keys = false, K indent_symbol = ' ', unsigned indent_count = 2) const {
        lstring<K, 0, true> res;
        store(res, prettify, order_keys, indent_symbol, indent_count);
        return res;
    }

protected:
    SIMJSON_API static const json_value UNDEFINED;

    // Тип значения
    Type type_;
    // Хранимое значение
    union Value {
        Value() : boolean(false){}
        ~Value(){}
        bool boolean;
        int64_t integer;
        double real;
        strType text;
        json_object object;
        json_array array;
    } val_;
};

/*!
 * @ru @brief Парсер текста в JsonValue. Позволяет парсить JSON порциями текста.
 *  Например, данные приходят пакетами из сети, скармливаем их в processChunk, пока не получим результат
 * @tparam K - тип символов.
 * @en @brief Parser for text in JsonValue. Allows you to parse JSON in chunks of text.
 * For example, data comes in packets from the network, feed them to processChunk until we get the result
 * @tparam K - character type.
 */
template<typename K>
struct StreamedJsonParser : StreamedJsonParserBase {

    JsonValueTempl<K> result_;

    using strType = typename JsonValueTempl<K>::strType;
    using ssType = typename JsonValueTempl<K>::ssType;

    void reset() {
        this->~StreamedJsonParser<K>();
        new (this) StreamedJsonParser<K>;
    }
    /*!
     * @ru @brief Распарсить весь текст за один раз.
     * @param text.
     * @return JsonParseResult.
     * @en @brief Parse all the text in one go.
     * @param text.
     * @return JsonParseResult.
     */
    JsonParseResult parseAll(ssType text) {
        return process<true, true>(text);
    }
    /*!
     * @ru @brief Распарсить порцию текста.
     * @param chunk - порция текста.
     * @param last - признак, что это последняя порция текста. В зависимости от этого может возвращать Success или NoNeedMore.
     * @return JsonParseResult.
     * @en @brief Parse a piece of text.
     * @param chunk - a portion of text.
     * @param last - a sign that this is the last portion of text. Depending on this, it can return Success or NoNeedMore.
     * @return JsonParseResult.
     */
    JsonParseResult processChunk(ssType chunk, bool last) {
        return last ? process<false, true>(chunk) : process<false, false>(chunk);
    }

protected:

    template<bool All, bool Last>
    SIMJSON_API JsonParseResult process(ssType chunk);

    static bool isWhiteSpace(K symbol) {
        return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
    }

    strType getText();
    bool processUnicode(K symbol);

    template<bool Compound, int NewState, typename ... Args>
    JsonValueTempl<K>* addValue(JsonValueTempl<K>* current, Args&& ... args);

    JsonValueTempl<K>* popStack();

    template<bool asInt, bool All>
    JsonValueTempl<K>* addNumber(JsonValueTempl<K>* current);

    const K* ptr_{};
    const K* startProcess_ {};
    std::vector<JsonValueTempl<K>*> stack_{&result_};
    chunked_string_builder<K> text_{512};
};

template<typename K>
std::tuple<JsonValueTempl<K>, JsonParseResult, unsigned, unsigned> JsonValueTempl<K>::parse(ssType jsonString) {
    StreamedJsonParser<K> parser;
    auto res = parser.parseAll(jsonString);
    return {std::move(parser.result_), res, parser.line_, parser.col_};
}

/// @ru Алиас для JsonValue с символами u8s.
/// @en Alias ​​for JsonValue with u8s characters.
using JsonValue = JsonValueTempl<u8s>;
/// @ru Алиас для JsonValue с символами uws.
/// @en Alias ​​for JsonValue with uws symbols.
using JsonValueW = JsonValueTempl<uws>;
/// @ru Алиас для JsonValue с символами u16s.
/// @en Alias ​​for JsonValue with u16s characters.
using JsonValueU = JsonValueTempl<u16s>;
/// @ru Алиас для JsonValue с символами u32s.
/// @en Alias ​​for JsonValue with u32s characters.
using JsonValueUU = JsonValueTempl<u32s>;

/// @ru Один объект "пустышка".
/// @ru One dummy object.
#if defined (SIMJSON_EXPORT) || !defined (SIMJSON_IN_SHARED)
template<typename K>
inline SIMJSON_API const JsonValueTempl<K> JsonValueTempl<K>::UNDEFINED;
#else
#endif

/*!
 * @ru @brief Прочитать файл в строку.
 * @param filePath.
 * @return stringa.
 * @en @brief Read the file into a line.
 * @param filePath.
 * @return stringa.
 */
SIMJSON_API stringa get_file_content(stra filePath);

} // namespace simjson
