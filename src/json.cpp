#include <simjson/json.h>
#include <cmath>
#include <algorithm>
#include <fstream>

namespace simjson {
using namespace simstr;
using namespace simstr::literals;

template<typename K>
struct expr_json_str {
    using symb_type = K;
    using ssType = typename JsonValueTempl<K>::ssType;
    ssType text;
    size_t l;

    size_t length() const noexcept {
        return l;
    }

    explicit expr_json_str(ssType t) : text(t) {
        const K* ptr = text.symbols();
        size_t add = 0;
        for (size_t i = text.length(); i-- ;) {
            K s = *ptr++;
            switch (s) {
            case '\b':
            case '\f':
            case '\r':
            case '\n':
            case '\t':
            case '\"':
            case '\\':
                add++;
                break;
            default:
                if (s < ' ') {
                    add += 5; // \u0001
                }
            }
        }
        l = text.length() + add;
    }

    K* place(K* ptr) const noexcept{
        static constexpr ssType repl[] = {
            uni_string(K, "\\u0000"),
            uni_string(K, "\\u0001"),
            uni_string(K, "\\u0002"),
            uni_string(K, "\\u0003"),
            uni_string(K, "\\u0004"),
            uni_string(K, "\\u0005"),
            uni_string(K, "\\u0006"),
            uni_string(K, "\\u0007"),
            uni_string(K, "\\b"),
            uni_string(K, "\\t"),
            uni_string(K, "\\n"),
            uni_string(K, "\\u000B"),
            uni_string(K, "\\f"),
            uni_string(K, "\\r"),
            uni_string(K, "\\u000E"),
            uni_string(K, "\\u000F"),
            uni_string(K, "\\u0010"),
            uni_string(K, "\\u0011"),
            uni_string(K, "\\u0012"),
            uni_string(K, "\\u0013"),
            uni_string(K, "\\u0014"),
            uni_string(K, "\\u0015"),
            uni_string(K, "\\u0016"),
            uni_string(K, "\\u0017"),
            uni_string(K, "\\u0018"),
            uni_string(K, "\\u0019"),
            uni_string(K, "\\u001A"),
            uni_string(K, "\\u001B"),
            uni_string(K, "\\u001C"),
            uni_string(K, "\\u001D"),
            uni_string(K, "\\u001E"),
            uni_string(K, "\\u001F"),
        };
        const K* r = text.symbols();
        size_t lenOfText = text.length(), lenOfTail = l - lenOfText;
        while (lenOfTail) {
            K s = *r++;
            switch (s) {
            case '\"':
                *ptr++ = '\\';
                *ptr++ = '\"';
                lenOfTail--;
                break;
            case '\\':
                *ptr++ = '\\';
                *ptr++ = '\\';
                lenOfTail--;
                break;
            default:
                if (s < ' ') {
                    ptr = repl[s].place(ptr);
                    lenOfTail -= repl[s].len - 1;
                } else {
                    *ptr++ = s;
                }
            }
            lenOfText--;
        }
        if (lenOfText) {
            std::char_traits<K>::copy(ptr, r, lenOfText);
            ptr += lenOfText;
        }
        return ptr;
    }
};

template<typename K>
JsonValueTempl<K>::JsonValueTempl(const JsonValueTempl& other) : type_(other.type_) {
    switch (type_) {
    case Boolean:
        val_.boolean = other.val_.boolean;
        break;
    case Integer:
        val_.integer = other.val_.integer;
        break;
    case Real:
        val_.real = other.val_.real;
        break;
    case Text:
        new (&val_.text) strType(other.as_text());
        break;
    case Object:
        new (&val_.object) json_object(other.as_object()); // копируем shared_ptr на объект
        break;
    case Array:
        new (&val_.array) json_array(other.as_array());    // копируем shared_ptr на массив
        break;
    default:
        break;
    }
}

template<typename K>
JsonValueTempl<K>::~JsonValueTempl() {
    switch (type_) {
    case Text:
        as_text().~strType();
        break;
    case Object:
        as_object().~json_object();
        break;
    case Array:
        as_array().~json_array();
        break;
    default:
        break;
    }
}

template<typename K>
JsonValueTempl<K>::JsonValueTempl(Type type) : type_(type) {
    switch (type_) {
    case Boolean:
        val_.boolean = false;
        break;
    case Text:
        new (&val_.text) strType;
        break;
    case Integer:
        val_.integer = 0;
        break;
    case Real:
        val_.real = 0.0;
        break;
    case Object:
        new (&val_.object) json_object(std::make_shared<obj_type>());
        break;
    case Array:
        new (&val_.array) json_array(std::make_shared<arr_type>());
        break;
    default:
        break;
    }
}

template<typename K>
JsonValueTempl<K>::JsonValueTempl(const Clone& clone) : type_(clone.from.type_) {
    const json_value& other = clone.from;
    switch (type_) {
    case Boolean:
        val_.boolean = other.val_.boolean;
        break;
    case Integer:
        val_.integer = other.val_.integer;
        break;
    case Real:
        val_.real = other.val_.real;
        break;
    case Text:
        new (&val_.text) strType(other.as_text());
        break;
    case Object:
        new (&val_.object) json_object(std::make_shared<obj_type>(*other.as_object()));
        break;
    case Array:
        new (&val_.array) json_array(std::make_shared<arr_type>(*other.as_array()));
        break;
    default:
        break;
    }
}

template<typename K>
bool JsonValueTempl<K>::to_boolean() const {
    switch (type_) {
        case Boolean:
            return val_.boolean;
        case Text:
            return !as_text().is_empty();
        case Integer:
            return val_.integer != 0;
        case Real:
            return val_.real != 0.0;
        case Object:
        case Array:
            return true;
        default:
            return false;
    }
}

inline static bool is_double_int64(double dbl) {
    return
        !std::isnan(dbl) &&
        !std::isinf(dbl) &&
        // std::numeric_limits<int64_t>::max() при конвертировании в double меняет 9223372036854775807 на 9223372036854775808
        dbl <= (uint64_t(1) << 53) &&
        dbl >= -(int64_t)(uint64_t(1) << 53);
}

template<typename K>
std::optional<int64_t> JsonValueTempl<K>::to_integer() const {
    switch (type_) {
    case Boolean:
        return val_.boolean ? 1 : 0;
    case Text: {
        auto txt = as_text().template trimmed_right<ssType>();
        auto [res, err, processed] = txt.template to_int<int64_t>();
        if (err == IntConvertResult::Success) {
            return res;
        }
        if (processed > 0 && err == IntConvertResult::BadSymbolAtTail &&
            (txt[processed] == '.' || txt[processed] == 'e' || txt[processed] == 'E')) {
            // Считаем, что в тексте double
            auto dbl = std::nan("");
            if constexpr (is_one_of_std_char_v<K>) {
                dbl = txt.to_double();
            } else {
                dbl = lstringa<100>{txt}.to_double();
            }
            if (is_double_int64(dbl)) {
                return static_cast<int64_t>(dbl);
            }
        }
        break;
    }
    case Integer:
        return val_.integer;
    case Real:
        if (is_double_int64(val_.real)) {
            return static_cast<int64_t>(val_.real);
        }
        break;
    case Array:
        // Для массивов в javascript если размер массива равен 0, то 0, если 1, то берётся значение из первого элемента, иначе NaN
        if (as_array()->size() == 0) {
            return 0;
        } else if (as_array()->size() == 1) {
            return (*as_array())[0].to_integer();
        }
        break;
    default:
        ;
    }
    return {};
}

template<typename K>
double JsonValueTempl<K>::to_real() const {
    switch (type_) {
    case Json::Boolean:
        return val_.boolean ? 1.0 : 0.0;
    case Json::Text:
        if constexpr (is_one_of_std_char_v<K>) {
            return val_.text.to_double();
        } else {
            return lstringa<100>{val_.text}.to_double();
        }
    case Json::Integer:
        return static_cast<double>(val_.integer);
    case Json::Real:
        return val_.real;
    default:
        return std::nan("");
    }
}

template<typename K>
std::optional<int64_t> JsonValueTempl<K>::number_int() const {
    if (type_ == Integer) {
        return val_.integer;
    }
    if (type_ == Real && is_double_int64(val_.real)) {
        return static_cast<int64_t>(val_.real);
    }
    return {};
}

template<typename K>
std::optional<double> JsonValueTempl<K>::number_real() const {
    if (type_ == Real) {
        return val_.real;
    }
    if (type_ == Integer) {
        return static_cast<double>(val_.integer);
    }
    return {};
}

template<typename K>
typename JsonValueTempl<K>::strType JsonValueTempl<K>::to_text() const {
    switch (type_) {
    case Undefined:
        return uni_string(K, "undefined");
    case Null:
        return uni_string(K, "null");
    case Boolean:
        return val_.boolean ? uni_string(K, "true") :  uni_string(K, "false");
    case Object:
        return uni_string(K, "[object Object]");
    case Array:
    {
        std::vector<strType> res;
        res.reserve(as_array()->size());
        for (const auto& e: *as_array())
            res.emplace_back(e.to_text());
        if constexpr(std::is_same_v<K, u8s>) {
            return e_join(res, ",");
        }
        if constexpr(std::is_same_v<K, u16s>) {
            return e_join(res, u",");
        }
        if constexpr(std::is_same_v<K, u32s>) {
            return e_join(res, U",");
        }
        if constexpr(std::is_same_v<K, wchar_t>) {
            return e_join(res, L",");
        }
    }
    case Text:
        return as_text();
    case Integer:
        return e_num<K>(val_.integer);
    case Real:
        if constexpr (is_one_of_std_char_v<K>) {
            return e_real<K>(val_.real);
        } else {
            return lstringa<64>{e_real<u8s>(val_.real)};
        }
    default:
        return {};
    }
}

template<typename K>
void JsonValueTempl<K>::merge(const json_value& other, bool replace) {
    if (is_object() && other.is_object()) {
        // Надо слить по ключам
        auto& self = *as_object();
        for (const auto& [key, value]: *other.as_object()) {
            auto fnd = self.find(key);
            if (fnd != self.end()) {
                fnd->second.merge(value, replace);
            } else {
                self.try_emplace(key, value);
            }
        }
    } else if (is_array() && other.is_array()) {
        if (other.as_array()->size()) {
            auto& arr = *as_array();
            arr.reserve(arr.size() + other.as_array()->size());
            for (const auto& e : *other.as_array()) {
                arr.emplace_back(e);
            }
        }
    } else if (replace && !other.is_undefined()) {
        *this = other;
    }
}

template<typename K>
struct json_store {
    lstring<K, 0, true>& buffer;
    bool prettify;
    bool order_keys;
    K indent_symb;
    unsigned indent_count;

    void store(const JsonValueTempl<K>& json, unsigned indent) {
        bool printed = false;
        switch (json.type()) {
        case Json::Undefined:
            break;
        case Json::Null:
            buffer += uni_string(K, "null");
            break;
        case Json::Boolean:
            if (json.as_boolean())
                buffer += uni_string(K, "true");
            else
                buffer += uni_string(K, "false");
            break;
        case Json::Integer:
            buffer += e_num<K>(json.as_integer());
            break;
        case Json::Real:
            buffer += json.to_text();
            break;
        case Json::Text:
            buffer += uni_string(K, "\"") + expr_json_str<K>{ json.as_text() } + uni_string(K, "\"");
            break;
        case Json::Object:
            buffer += uni_string(K, "{");
            if (order_keys && json.as_object()->size() > 1) {
                std::vector<typename JsonValueTempl<K>::obj_type::iterator> keys;
                keys.reserve(json.as_object()->size());
                for (auto it = json.as_object()->begin(), e = json.as_object()->end(); it != e; ++it) {
                    keys.emplace_back(it);
                }
                std::sort(keys.begin(), keys.end(), [](const auto& s1, const auto& s2) {
                    return s1->first.str < s2->first.str;
                });
                for (const auto& it : keys) {
                    if (it->second.type() != Json::Undefined) {
                        buffer +=
                            e_c(printed ? 1 : 0, K(',')) +
                            e_if(prettify, uni_string(K, "\n") + e_c(indent, indent_symb)) +
                            uni_string(K, "\"") +
                            expr_json_str<K>{ it->first.to_str() } +
                            e_choice(prettify, uni_string(K, "\": "), uni_string(K, "\":"));
                        printed = true;
                        store(it->second, indent + indent_count);
                    }
                }
            } else {
                for (const auto& it : *json.as_object()) {
                    if (it.second.type() != Json::Undefined) {
                        buffer +=
                            e_c(printed ? 1 : 0, K(',')) +
                            e_if(prettify, uni_string(K, "\n") + e_c(indent, indent_symb)) +
                            uni_string(K, "\"") +
                            expr_json_str<K>{ it.first.to_str() } +
                            e_choice(prettify, uni_string(K, "\": "), uni_string(K, "\":"));
                        printed = true;
                        store(it.second, indent + indent_count);
                    }
                }
            }
            buffer += e_if(prettify && printed, uni_string(K, "\n") + e_c(indent - indent_count, indent_symb)) + uni_string(K, "}");
            break;
        case Json::Array:
            buffer += uni_string(K, "[");
            for (const auto& it : *json.as_array()) {
                buffer += e_if(printed, e_c(1, K(','))) + e_if(prettify, uni_string(K, "\n") + e_c(indent, indent_symb));
                store(it, indent + indent_count);
                printed = true;
            }
            buffer += e_if(prettify && printed, uni_string(K, "\n") + e_c(indent - indent_count, indent_symb)) + uni_string(K, "]");
            break;
        }
    }
};

template<typename K>
void JsonValueTempl<K>::store(lstring<K, 0, true>& stream, bool prettify, bool order_keys, K indent_symbol, unsigned indent_count) const {
    json_store<K>{stream, prettify, order_keys, indent_symbol, indent_count}.store(*this, indent_count);
}

enum States {
    WaitValue,
    Done,
    WaitKey,
    WaitColon,
    WaitComma,
    ProcessT,
    ProcessTr,
    ProcessTru,
    ProcessF,
    ProcessFa,
    ProcessFal,
    ProcessFals,
    ProcessN,
    ProcessNu,
    ProcessNul,
    ProcessString,
    ProcessStringSlash,
    ProcessStringSlashU,
    ProcessStringSlashU1,
    ProcessStringSlashU2,
    ProcessStringSlashU3,
    ProcessStringSlashU4,
    ProcessStringSlashU4Slash,
    ProcessStringSlashU4SlashU,
    ProcessStringSlashU4SlashU1,
    ProcessStringSlashU4SlashU2,
    ProcessStringSlashU4SlashU3,
    ProcessNumber,
    ProcessNumberSign,
    ProcessNumberZero,
    ProcessNumberDot,
    ProcessNumberDotNumber,
    ProcessNumberDotNumberExp,
    ProcessNumberDotNumberExpSign,
    ProcessNumberDotNumberExpSignNumber,
};

enum StartSymbols {
    ErrorSymbol,
    Object,
    Array,
    True,
    False,
    Null,
    String,
    Number,
    Zero,
    NegateNumber
};

inline static const char START_SYMBOLS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    String,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    NegateNumber,
    0, 0,
    Zero,
    Number,
    Number,
    Number,
    Number,
    Number,
    Number,
    Number,
    Number,
    Number,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Array,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    False,
    0, 0, 0, 0, 0, 0, 0,
    Null,
    0, 0, 0, 0, 0,
    True,
    0, 0, 0, 0, 0, 0,
    Object
};

template<typename K>
template<bool All, bool Last>
JsonParseResult StreamedJsonParser<K>::process(ssType chunk) {
    ptr_ = chunk.begin();
    const K* end = chunk.end();
    JsonValueTempl<K>* current = stack_.empty() ? nullptr : stack_.back();

    for (; ptr_ < end ; ptr_++) {
        K symbol = *ptr_;
        if (symbol == '\n') {
            line_++;
            col_ = 0;
        } else {
            col_++;
        }

      processSymbol:
        switch (state_) {
        case WaitValue:
        case WaitKey:
        case WaitColon:
        case WaitComma:
        case Done:
            if (isWhiteSpace(symbol)) {
                continue;
            }
            break;
        }

        if (state_ == Done || !current) {
            break;
        }
        switch (state_) {
        case WaitValue:
            if (std::make_unsigned_t<K>(symbol) > sizeof(START_SYMBOLS)) {
                return JsonParseResult::Error;
            }
            switch(START_SYMBOLS[(size_t)symbol]) {
            case ErrorSymbol:
                if (symbol == ']' && current->is_array() && !current->as_array()->size()) {
                    state_ = WaitComma;
                    current = popStack();
                    break;
                }
                return JsonParseResult::Error;
            case Object:
                current = addValue<true, WaitKey>(current, Json::emptyObject);
                break;
            case Array:
                current = addValue<true, WaitValue>(current, Json::emptyArray);
                break;
            case True:
                state_ = ProcessT;
                break;
            case False:
                state_ = ProcessF;
                break;
            case Null:
                state_ = ProcessN;
                break;
            case String:
                state_ = ProcessString;
                startProcess_ = ptr_;
                break;
            case Number:
                state_ = ProcessNumber;
                startProcess_ = ptr_;
                break;
            case Zero:
                state_ = ProcessNumberZero;
                startProcess_ = ptr_;
                break;
            case NegateNumber:
                state_ = ProcessNumberSign;
                startProcess_ = ptr_;
                break;
            }
            break;
        case WaitKey:
            if (symbol == '\"') {
                state_ = ProcessString;
                startProcess_ = ptr_;
            } else if (symbol == '}') {
                state_ = WaitComma;
                current = popStack();
            } else {
                return JsonParseResult::Error;
            }
            break;
        case WaitColon:
            if (symbol == ':') {
                state_ = WaitValue;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case WaitComma:
            if (symbol == ',') {
                state_ = current->is_object() ? WaitKey : WaitValue;
            } else if ((symbol == '}' && current->is_object()) || (symbol == ']' && current->is_array())) {
                current = popStack();
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessT:
            if (symbol == 'r') {
                state_ = ProcessTr;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessTr:
            if (symbol == 'u') {
                state_ = ProcessTru;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessTru:
            if (symbol == 'e') {
                current = addValue<false, WaitComma>(current, true);
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessF:
            if (symbol == 'a') {
                state_ = ProcessFa;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessFa:
            if (symbol == 'l') {
                state_ = ProcessFal;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessFal:
            if (symbol == 's') {
                state_ = ProcessFals;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessFals:
            if (symbol == 'e') {
                current = addValue<false, WaitComma>(current, false);
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessN:
            if (symbol == 'u') {
                state_ = ProcessNu;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessNu:
            if (symbol == 'l') {
                state_ = ProcessNul;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessNul:
            if (symbol == 'l') {
                current = addValue<false, WaitComma>(current, Json::null);
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessString:
            if (symbol == '\"') {
                // end of string, add to value
                auto value = getText();

                if (current->is_object()) {
                    // value is key name
                    const auto& [newVal, not_exist] = current->as_object()->try_emplace(std::move(value));
                    if (!not_exist) {
                        // key already exist
                        return JsonParseResult::Error;
                    }
                    current = &newVal->second;
                    stack_.push_back(current);
                    state_ = WaitColon;
                } else {
                    current = addValue<false, WaitComma>(current, std::move(value));
                }
            } else if (symbol == '\\') {
                if (startProcess_) {
                    if (ptr_ - startProcess_ > 1) {
                        text_ << typename JsonValueTempl<K>::ssType{startProcess_ + 1, size_t(ptr_ - startProcess_ - 1)};
                    }
                    startProcess_ = nullptr;
                }
                state_ = ProcessStringSlash;
            } else if (std::make_unsigned_t<K>(symbol) < ' ') {
                return JsonParseResult::Error;
            } else if (!startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessStringSlash:
            switch(symbol) {
            case '\\':
                text_ << K('\\');
                state_ = ProcessString;
                break;
            case '\"':
                text_ << K('\"');
                state_ = ProcessString;
                break;
            case '/':
                text_ << K('/');
                state_ = ProcessString;
                break;
            case 'b':
                text_ << K('\b');
                state_ = ProcessString;
                break;
            case 'f':
                text_ << K('\f');
                state_ = ProcessString;
                break;
            case 'n':
                text_ << K('\n');
                state_ = ProcessString;
                break;
            case 'r':
                text_ << K('\r');
                state_ = ProcessString;
                break;
            case 't':
                text_ << K('\t');
                state_ = ProcessString;
                break;
            case 'u':
                state_ = ProcessStringSlashU;
                idxUnicode_ = 0;
                currentUnicode_[0] = 0;
                break;
            default:
                return JsonParseResult::Error;
            }
            break;
        case ProcessStringSlashU:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU1;
            break;
        case ProcessStringSlashU1:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU2;
            break;
        case ProcessStringSlashU2:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU3;
            break;
        case ProcessStringSlashU3:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            if constexpr (sizeof(K) == 2) {
                text_ << (K)currentUnicode_[0];
            } else {
                if (currentUnicode_[0] >= 0xD800 && currentUnicode_[0] < 0xDC00) {
                    // surrogate pair
                    state_ = ProcessStringSlashU4;
                } else {
                    text_ << lstring<K, 10>{ssu{currentUnicode_, 1}};
                    state_ = ProcessString;
                }
            }
            break;
        case ProcessStringSlashU4:
            if (symbol == '\\') {
                state_ = ProcessStringSlashU4Slash;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessStringSlashU4Slash:
            if (symbol == 'u') {
                state_ = ProcessStringSlashU4SlashU;
                idxUnicode_ = 1;
                currentUnicode_[1] = 0;
            } else {
                return JsonParseResult::Error;
            }
            break;
        case ProcessStringSlashU4SlashU:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU4SlashU1;
            break;
        case ProcessStringSlashU4SlashU1:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU4SlashU2;
            break;
        case ProcessStringSlashU4SlashU2:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            state_ = ProcessStringSlashU4SlashU3;
            break;
        case ProcessStringSlashU4SlashU3:
            if (!processUnicode(symbol)) {
                return JsonParseResult::Error;
            }
            text_ << lstring<K, 10>{ssu{currentUnicode_, 2}};
            state_ = ProcessString;
            break;
        case ProcessNumber:
            if (symbol == '.') {
                state_ = ProcessNumberDot;
            } else if (symbol == 'e' || symbol == 'E') {
                state_ = ProcessNumberDotNumberExp;
            } else if (symbol < '0' || symbol > '9') {
                current = addNumber<true, Last>(current);
                goto processSymbol;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberSign:
            if (symbol >= '1' && symbol <= '9') {
                state_ = ProcessNumber;
            } else if (symbol == '0') {
                state_ = ProcessNumberZero;
            } else {
                return JsonParseResult::Error;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberZero:
            if (symbol == '.') {
                state_ = ProcessNumberDot;
            } else {
                current = addNumber<true, Last>(current);
                goto processSymbol;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberDot:
            if (symbol < '0' || symbol > '9') {
                return JsonParseResult::Error;
            }
            state_ = ProcessNumberDotNumber;
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberDotNumber:
            if (symbol == 'e' || symbol == 'E') {
                state_ = ProcessNumberDotNumberExp;
            } else if (symbol < '0' || symbol > '9') {
                current = addNumber<false, Last>(current);
                goto processSymbol;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberDotNumberExp:
            if (symbol == '-' || symbol == '+') {
                state_ = ProcessNumberDotNumberExpSign;
            } else if (symbol >= '0' && symbol <= '9') {
                state_ = ProcessNumberDotNumberExpSignNumber;
            } else {
                return JsonParseResult::Error;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberDotNumberExpSign:
            if (symbol >= '0' && symbol <= '9') {
                state_ = ProcessNumberDotNumberExpSignNumber;
            } else {
                return JsonParseResult::Error;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        case ProcessNumberDotNumberExpSignNumber:
            if (symbol < '0' || symbol > '9') {
                current = addNumber<false, Last>(current);
                goto processSymbol;
            }
            if (!All && !startProcess_) {
                text_ << symbol;
            }
            break;
        }
    }
    if constexpr (Last) {
        if (state_ == ProcessNumber || state_ == ProcessNumberZero) {
            addNumber<true, All>(current);
        } else if (state_ == ProcessNumberDotNumber || state_ == ProcessNumberDotNumberExpSignNumber) {
            addNumber<false, All>(current);
        }
    } else {
        if (startProcess_) {
            if (state_ == ProcessString) {
                startProcess_++;    // for string startProcess from \", no need copy it
            }
            if (ptr_ - startProcess_ > 0) {
                text_ << ssType{startProcess_, size_t(ptr_ - startProcess_)};
            }
            startProcess_ = nullptr;
        }
    }
    if (state_ == Done) {
        return Last && ptr_ == end ? JsonParseResult::Success : JsonParseResult::NoNeedMore;
    }
    return JsonParseResult::Pending;
}

template<typename K>
typename JsonValueTempl<K>::strType StreamedJsonParser<K>::getText() {
    if (startProcess_) {
        ssType text{startProcess_ + 1, size_t(ptr_ - startProcess_ - 1)};
        startProcess_ = nullptr;
        return text;
    }
    strType text(text_);
    text_.reset();
    return text;
}

template<typename K>
bool StreamedJsonParser<K>::processUnicode(K symbol) {
    if (symbol >= '0' && symbol <= '9') {
        currentUnicode_[idxUnicode_] = currentUnicode_[idxUnicode_] * 16 + symbol - '0';
    } else if (symbol >= 'a' && symbol <= 'f') {
        currentUnicode_[idxUnicode_] = currentUnicode_[idxUnicode_] * 16 + symbol + 10 - 'a';
    } else if (symbol >= 'A' && symbol <= 'F') {
        currentUnicode_[idxUnicode_] = currentUnicode_[idxUnicode_] * 16 + symbol + 10 - 'A';
    } else {
        return false;
    }
    return true;
}

template<typename K>
template<bool Compound, int NewState, typename ... Args>
JsonValueTempl<K>* StreamedJsonParser<K>::addValue(JsonValueTempl<K>* current, Args&& ... args) {
    state_ = NewState;
    if (current->is_array()) {
        current->as_array()->emplace_back(std::forward<Args>(args)...);
        if  constexpr (Compound) {
            current = &current->as_array()->back();
            stack_.emplace_back(current);
        }
        return current;
    } else {
        new (current) JsonValueTempl<K>(std::forward<Args>(args)...);
        if  constexpr (!Compound) {
            return popStack();
        } else {
            return current;
        }
    }
}

template<typename K>
JsonValueTempl<K>* StreamedJsonParser<K>::popStack() {
    stack_.pop_back();
    if (stack_.empty()) {
        state_ = Done;
        return nullptr;
    }
    return stack_.back();
}

template<typename K, bool All>
struct extractor {
    using strType = typename JsonValueTempl<K>::strType;
    using ssType = typename JsonValueTempl<K>::ssType;

    strType strValue;

    ssType extract(const K* startProcess, const K* ptr, chunked_string_builder<K>& text) {
        if (startProcess) {
            return {startProcess, size_t(ptr - startProcess)};
        } else if (text.is_continuous()) {
            return {text.begin(), text.length()};
        }
        strValue = text;
        return strValue;
    }
};

template<typename K>
struct extractor<K, true> {
    using ssType = typename JsonValueTempl<K>::ssType;

    ssType extract(const K* startProcess, const K* ptr, chunked_string_builder<K>& text) {
        return {startProcess, size_t(ptr - startProcess)};
    }
};

template<typename K>
template<bool asInt, bool All>
JsonValueTempl<K>* StreamedJsonParser<K>::addNumber(JsonValueTempl<K>* current) {
    extractor<K, All> e;
    ssType ssValue = e.extract(startProcess_, ptr_, text_);
    JsonValueTempl<K> jsonValue;

    if constexpr (asInt) {
        auto [res, err, _] = ssValue.template to_int<int64_t, true, 10, false>();
        if (err == IntConvertResult::Success) {
            jsonValue = res;
        }
    }

    if (!asInt || jsonValue.is_undefined()) {
        if constexpr (is_one_of_std_char_v<K>) {
            jsonValue = ssValue.to_double();
        } else {
            jsonValue = lstringa<100>(ssValue).to_double();
        }
    }

    if constexpr (!All) {
        if (!startProcess_) {
            text_.reset();
        }
    }
    startProcess_ = nullptr;

    return addValue<false, WaitComma>(current, std::move(jsonValue));
}

stringa get_file_content(stra filePath) {
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Can not open file " << filePath << std::endl;
        throw std::runtime_error{"Can not open file"};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // Такой тип удобен для передачи потом в stringa
    lstringsa<0> result;
    file.read(result.set_size(size), size);
    result.replace("\r\n", "\n");
    return result;
}

// Явно инстанцируем шаблоны для этих типов
template class JsonValueTempl<u8s>;
template class JsonValueTempl<u16s>;
template class JsonValueTempl<u32s>;
template class JsonValueTempl<wchar_t>;

template class StreamedJsonParser<u8s>;
template class StreamedJsonParser<u16s>;
template class StreamedJsonParser<u32s>;
template class StreamedJsonParser<wchar_t>;

} // namespace simjson
