#include "intent/registry/GetAirQualityAction.hpp"

#include "intent/Formatters.hpp"
#include "intent/IPositioningClient.hpp"
#include "intent/WitHelpers.hpp"

#include <jarvis/Logger.hpp>
#include <jarvis/weather/Formatters.hpp>

#include <algorithm>
#include <limits>
#include <ranges>

namespace jar {

std::shared_ptr<GetAirQualityAction>
GetAirQualityAction::create(std::string intent,
                            IPositioningClient& positioningClient,
                            ISpeakerClient& speakerClient,
                            IWeatherClient& weatherClient,
                            Entities entities)
{
    return std::shared_ptr<GetAirQualityAction>(new GetAirQualityAction{
        std::move(intent), positioningClient, speakerClient, weatherClient, std::move(entities)});
}

GetAirQualityAction::GetAirQualityAction(std::string intent,
                                         IPositioningClient& positioningClient,
                                         ISpeakerClient& speakerClient,
                                         IWeatherClient& weatherClient,
                                         Entities entities)
    : DateTimeAction{std::move(intent), std::move(entities)}
    , _positioningClient{positioningClient}
    , _speakerClient{speakerClient}
    , _weatherClient{weatherClient}
{
}

const GetAirQualityAction::Result&
GetAirQualityAction::result() const
{
    return _result;
}

std::shared_ptr<Action>
GetAirQualityAction::clone(Entities entities)
{
    return create(
        intent(), _positioningClient, _speakerClient, _weatherClient, std::move(entities));
}

void
GetAirQualityAction::perform()
{
    const auto loc{_positioningClient.location()};
    LOGD("[{}]: Getting air quality data for <{}> location", intent(), loc);

    auto onReady = [weakSelf = weak_from_this()](auto qualityData) {
        if (auto self = weakSelf.lock()) {
            self->onAirQualityDataReady(std::move(qualityData));
        }
    };
    auto onError = [weakSelf = weak_from_this()](const std::runtime_error& error) {
        if (auto self = weakSelf.lock()) {
            self->onAirQualityDataError(error);
        }
    };

    if (hasTimestamps()) {
        LOGD("[{}]: Time boundaries is available", intent());
        _weatherClient.getForecastAirQuality(
            loc.lat, loc.lon, std::move(onReady), std::move(onError));
    } else {
        LOGD("[{}]: No time boundaries is available", intent());
        _weatherClient.getCurrentAirQuality(
            loc.lat, loc.lon, std::move(onReady), std::move(onError));
    }
}

void
GetAirQualityAction::onAirQualityDataReady(CurrentAirQualityData data)
{
    LOGD("[{}]: Getting current air quality data was succeed", intent());

    if (cancelled()) {
        setError(std::make_error_code(std::errc::operation_canceled));
    } else {
        retrieveResult(data);
    }
}

void
GetAirQualityAction::onAirQualityDataReady(ForecastAirQualityData data)
{
    LOGD("[{}]: Getting forecast air quality data was succeed", intent());
    if (cancelled()) {
        setError(std::make_error_code(std::errc::operation_canceled));
    } else {
        retrieveResult(data);
    }
}

void
GetAirQualityAction::onAirQualityDataError(std::runtime_error error)
{
    LOGE("[{}]: Getting air quality data has failed: error<{}>", intent(), error.what());

    setError(std::make_error_code(std::errc::result_out_of_range));
}

void
GetAirQualityAction::retrieveResult(const CurrentAirQualityData& airQuality)
{
    try {
        setResult(AirQualityIndex{airQuality.data.get<int32_t>("aqi")});
    } catch (const std::exception& e) {
        LOGE("[{}]: Getting air quality status has failed: {}", intent(), e.what());
        setError(std::make_error_code(std::errc::invalid_argument));
    }
}

void
GetAirQualityAction::retrieveResult(const ForecastAirQualityData& airQuality)
{
    int32_t aqi1 = std::numeric_limits<int32_t>::min();
    try {
        const wit::DateTimePredicate predicate{timestampFrom(), timestampTo()};
        std::ranges::for_each(airQuality.data | std::views::filter(predicate),
                              [&](const CustomData& d) {
                                  if (const auto aqi2 = d.get<int32_t>("aqi"); aqi2 > aqi1) {
                                      aqi1 = aqi2;
                                  }
                              });
        setResult(AirQualityIndex{aqi1});
    } catch (const std::exception& e) {
        LOGE("[{}]: Getting air quality status has failed: {}", intent(), e.what());
        setError(std::make_error_code(std::errc::invalid_argument));
    }
}

void
GetAirQualityAction::setResult(Result result)
{
    LOGD("[{}]: Air quality status is available: {}", intent(), result.value());

    _result = std::move(result);

    announceResult();

    complete({});
}

void
GetAirQualityAction::setError(std::error_code errorCode)
{
    complete(errorCode);
}

void
GetAirQualityAction::announceResult()
{
    _speakerClient.synthesizeText(fmt::format("The air quality is {}", _result.value()), "en-US");
}

} // namespace jar