#include "common/Config.hpp"

#include "common/Constants.hpp"
#include "common/Logger.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fstream>

namespace pt = boost::property_tree;

namespace jar {

struct Config::Options {
    Options()
        : proxyServerPort{kDefaultProxyServerPort}
        , proxyServerThreads{kDefaultProxyServerThreads}
        , recognizeServerHost{}
        , recognizeServerPort{}
        , recognizeServerAuth{}
        , recognizeServerThreads{kDefaultRecognizeServerThreads}
    {
    }

    std::uint16_t proxyServerPort;
    std::size_t proxyServerThreads;
    std::string recognizeServerHost;
    std::uint16_t recognizeServerPort;
    std::string recognizeServerAuth;
    std::size_t recognizeServerThreads;
};

Config::Config()
    : _options{std::make_unique<Options>()}
{
}

Config::~Config()
{
}

std::uint16_t
Config::proxyServerPort() const
{
    return _options->proxyServerPort;
}

std::size_t
Config::proxyServerThreads() const
{
    return _options->proxyServerThreads;
}

std::string_view
Config::recognizeServerHost() const
{
    return _options->recognizeServerHost;
}

std::uint16_t
Config::recognizeServerPort() const
{
    return _options->recognizeServerPort;
}

std::string_view
Config::recognizeServerAuth() const
{
    return _options->recognizeServerAuth;
}

std::size_t
Config::recognizeThreads() const
{
    return _options->recognizeServerThreads;
}

bool
Config::load(fs::path filePath)
{
    std::error_code error;
    if (!fs::exists(filePath, error)) {
        return false;
    }

    std::ifstream stream{filePath};
    if (!stream) {
        return false;
    }

    pt::ptree tree;
    try {
        pt::read_json(stream, tree);
    } catch (pt::json_parser_error& e) {
        LOGE("Failed to parse <{}> config file on <{}> line: {}",
             e.filename(),
             e.line(),
             e.message());
        return false;
    }

    if (auto port = tree.get_optional<int>("proxy.port"); port) {
        _options->proxyServerPort = static_cast<std::uint16_t>(port.get());
    }
    if (auto threads = tree.get_optional<int>("proxy.threads"); threads) {
        _options->proxyServerThreads = static_cast<std::size_t>(threads.get());
    }
    if (auto host = tree.get_optional<std::string>("recognize.host"); host) {
        _options->recognizeServerHost = std::move(host.get());
    }
    if (auto port = tree.get_optional<int>("recognize.port"); port) {
        _options->recognizeServerPort = static_cast<std::uint16_t>(port.get());
    }
    if (auto auth = tree.get_optional<std::string>("recognize.auth"); auth) {
        _options->recognizeServerAuth = std::move(auth.get());
    }
    if (auto threads = tree.get_optional<int>("recognize.threads"); threads) {
        _options->recognizeServerThreads = static_cast<std::size_t>(threads.get());
    }

    return true;
}

} // namespace jar