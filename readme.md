# simjson - простейшая библиотека для работы с JSON
[![CMake on multiple platforms](https://github.com/orefkov/simjson/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/orefkov/simjson/actions/workflows/cmake-multi-platform.yml)

Предназначена для работы с JSON при использовании библиотеки [simstr](https://github.com/orefkov/simstr).

Версия 1.0.

В этой библиотеке содержится простая реализация простого объекта JsonValue для работы с JSON с использованием строковых объектов
библиотеки *simstr*, так как другие библиотеки работают в основном с `std::string` или сырыми `const char*`.
Задача как-то соревноваться по производительности или оптимальности с другими библиотеками не ставилась, я её применяю в-основном 
для работы с небольшими конфиг-файлами - прочитать, изменить, записать.

Для json-объектов используется `std::unordered_map`, в лице `hashStrMap<K, JsonValueTemp<K>>`,
для массивов - `std::vector<JsonValueTemp<K>>`, строки хранятся в `sstring<K>`.

## Основные возможности библиотеки
- Работает со всеми строками simstr.
- Поддерживает работу со строками `char`, `char16_t`, `char32_t`, `wchar_t`.
- Удобное создание, чтение и модификация json значений.
- Копирование таких JSON-значений, как массивы и объекты производится по ссылке (копируется только `shared_ptr`).
- Возможно "глубокое" копирование aka клонирование, JSON-значений, в этом случае для массивов и объектов создаётся полная копия.
- "Слияние" одного JSON объекта с другим, с возможностью задать приоритет.
- Расширенная работа с числами - позволяет использовать int64_t и double.
- Парсинг строки в Json, с поддержкой порционного парсинга.
- Сериализация json в строку, с опциями - сортировка ключей, "читаемый" вывод, количество отступов и символ
  отступа при "читаемом" выводе.

## Основные объекты библиотеки
- JsonValueTempl<K> - тип Json значения, параметр К задаёт тип используемых символов в строке. Алиасы:
  - JsonValue - для строк char
  - JsonValueU - для строк char16_t
  - JsonValueUU - для строк char32_t
  - JsonValueW - для строк wchar_t
- StreamedJsonParser<K> - парсер строки в JSON, поддерживающий "порционный" парсинг.\
  Например, данные приходят порциями из сети, вы их по мере поступления скармливаете парсеру, пока он или не
  распарсит, или выдаст ошибку.

## Использование
`simjson` состоит из заголовочного файла и одного исходника. Можно подключать как CMake проект через `add_subdirectory` (библиотека `simjson`),
можно просто включить файлы в свой проект. Для сборки также требуется [simstr](https://github.com/orefkov/simstr) (при использовании CMake
скачивается автоматически).

Для работы `simjson` требуется компилятор с поддержкой стандарта не ниже С++20 (используются концепты).

## Примеры использования
### Создание, чтение
```cpp
    // Простой json, равный 1.
    JsonValue json = 1; // int64
    stringa text = json.store(); // "1"
    
    // Инициализация объекта, ""_h - несколько оптимизирует ключ, вычисляя хэш при компиляции
    JsonValue obj = {
        {"Key1"_h, 1},
        {"Key2"_h, true},
        {"Key3", {1, 2, Json::null, "test", false}}, // тут будет массив [1, 2, null, "test", false]
        {"Key4"_h, {
            {"subkey1"_h, true},
            {"subkey2", "subkey"},
        }},
    };

    // Пустой объект
    JsonValue test;
    // Сохраняем ключи, сразу несколько уровней
    test["a"_h]["b"_h]["c"_h] = 10;
    text = json.store(); // {"a":{"b":{"c":10}}}
    
    // Преобразует json-объект в массив
    test[0] = "value";
    test[-1] = true;  // При использовании -1 - значение добавляется в конец массива
    test[10] = false; // Увеличит размер массива до 11

    // Прочитать значение
    JsonValue config;
    ....
    stringa log_path = config("instance"_h, "base"_h, "log"_h, "path"_h).text().value_or("./log.txt");
    stringa work_dir = config("instance"_h, "base"_h, "work_dir"_h).text_or_throw<std::runtime_error>(
      "not specified work dir");
```

### Пример: Задание дефолтных значений, чтение из файла и объединение с дефолтными значениями.
```cpp

JsonValue json_config = {
    {"section1"_h, {
        {"sub1"_h, {
            {"path"_h, "."},
            {"workers"_h, 10},
        }},
        {"sub2"_h, {
            {"check"_h, true},
        }},
        {"key1"_h, true},
        {"key2"_h, false},
    }},
    {"section2"_h, {
        {"key1"_h, false},
        {"key2"_h, true},
    }},
};

inline bool is_file_exist(stra path) {
    auto status = std::filesystem::status(path.to_sv());
    return std::filesystem::status_known(status) && std::filesystem::is_regular_file(status);
}

void read_config_from_file(ssa folder, ssa file_name) {
    stringa path_checked = folder + e_if(folder(-1) != PATH_SEPARATOR, PATH_SEPARATOR);
    if (!file_name.is_empty()) {
        lstringa<MAX_PATH> fullPath = path_checked + file_name;
        stringa config;
        if (is_file_exist(fullPath)) {
            config = get_file_content(fullPath);
            
            auto [readed, error, line, col] = JsonValue::parse(config);
            
            if (error != JsonParseResult::Success) {
                std::cerr << "Error in parse config file " << fullPath <<
                  " at line " << line << ", col " << col << std::endl;
                throw std::runtime_error{"Error parse config file"};
            }
            json_config.merge(readed);
        }
        // У нас в дефолтном конфиге могли появится новые ключи, и хотелось бы их дефолтные значения
        // также скинуть в файл, чтобы проще потом редактировать.
        stringa new_config = json_config.store(true, true, ' ', 4);
        if (new_config != config) {
            // В дефолтном конфиге добавились новые ключи
            std::ofstream file(fullPath.c_str(), std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                std::cerr << "Error in create config file " << fullPath << std::endl;
            } else {
                file.write(new_config.c_str(), new_config.length());
            }
        }
    }
    json_config["runtime"_h]["location"_h]["base_dir"_h] = std::move(path_checked);
}
```
### Пример - инициализация из стандартных контейнеров
```cpp
    // Массивы
    std::vector<int> vals = {1, 2, 3, 4};
    JsonValue json = vals;
    EXPECT_EQ(json.type(), Json::Array);
    EXPECT_EQ(json.store(), "[1,2,3,4]");

    JsonValue json1 = std::array<int, 4>{4, 3, 2, 1};
    EXPECT_EQ(json1.type(), Json::Array);
    EXPECT_EQ(json1.store(), "[4,3,2,1]");

    std::list<stringa> texts = {"one", "two", "three"};
    JsonValue json2 = texts;
    EXPECT_EQ(json2.type(), Json::Array);
    EXPECT_EQ(json2.store(), "[\"one\",\"two\",\"three\"]");

    // Ключ-значение
    hashStrMapA<int> vals = {
        {"one"_h, 1}, {"two"_h, 2}, {"three"_h, 3}, {"four"_h, 4}
    };
    JsonValue json = vals;
    EXPECT_EQ(json.type(), Json::Object);
    EXPECT_EQ(json.store(false, true), "{\"four\":4,\"one\":1,\"three\":3,\"two\":2}");

    std::map<stringa, int> vals1 = {
        {"one", 1}, {"two", 2}, {"three", 3}, {"four", 4}
    };
    JsonValue json1 = vals1;
    EXPECT_EQ(json1.type(), Json::Object);
    EXPECT_EQ(json1.store(false, true), "{\"four\":4,\"one\":1,\"three\":3,\"two\":2}");

    std::vector<std::pair<lstringa<20>, int>> vals2 = {
        {"one", 1}, {"two", 2}, {"three", 3}, {"four", 4}
    };
    JsonValue json2 = vals2;
    EXPECT_EQ(json2.type(), Json::Object);
    EXPECT_EQ(json2.store(false, true), "{\"four\":4,\"one\":1,\"three\":3,\"two\":2}");

```

## Сгенерированная документация
[Находится здесь](https://snegopat.ru/simjson/docs/)
