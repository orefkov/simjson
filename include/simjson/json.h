#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <simstr/sstring.h>
#include <cassert>
#include <type_traits>
#include <utility>

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

/*!
 * @brief Класс для представления json значения
 * @tparam K - тип символов
 */
template<typename K>
class JsonValueTempl : public Json {
public:

    using strType = sstring<K>;
    using ssType = simple_str<K>;

    using json_value = JsonValueTempl<K>;
    using json_object = std::shared_ptr<hashStrMap<K, JsonValueTempl<K>>>;
    using json_array = std::shared_ptr<std::vector<JsonValueTempl<K>>>;
    using obj_type = typename json_object::element_type;
    using arr_type = typename json_array::element_type;

    /// Создает пустой объект с типом Undefined
    JsonValueTempl() : type_(Undefined) {}
    /// Конструктор копирования. Объекты и массивы копируются по ссылке
    JsonValueTempl(const JsonValueTempl& other);
    /// Конструктор перемещения
    JsonValueTempl(JsonValueTempl&& other) noexcept {
        // Расчёт на то, что shared_ptr не содержит указателей на свои внутренности
        memcpy((void*)this, &other, sizeof(*this));
        other.type_ = Undefined;
    }
    /// Деструктор
    ~JsonValueTempl();

    /// Конструктор из int8_t
    JsonValueTempl(int8_t v) : type_(Integer) { val_.integer = v; }
    /// Конструктор из int16_t
    JsonValueTempl(int16_t v) : type_(Integer) { val_.integer = v; }
    /// Конструктор из int32_t
    JsonValueTempl(int32_t v) : type_(Integer) { val_.integer = v; }
    /// Конструктор из int64_t
    JsonValueTempl(int64_t v) : type_(Integer) { val_.integer = v; }
    /// Конструктор из bool
    JsonValueTempl(bool v) : type_(Boolean) { val_.boolean = v; }
    /// Конструктор из double
    JsonValueTempl(double v) : type_(Real) { val_.real = v; }

    /// Конструктор из sstring
    JsonValueTempl(strType t) : type_(Text) {
        new (&val_.text) strType(std::move(t));
    }
    /// Конструктор из остальных строк
    JsonValueTempl(ssType t) : type_(Text) {
        new (&val_.text) strType(t);
    }
    /// Конструктор из строковых выражений
    template<StrExprForType<K> A>
    JsonValueTempl(const A& e) : type_(Text) {
        new (&val_.text) strType(e);
    }
    /// Конструктор из строковых литералов
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    JsonValueTempl(T&& str) : type_(Text) {
        new (&val_.text) strType(std::forward<T>(str));
    }
    /// Конструктор для создания json::null
    JsonValueTempl(const null_t&) : type_(Null) {}
    /// Конструктор для создания пустой строки
    JsonValueTempl(const emptyString_t&) : type_(Text) { new (&val_.text) strType; }
    /// Конструктор для создания json::object
    JsonValueTempl(const emptyObject_t&) : type_(Object) {
        new (&val_.object) json_object(std::make_shared<obj_type>());
    }
    /// Конструктор для создания json::array
    JsonValueTempl(const emptyArray_t&) : type_(Array) {
        new (&val_.array) json_array(std::make_shared<arr_type>());
    }
    /// Конструктор для создания дефолтного значения с типом type
    JsonValueTempl(Type type);

    struct KeyInit : std::pair<const StoreType<K>, json_value> {
        using base = std::pair<const StoreType<K>, json_value>;
        KeyInit(const StoreType<K>& k, const json_value& v) : base(k, v){}
        KeyInit(ssType k, const json_value& v) : base(obj_type::toStoreType(k), v){}
    };
    using ObjectInit = std::initializer_list<KeyInit>;
    /// Конструктор из инициализатора для создания json::object: JsonValue v = {{"key1"_h, 1}, {"key2", "text"}, {"key3"_h, false}}; -> {"key1": 10, "key2": "text", "key3": false}
    JsonValueTempl(ObjectInit&& init) : type_(Object) {
        new (&val_.object) json_object(std::make_shared<obj_type>(std::move(reinterpret_cast<std::initializer_list<std::pair<const StoreType<K>, json_value>>&>(init))));
    }
    using ArrayInit = std::initializer_list<json_value>;
    /// Конструктор из инициализатора для создания json::array: JsonValue json = {"key1", 12, false, Json::null, 1.25, true}; -> ["key1", 12, false, null, 1.25, true]
    JsonValueTempl(ArrayInit&& init) : type_(Array) {
        new (&val_.array) json_array(std::make_shared<arr_type>(std::move(init)));
    }

    struct Clone {
        const json_value& from;
    };
    /// Конструктор клонирования. В этом случае для объектов и массивов создаются "глубокие" копии
    JsonValueTempl(const Clone& clone);
    /// Получить тип значения
    Type type() const { return type_; }
    /// Клонировать значение
    json_value clone() const {
        return Clone{*this};
    }

    // Проверка типов
    /// Значение undefined
    bool is_undefined() const { return type_ == Undefined; };
    /// Значение null
    bool is_null() const { return type_ == Null; };
    /// Значение boolean
    bool is_boolean() const { return type_ == Boolean; }
    /// Значение integer
    bool is_integer() const { return type_ == Integer; }
    /// Значение real (double)
    bool is_real() const { return type_ == Real; }
    /// Значение text
    bool is_text() const { return type_ == Text; }
    /// Значение array
    bool is_array() const { return type_ == Array; }
    /// Значение object
    bool is_object() const { return type_ == Object; }


    // -----------------------------   Boolean -----------------------------------------------

    /// Получить значение как boolean. В отладочной версии проверяется, что значение действительно boolean
    bool as_boolean() const {
        assert(type_ == Boolean);
        return val_.boolean;
    }
    /// Получить boolean, если хранится boolean, или ничего. Пример: bool val = json.boolean().value_or(false);
    std::optional<bool> boolean() const {
        if (type_ == Boolean) {
            return val_.boolean;
        }
        return {};
    }
    /// Получить boolean, если хранится boolean, или выкинуть исключение. Пример: bool val = json.boolean_or_throw<std::runtime_error>("Need boolean value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    bool boolean_or_throw(Args&&...args) const {
        if (type_ == Boolean) {
            return val_.boolean;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// Получить значение, конвертированное в boolean. Логика работы как в javascript `!!val`. Пример: bool val = json.to_boolean();
    bool to_boolean() const;

    // ---------------------------------------- Integer -------------------------------------
    /// Получить значение как integer. В отладочной версии проверяется, что значение действительно integer
    int64_t as_integer() const {
        assert(type_ == Integer);
        return val_.integer;
    }
    /// Получить integer, если хранится integer, или ничего. Пример: auto val = json.integer().value_or(10);
    std::optional<int64_t> integer() const {
        if (type_ == Integer) {
            return val_.integer;
        }
        return {};
    }
    /// Получить integer, если хранится integer, или выкинуть исключение. Пример: int val = json.integer_or_throw<std::runtime_error>("Need integer value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t integer_or_throw(Args&&...args) const {
        if (type_ == Integer) {
            return val_.integer;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// Получить значение, конвертированное в integer, или ничего. Логика работы как в javascript `1 * val`.
    /// Если в результате NaN или Inf, то ничего. Нецелые числа - переводятся в целое.
    /// Пример: int val = json.to_integer().value_or(10);
    std::optional<int64_t> to_integer() const;
    /// Получить значение из to_integer, если оно есть, или выкинуть исключение.
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t to_integer_or_throw(Args&&...args) const {
        if (auto val = to_integer()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    // -------------------------------------- Real --------------------------------------------
    /// Получить значение как double. В отладочной версии проверяется, что значение действительно double
    double as_real() const {
        assert(type_ == Real);
        return val_.real;
    }
    /// Получить double, если хранится double, или ничего. Пример: auto val = json.real().value_or(10.0);
    std::optional<double> real() const {
        if (type_ == Real) {
            return val_.real;
        }
        return {};
    }
    /// Получить double, если хранится double, или выкинуть исключение. Пример: double val = json.real_or_throw<std::runtime_error>("Need double value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    double real_or_throw(Args&&...args) {
        if (type_ == Real) {
            return val_.real;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    /// Получить значение, конвертированное в double. Логика работы как в javascript `1 * val`. Для "не чисел" возвращает NaN.
    double to_real() const;
    // --------------------------------------- Number -----------------------------
    // В нашей реализации в отличии от чистого json числа могут быть int64_t или double
    // Поэтому добавим ещё логику для работы только с обоими вариантами чисел

    /// Возвращает int64_t, если хранится int64_t или double влезающий в int64_t, либо ничего
    std::optional<int64_t> number_int() const;
    /// Получить int64_t, если хранится int64_t или double влезающий в int64_t, либо выбросить исключение
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    int64_t number_int_or_throw(Args&&...args) const {
        if (auto val = number_int()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// Возвращает double, если хранится double или int64_t, либо ничего
    std::optional<double> number_real() const;
    /// Получить double, если хранится double или int64_t, либо выбросить исключение
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    double number_real_or_throw(Args&&...args) const {
        if (auto val = number_real()) {
            return *val;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    // --------------------------------------- Text -----------------------------

    /// Получить значение как текст. В отладочной версии проверяется, что значение действительно текст
    const strType& as_text() const {
        assert(type_ == Text);
        return val_.text;
    }

    /// Получить текст, если хранится текст, или ничего. Пример: auto val = json.text().value_or("default value");
    std::optional<strType> text() const {
        if (type_ == Text) {
            return val_.text;
        }
        return {};
    }
    /// Получить текст, если хранится текст, или выкинуть исключение. Пример: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    const strType& text_or_throw(Args&&...args) const {
        if (type_ == Text) {
            return val_.text;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /// Получить текст, если хранится текст и он не пустой, или выкинуть исключение. Пример: auto val = json.text_or_throw<std::runtime_error>("Need text value");
    template<typename Exc, typename ... Args> requires (std::is_constructible_v<Exc, Args...>)
    const strType& not_empty_text_or_throw(Args&&...args) const {
        if (type_ == Text && !val_.text.template trimmed<ssType>().is_empty()) {
            return val_.text;
        }
        throw Exc(std::forward<Args>(args)...);
    }

    /// Получить значение, конвертированное в текст. Логика работы как в javascript `"" + val`.
    strType to_text() const;

    /// Получить значение как json Object. В отладочной версии проверяется, что значение действительно Object
    json_object& as_object() {
        assert(type_ == Object);
        return val_.object;
    }
    /// Получить значение как json Object. В отладочной версии проверяется, что значение действительно Object
    const json_object& as_object() const {
        assert(type_ == Object);
        return val_.object;
    }
    /// Получить значение как json Array. В отладочной версии проверяется, что значение действительно Array
    json_array& as_array() {
        assert(type_ == Array);
        return val_.array;
    }
    /// Получить значение как json Array. В отладочной версии проверяется, что значение действительно Array
    const json_array& as_array() const {
        assert(type_ == Array);
        return val_.array;
    }
    /// Обменять значения
    void swap(json_value& other) noexcept {
        char tmp[sizeof(*this)];
        memcpy(tmp, this, sizeof(*this));
        memcpy((void*)this, &other, sizeof(*this));
        memcpy((void*)&other, tmp, sizeof(*this));
    }

    /// Присвоение
    JsonValueTempl& operator=(json_value t) noexcept {
        swap(t);
        return *this;
    }

    /// Обращение к свойству константного объекта по ключу. Не создаёт новую пару при отсутствии ключа
    template<typename T> requires (!std::is_integral_v<T>)
    const json_value& at(T&& key) const {
        if (type_ == Object) {
            if (auto it = as_object()->find(std::forward<T>(key)); it != as_object()->end())
                return it->second;
        }
        return UNDEFINED;
    }

    /// Обращение к свойству константного объекта по ключу. Не создаёт новую пару при отсутствии ключа
    template<typename T> requires (!std::is_integral_v<T>)
    const json_value& operator[](T&& key) const {
        return at(std::forward<T>(key));
    }

    /// Обращение к свойству константного объекта по ключам. Не создаёт новую пару при отсутствии ключа
    template<typename T, typename...Args> requires (!std::is_integral_v<T>)
    const json_value& operator()(T&& key, Args&&...args) const {
        const json_value& res = at(std::forward<T>(key));
        if constexpr (sizeof...(Args) == 0) {
            return res;
        } else {
            return res.is_undefined() ? res : res(std::forward<Args>(args)...);
        }
    }

    /// Обращение к свойству объекта по ключу. Создаёт новую пару при отсутствии ключа
    /// Если значение не объект, заменят его на объект
    template<typename T> requires (!std::is_integral_v<T>)
    json_value& operator[](T&& key) {
        if (type_ != Object) {
            assert(this != &UNDEFINED);
            *this = emptyObject;
        }
        return as_object()->try_emplace(std::forward<T>(key)).first->second;
    }

    /// Установка значения свойству объекта по ключу. Создаёт новую пару при отсутствии ключа
    /// Если значение не объект, заменят его на объект
    template<typename Key, typename ... Args> requires (!std::is_integral_v<Key>)
    json_value& set(Key&& key, Args&& ... args) {
        if (type_ != Object) {
            assert(this != &UNDEFINED);
            *this = emptyObject;
        }
        return as_object()->emplace(std::forward<Key>(key), std::forward<Args>(args)...).first->second;
    }

    /// Обращение к элементу константного массива по индексу.
    const json_value& at(size_t idx) const {
        if (type_ == Array) {
            const auto& arr = *as_array();
            if (idx < arr.size()) {
                return arr[idx];
            }
        }
        return UNDEFINED;
    }
    /// Обращение к элементу константного массива по индексу.
    const json_value& operator[](size_t idx) const {
        return at(idx);
    }

    /// Обращение к элементу массива по индексу.
    /// Если значение не массив - заменяется на массив.
    /// Если индекс больше длины массива - увеличивает массив до заданного индекса
    /// Если индекс == -1 - добавляет в массив ещё один элемент
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
    /// Количество элементов массива или ключей объекта
    size_t size() const {
        if (type_ == Array) {
            return as_array()->size();
        } else if (type_ == Object) {
            return as_object()->size();
        }
        return 0;
    }

    /// Слияние с другим JSON. Если replace == true, то другой JSON имеет приоритет, если он не undefined.
    /// Если оба объекта массивы, то другой массив дописывается к этому массиву.
    /// Если оба JSONа объекты - то сливаются по ключам.
    /// Иначе другой JSON заменяет текущее значение.
    void merge(const json_value& other, bool replace = true);

    /// Распарсить текст с json.
    static std::tuple<json_value, JsonParseResult, unsigned, unsigned> parse(ssType jsonString);

    /// Сериализовать значение в lstring<K, 0>. При prettify == true - добавляет переносы строк
    /// и отступы в указанное количество указанных символов, по умолчанию - два пробела.
    /// При order_keys == true - сортирует ключи объектов по их имени
    void store(lstring<K, 0, true>& stream, bool prettify = false, bool order_keys = false, K indent_symbol = ' ', unsigned indent_count = 2) const;

    /// Сериализовать значение в lstring<K, 0>. При prettify == true - добавляет переносы строк
    /// и отступы в указанное количество указанных символов, по умолчанию - два пробела.
    /// При order_keys == true - сортирует ключи объектов по их имени
    lstring<K, 0, true> store(bool prettify = false, bool order_keys = false, K indent_symbol = ' ', unsigned indent_count = 2) const {
        lstring<K, 0, true> res;
        store(res, prettify, order_keys, indent_symbol, indent_count);
        return res;
    }

protected:
    // Один объект "пустышка"
    static const json_value UNDEFINED;
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
 * @brief Парсер текста в JsonValue. Позволяет парсить JSON порциями текста.
 *  Например, данные приходят пакетами из сети, скармливаем их в processChunk, пока не получим результат
 * @tparam K - тип символов
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
     * @brief Распарсить весь текст за один раз
     * @param text
     * @return JsonParseResult
     */
    JsonParseResult parseAll(ssType text) {
        return process<true, true>(text);
    }
    /*!
     * @brief Распарсить порцию текста
     * @param chunk - порция текста
     * @param last - признак, что это последняя порция текста. В зависимости от этого может возвращать Success или NoNeedMore.
     * @return JsonParseResult
     */
    JsonParseResult processChunk(ssType chunk, bool last) {
        return last ? process<false, true>(chunk) : process<false, false>(chunk);
    }

protected:

    template<bool All, bool Last>
    JsonParseResult process(ssType chunk);

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

using JsonValue = JsonValueTempl<u8s>;
using JsonValueW = JsonValueTempl<uws>;
using JsonValueU = JsonValueTempl<u16s>;
using JsonValueUU = JsonValueTempl<u32s>;

// Один объект "пустышка"
template<typename K>
inline const JsonValueTempl<K> JsonValueTempl<K>::UNDEFINED;
/*!
 * @brief Прочитать файл в строку
 * @param filePath
 * @return stringa
 */
stringa get_file_content(stra filePath);

} // namespace simjson
