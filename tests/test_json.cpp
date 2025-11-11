#include <simjson/json.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>

namespace simjson::tests {

TEST(SimJson, CreateJsonUndefined) {
    JsonValue json;
    EXPECT_TRUE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
}

TEST(SimJson, CreateJsonNull) {
    JsonValue json(Json::Null);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_null());

    JsonValue jsont(Json::null);
    EXPECT_FALSE(jsont.is_undefined());
    EXPECT_FALSE(jsont.is_object());
    EXPECT_TRUE(jsont.is_null());
}

TEST(SimJson, CreateJsonBool) {
    JsonValue json(false);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_boolean());
    EXPECT_EQ(json.as_boolean(), false);

    JsonValue jsont(true);
    EXPECT_FALSE(jsont.is_undefined());
    EXPECT_FALSE(jsont.is_object());
    EXPECT_TRUE(jsont.is_boolean());
    EXPECT_EQ(jsont.as_boolean(), true);

    JsonValue jsonk(Json::Boolean);
    EXPECT_FALSE(jsonk.is_undefined());
    EXPECT_FALSE(jsonk.is_object());
    EXPECT_TRUE(jsonk.is_boolean());
    EXPECT_EQ(jsonk.as_boolean(), false);
}

TEST(SimJson, CreateJsonInt) {
    JsonValue json(1);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_integer());
    EXPECT_EQ(json.as_integer(), 1);

    JsonValue jsonk(Json::Integer);
    EXPECT_FALSE(jsonk.is_undefined());
    EXPECT_FALSE(jsonk.is_object());
    EXPECT_TRUE(jsonk.is_integer());
    EXPECT_EQ(jsonk.as_integer(), 0);
}

TEST(SimJson, CreateJsonString) {
    JsonValue json("test");
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_text());
    EXPECT_EQ(json.as_text(), "test");

    JsonValue jsont(stringa("test"));
    EXPECT_FALSE(jsont.is_undefined());
    EXPECT_FALSE(jsont.is_object());
    EXPECT_TRUE(jsont.is_text());
    EXPECT_EQ(jsont.as_text(), "test");

    JsonValue jsonk(Json::Text);
    EXPECT_FALSE(jsonk.is_undefined());
    EXPECT_FALSE(jsonk.is_object());
    EXPECT_TRUE(jsonk.is_text());
    EXPECT_EQ(jsonk.as_text(), "");

    JsonValue jsonc(Json::emptyString);
    EXPECT_FALSE(jsonc.is_undefined());
    EXPECT_FALSE(jsonc.is_object());
    EXPECT_TRUE(jsonc.is_text());
    EXPECT_EQ(jsonc.as_text(), "");
}

TEST(SimJson, CreateJsonDouble) {
    JsonValue json(0.1);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_real());
    EXPECT_EQ(json.as_real(), 0.1);

    JsonValue jsonf(0.1f);
    EXPECT_FALSE(jsonf.is_undefined());
    EXPECT_FALSE(jsonf.is_object());
    EXPECT_TRUE(jsonf.is_real());
    EXPECT_EQ(jsonf.as_real(), 0.1f);

    JsonValue jsonk(Json::Real);
    EXPECT_FALSE(jsonk.is_undefined());
    EXPECT_FALSE(jsonk.is_object());
    EXPECT_TRUE(jsonk.is_real());
    EXPECT_EQ(jsonk.as_real(), 0.0);
}

TEST(SimJson, CreateJsonArray) {
    JsonValue json(Json::Array);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.as_array()->size(), 0);

    JsonValue jsona(Json::emptyArray);
    EXPECT_FALSE(jsona.is_undefined());
    EXPECT_FALSE(jsona.is_object());
    EXPECT_TRUE(jsona.is_array());
    EXPECT_EQ(jsona.as_array()->size(), 0);

    JsonValue jsoni = {
        true,
        false,
        Json::null,
        "test",
    };

    EXPECT_TRUE(jsoni.is_array());
    EXPECT_EQ(jsoni.as_array()->size(), 4);
    EXPECT_TRUE(jsoni[0].is_boolean());
    EXPECT_TRUE(jsoni[0].as_boolean());
    EXPECT_TRUE(jsoni[1].is_boolean());
    EXPECT_FALSE(jsoni[1].as_boolean());
    EXPECT_TRUE(jsoni[2].is_null());
    EXPECT_TRUE(jsoni[3].is_text());
    EXPECT_EQ(jsoni[3].as_text(), "test");
}

TEST(SimJson, CreateJsonObject) {
    JsonValue json(Json::Object);
    EXPECT_FALSE(json.is_undefined());
    EXPECT_TRUE(json.is_object());
    EXPECT_FALSE(json.is_array());
    EXPECT_EQ(json.as_object()->size(), 0);

    JsonValue jsona(Json::emptyObject);
    EXPECT_FALSE(jsona.is_undefined());
    EXPECT_TRUE(jsona.is_object());
    EXPECT_FALSE(jsona.is_array());
    EXPECT_EQ(jsona.as_object()->size(), 0);

    JsonValue jsoni = {
        {"key1"_h, true},
        {"key2"_h, false},
        {"key3"_h, {0, 1, 2, 3}},
        {"key4"_h, "test"},
    };

    EXPECT_TRUE(jsoni.is_object());
    EXPECT_EQ(jsoni.as_object()->size(), 4);
    EXPECT_TRUE(jsoni.at("key1").is_boolean());
    EXPECT_TRUE(jsoni.at("key1").as_boolean());
    EXPECT_TRUE(jsoni.at("key2").is_boolean());
    EXPECT_FALSE(jsoni.at("key2").as_boolean());

    EXPECT_TRUE(jsoni.at("key3").is_array());
    EXPECT_EQ(jsoni.at("key3").as_array()->size(), 4);
    EXPECT_TRUE(jsoni.at("key3")[1].is_integer());
    EXPECT_EQ(jsoni.at("key3")[1].as_integer(), 1);

    EXPECT_TRUE(jsoni.at("key4").is_text());
    EXPECT_EQ(jsoni.at("key4").as_text(), "test");
}
TEST(SimJson, CreateJsonObjectArray) {
    {
        stringa key3 = "key3";
        JsonValue json = {
            {"key1"_h, 12},
            {"key2", "test"},
            {key3, "test"}
        };
        EXPECT_EQ(json.type(), Json::Object);
        EXPECT_EQ(json["key1"_h].as_integer(), 12);
        EXPECT_EQ(json["key1"_h].type(), Json::Integer);
        EXPECT_EQ(json["key2"_h].type(), Json::Text);
        EXPECT_EQ(json["key2"_h].as_text(), "test");
        EXPECT_EQ(json[key3].as_text(), "test");
    }
    {
        JsonValue json = {
            "key1", 12
        };
        EXPECT_EQ(json.type(), Json::Array);
        EXPECT_EQ(json.as_array()->size(), 2);
        EXPECT_EQ(json[0].as_text(), "key1");
        EXPECT_EQ(json[1].as_integer(), 12);
    }
    {
        JsonValue json = {
            "key1",
            12,
            false,
            Json::null,
            1.25,
            true,
            {
                {"subkey"_h, "test"},
                {"subkey1"_h, "test"}
            }
        };
        EXPECT_EQ(json.type(), Json::Array);
        EXPECT_EQ(json.as_array()->size(), 7);
        EXPECT_EQ(json[0].as_text(), "key1");
        EXPECT_EQ(json[1].as_integer(), 12);
        EXPECT_EQ(json[2].as_boolean(), false);
        EXPECT_EQ(json[3].is_null(), true);
        EXPECT_EQ(json[4].as_real(), 1.25);
        EXPECT_EQ(json[5].as_boolean(), true);
        EXPECT_EQ(json[6].is_object(), true);
        EXPECT_EQ(json[6]["subkey"].as_text(), "test");
    }
}

TEST(SimJson, AssignJsonString) {
    JsonValue json;
    json = "test";
    EXPECT_FALSE(json.is_undefined());
    EXPECT_FALSE(json.is_object());
    EXPECT_TRUE(json.is_text());
    EXPECT_EQ(json.as_text(), "test");
}

TEST(SimJson, ObjectProps) {
    JsonValue json;
    json["key1"][stringa("key2")]["key3"] = "test";
    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.as_object()->contains("key1"_h));
    EXPECT_TRUE(json.at("key1").is_object());
    EXPECT_EQ(json["key1"]["key2"]["key3"].as_text(), "test");
}

TEST(SimJson, ArrayElems) {
    JsonValue json;
    json[0] = 1;
    json[3] = "test";
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.as_array()->size(), 4);
    EXPECT_EQ(json.as_array()->at(0).as_integer(), 1);
    EXPECT_EQ(json.as_array()->at(1).type(), Json::Undefined);
    EXPECT_EQ(json[3].as_text(), "test");
}

TEST(SimJson, Converts) {
    {
        JsonValue json("12");
        EXPECT_FALSE(json.boolean().has_value());
        EXPECT_TRUE(json.to_boolean());

        EXPECT_FALSE(json.integer().has_value());
        EXPECT_EQ(json.integer().value_or(10), 10);
        EXPECT_EQ(json.to_integer().value(), 12);
    }
    {
        JsonValue json(12);
        EXPECT_FALSE(json.boolean().has_value());
        EXPECT_TRUE(json.to_boolean());
        EXPECT_EQ(json.to_text(), "12");
    }
    {
        JsonValue json("sdss");
        EXPECT_FALSE(json.to_integer().has_value());
        EXPECT_EQ(json.to_integer().value_or(0), 0);
    }
}

TEST(SimJson, Clone) {
    JsonValue json{
        {"p1"_h, 1},
        {"p2"_h, 2},
    };
    auto refCopy = json;
    EXPECT_EQ(refCopy["p1"_h].as_integer(), 1);
    EXPECT_EQ(refCopy["p2"_h].as_integer(), 2);
    EXPECT_EQ(refCopy.as_object().get(), json.as_object().get());

    auto copy = json.clone();
    EXPECT_EQ(copy["p1"_h].as_integer(), 1);
    EXPECT_EQ(copy["p2"_h].as_integer(), 2);
    EXPECT_NE(copy.as_object().get(), json.as_object().get());
}

TEST(SimJson, ParseJsonString) {
    {
        auto [json, res, l, c] = JsonValue::parse("  true  ");
        EXPECT_EQ(res, JsonParseResult::Success);
        EXPECT_TRUE(json.is_boolean());
        EXPECT_TRUE(json.as_boolean());
    }

    {
        auto [json, res, line, col] = JsonValue::parse("  123  ");
        EXPECT_EQ(res, JsonParseResult::Success);
        EXPECT_TRUE(json.is_integer());
        EXPECT_EQ(json.as_integer(), 123);
    }

    {
        StreamedJsonParser<u8s> parser;
        EXPECT_EQ(parser.processChunk("12", false), JsonParseResult::Pending);
        EXPECT_EQ(parser.processChunk("3", true), JsonParseResult::Success);
        EXPECT_TRUE(parser.result_.is_integer());
        EXPECT_EQ(parser.result_.as_integer(), 123);
    }

    {
        StreamedJsonParser<u8s> parser;
        EXPECT_EQ(parser.processChunk(" trues", false), JsonParseResult::NoNeedMore);
        EXPECT_TRUE(parser.result_.is_boolean());
        EXPECT_EQ(parser.result_.as_boolean(), true);
        EXPECT_EQ(parser.col_, 6);
    }

    {
        auto [json, res, l, c] = JsonValue::parse(R"({"abc" : 10, "cde": [1, 2, "14\n"]})");
        EXPECT_TRUE(json.is_object());
        EXPECT_EQ(json.at("abc"_h).as_integer(), 10);
        EXPECT_EQ(json.at("cde"_h).type(), Json::Array);
        EXPECT_EQ(json.at("cde"_h).size(), 3);
        EXPECT_EQ(json.at("cde"_h)[0].as_integer(), 1);
        EXPECT_EQ(json.at("cde"_h)[1].as_integer(), 2);
        EXPECT_EQ(json.at("cde"_h)[2].as_text(), "14\n");
    }

    {
        StreamedJsonParser<u8s> parser;
        parser.processChunk(R"({"abc" : 1)", false);
        parser.processChunk(R"(0, "cde": [1, 2, "14\)", false);
        parser.processChunk(R"(n",{  }]}  )", true);

        auto json = std::move(parser.result_);

        EXPECT_TRUE(json.is_object());
        EXPECT_EQ(json.at("abc"_h).as_integer(), 10);
        EXPECT_EQ(json.at("cde"_h).type(), Json::Array);
        EXPECT_EQ(json.at("cde"_h).size(), 4);
        EXPECT_EQ(json.at("cde"_h)[0].as_integer(), 1);
        EXPECT_EQ(json.at("cde"_h)[1].as_integer(), 2);
        EXPECT_EQ(json.at("cde"_h)[2].as_text(), "14\n");
        EXPECT_TRUE(json.at("cde"_h)[3].is_object());
        EXPECT_EQ(json.at("cde"_h)[3].size(), 0);
    }
}

TEST(SimJson, JsonStoreString) {
    JsonValue json {
        {"p1", 1},
        {"p3"_h, true},
        {"p2"_h, 2},
        {"p4"_h, {1, 2, false, Json::null, "test\ntest\x01", Json::emptyObject, Json::emptyArray}}
    };
    EXPECT_EQ(stringa{json.store(false, true)}, R"({"p1":1,"p2":2,"p3":true,"p4":[1,2,false,null,"test\ntest\u0001",{},[]]})");
    EXPECT_EQ(stringa{json.store(true, true)}, R"({
  "p1": 1,
  "p2": 2,
  "p3": true,
  "p4": [
    1,
    2,
    false,
    null,
    "test\ntest\u0001",
    {},
    []
  ]
})");
}

TEST(SimJson, JsonThrow) {
    JsonValue val = 10;
    lstringa<100> err_descr;
    try {
        EXPECT_EQ(val.integer().value(), 10);
        val.boolean_or_throw<std::runtime_error>("Need boolean value");
    } catch (const std::runtime_error& err) {
        err_descr = stra{err.what()};
    }
    EXPECT_EQ(err_descr, "Need boolean value");
}

/*
stringa getFileContent(stra fileName) {
    std::ifstream file(fileName, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    lstringsa<0> result;
    file.read(result.reserve(size), size);
    return result;
}

TEST(SimJson, JsonParseBig) {
    {
        stringa content1 = get_file_content("citm_catalog.json");
        stringa content2 = get_file_content("canada.json");
        stringa content3 = get_file_content("twitter.json");

        auto t1 = std::chrono::high_resolution_clock::now();

        for (size_t idx = 0; idx < 10; idx++) {
            auto json1 = JsonValue::parse(content1);
            EXPECT_EQ(std::get<1>(json1), JsonParseResult::Success);
            auto json2 = JsonValue::parse(content2);
            EXPECT_EQ(std::get<1>(json2), JsonParseResult::Success);
            auto json3 = JsonValue::parse(content3);
            EXPECT_EQ(std::get<1>(json3), JsonParseResult::Success);
        }

        auto t2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> ms_double = t2 - t1;

        std::cout << "Parse at " << ms_double.count() / 10.0 << std::endl;

        //EXPECT_TRUE(json1.is_object());
        //EXPECT_TRUE(json2.is_object());
        //EXPECT_TRUE(json3.is_object());
    }
}
*/

} // namespace simjson::tests
