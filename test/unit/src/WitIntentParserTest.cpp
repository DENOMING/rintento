#include "intent/WitIntentParser.hpp"
#include "test/Matchers.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace jar;

TEST(WitIntentParserTest, Parse1)
{
    static const std::string_view kInput{R"(
        {
          "text": "turn off the light",
          "intents": [
            {
              "id": "554903196208362",
              "name": "light_off",
              "confidence": 0.9265
            }
          ],
          "entities": {},
          "traits": {}
        }
    )"};

    WitIntentParser parser;
    EXPECT_THAT(parser.parse(kInput),
                Optional(Contains(isUtterance("turn off the light",
                                              Contains(isConfidentIntent("light_off", 0.9f))))));
}

TEST(WitIntentParserTest, Parse2)
{
    static const std::string_view kInput{R"(
        {
          "text": "Turn"
        }
        {
          "entities": {},
          "intents": [
            {
              "confidence": 0.9166,
              "id": "695468701564151",
              "name": "light_on"
            }
          ],
          "speech": {
            "confidence": 0.7675,
            "tokens": [
              {
                "end": 720,
                "start": 0,
                "token": "Turn"
              }
            ]
          },
          "text": "Turn on the light",
          "traits": {}
        }
    )"};

    WitIntentParser parser;
    EXPECT_THAT(parser.parse(kInput),
                Optional(Contains(isUtterance("Turn on the light",
                                              Contains(isConfidentIntent("light_on", 0.9f))))));
}

TEST(WitIntentParserTest, ErrorParse)
{
    static const std::string_view kInput{R"(
        {
            "text": "Turn"
        }
        {
            "entities:
    )"};

    WitIntentParser parser;
    EXPECT_EQ(parser.parse(kInput), std::nullopt);
}

TEST(WitIntentParserTest, ParseEmpty)
{
    WitIntentParser parser;
    EXPECT_EQ(parser.parse(""), std::nullopt);
}