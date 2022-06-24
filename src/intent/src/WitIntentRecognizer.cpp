#include "intent/WitIntentRecognizer.hpp"

#include "intent/Config.hpp"
#include "intent/WitPendingRecognition.hpp"
#include "intent/WitIntentMessageSession.hpp"
#include "intent/WitIntentSpeechSession.hpp"

namespace jar {

WitIntentRecognizer::WitIntentRecognizer(net::any_io_executor executor)
    : _context{ssl::context::tlsv12_client}
    , _executor{std::move(executor)}
{
    _context.set_default_verify_paths();
    _context.set_verify_mode(ssl::verify_peer);
}

PendingRecognition::Ptr
WitIntentRecognizer::recognize(std::string_view message)
{
    auto session = WitIntentMessageSession::create(_context, _executor);
    auto pending = WitPendingRecognition::create(session);
    session->run(WitBackendHost, WitBackendPort, WitBackendAuth, message);
    return pending;
}

PendingRecognition::Ptr
WitIntentRecognizer::recognize(std::string_view message,
                               RecognitionCalback callback,
                               net::any_io_executor executor)
{
    auto session = WitIntentMessageSession::create(_context, _executor);
    auto pending = WitPendingRecognition::create(session, std::move(callback), std::move(executor));
    session->run(WitBackendHost, WitBackendPort, WitBackendAuth, message);
    return pending;
}

PendingRecognition::Ptr
WitIntentRecognizer::recognize(fs::path filePath)
{
    auto session = WitIntentSpeechSession::create(_context, _executor);
    auto pending = WitPendingRecognition::create(session);
    session->run(WitBackendHost, WitBackendPort, WitBackendAuth, std::move(filePath));
    return pending;
}

PendingRecognition::Ptr
WitIntentRecognizer::recognize(fs::path filePath,
                               RecognitionCalback callback,
                               net::any_io_executor executor)
{
    auto session = WitIntentSpeechSession::create(_context, _executor);
    auto pending = WitPendingRecognition::create(session, std::move(callback), std::move(executor));
    session->run(WitBackendHost, WitBackendPort, WitBackendAuth, std::move(filePath));
    return pending;
}

} // namespace jar