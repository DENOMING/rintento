#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "intent/registry/GetWeatherHumidityAction.hpp"
#include "test/MockPositioningClient.hpp"
#include "test/MockSpeakerClient.hpp"
#include "test/MockWeatherClient.hpp"

#include <chrono>

using namespace testing;
using namespace jar;

namespace krn = std::chrono;

namespace {

CurrentWeatherData
getCurrentWeatherData()
{
    CustomData d;
    d.assign({
        {"main.humidity", 45},
    });
    return CurrentWeatherData{.data = std::move(d)};
}

ForecastWeatherData
getForecastWeatherData(krn::sys_seconds tsFrom, krn::sys_seconds tsTo)
{
    static constexpr auto kStep = krn::hours{3};
    int32_t humidity{50};
    CustomDataSet dataSet;
    while (tsFrom < tsTo) {
        const auto dt = krn::duration_cast<krn::seconds>(tsFrom.time_since_epoch()).count();
        CustomData d;
        d.assign({
            {"dt", int64_t{dt}},
            {"duration", int32_t{10800}},
            {"main.humidity", humidity},
        });
        dataSet.push_back(std::move(d));
        humidity += 5;
        tsFrom += kStep;
    }
    return ForecastWeatherData{.data = std::move(dataSet)};
}

} // namespace

class GetWeatherHumidityActionTest : public Test {
public:
    const std::string kIntentName{"test_intent"};

public:
    NiceMock<MockPositioningClient> positioning;
    NiceMock<MockSpeakerClient> speaker;
    NiceMock<MockWeatherClient> weather;
};

TEST_F(GetWeatherHumidityActionTest, GetCurrent)
{
    const auto weatherData{getCurrentWeatherData()};
    EXPECT_CALL(speaker, synthesizeSsml(Not(IsEmpty()), Not(IsEmpty())));
    EXPECT_CALL(weather, getCurrentWeather).WillOnce(InvokeArgument<2>(weatherData));

    auto action = GetWeatherHumidityAction::create(kIntentName, positioning, speaker, weather);
    ASSERT_TRUE(action);

    MockFunction<void(std::error_code)> onDone;
    EXPECT_CALL(onDone, Call(std::error_code{}));
    auto c = action->onDone(onDone.AsStdFunction());
    action->perform();
    c.disconnect();

    const auto& result = action->result();
    ASSERT_TRUE(result);
    EXPECT_EQ(result->min, HumidityGrade::Comfortable);
    EXPECT_EQ(result->avg, HumidityGrade::Comfortable);
    EXPECT_EQ(result->max, HumidityGrade::Comfortable);
}

TEST_F(GetWeatherHumidityActionTest, GetForPeriod)
{
    const auto now = krn::floor<krn::days>(krn::system_clock::now());
    const auto t1 = krn::floor<krn::days>(krn::system_clock::now());
    const auto t2 = t1 + krn::days{1};

    const auto weatherData{getForecastWeatherData(t1, t2)};
    EXPECT_CALL(speaker, synthesizeSsml(Not(IsEmpty()), Not(IsEmpty())));
    EXPECT_CALL(weather, getForecastWeather).WillOnce(InvokeArgument<2>(weatherData));

    DateTimeEntity entity;
    entity.from = DateTimeEntity::Value{
        .grain = DateTimeEntity::Grains::hour,
        .timestamp = Timestamp{t1},
    };
    entity.to = DateTimeEntity::Value{
        .grain = DateTimeEntity::Grains::hour,
        .timestamp = Timestamp{t2},
    };

    auto action = GetWeatherHumidityAction::create(kIntentName,
                                                   positioning,
                                                   speaker,
                                                   weather,
                                                   {
                                                       {DateTimeEntity::key(), EntityList{entity}},
                                                   });
    ASSERT_TRUE(action);

    MockFunction<void(std::error_code)> onDone;
    EXPECT_CALL(onDone, Call(std::error_code{}));
    auto c = action->onDone(onDone.AsStdFunction());
    action->perform();
    c.disconnect();

    const auto& result = action->result();
    ASSERT_TRUE(result);
    EXPECT_THAT(result->min, Eq(HumidityGrade::Comfortable));
    EXPECT_THAT(result->avg, Not(HumidityGrade::Comfortable));
    EXPECT_THAT(result->max, Not(HumidityGrade::Comfortable));
}

TEST_F(GetWeatherHumidityActionTest, Error)
{
    EXPECT_CALL(weather, getCurrentWeather)
        .WillOnce(InvokeArgument<3>(std::runtime_error{"Error"}));

    auto action = GetWeatherHumidityAction::create(kIntentName, positioning, speaker, weather);
    ASSERT_TRUE(action);

    MockFunction<void(std::error_code)> onDone;
    EXPECT_CALL(onDone, Call(Not(std::error_code{})));
    auto c = action->onDone(onDone.AsStdFunction());
    action->perform();
    c.disconnect();

    EXPECT_EQ(action->result(), std::nullopt);
}