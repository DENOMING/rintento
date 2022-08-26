#include "intent/IntentRecognizeDispatcher.hpp"

#include "common/Logger.hpp"
#include "intent/IntentRecognizeMessageHandler.hpp"
#include "intent/IntentRecognizeSpeechHandler.hpp"
#include "intent/IntentRecognizeTerminalHandler.hpp"
#include "intent/Utils.hpp"

namespace jar {

IntentRecognizeDispatcher::Ptr
IntentRecognizeDispatcher::create(uint16_t identity,
                                  IntentRecognizeConnection::Ptr connection,
                                  IntentPerformer::Ptr performer,
                                  WitRecognitionFactory::Ptr factory)
{
    assert(identity > 0);
    assert(connection);
    assert(performer);
    assert(factory);

    // clang-format off
    return std::shared_ptr<IntentRecognizeDispatcher>(
        new IntentRecognizeDispatcher{identity, std::move(connection), std::move(performer), std::move(factory)}
    );
    // clang-format on
}

IntentRecognizeDispatcher::IntentRecognizeDispatcher(uint16_t identity,
                                                     IntentRecognizeConnection::Ptr connection,
                                                     IntentPerformer::Ptr performer,
                                                     WitRecognitionFactory::Ptr factory)
    : _identity{identity}
    , _connection{std::move(connection)}
    , _performer{std::move(performer)}
    , _factory{std::move(factory)}
{
}

uint16_t
IntentRecognizeDispatcher::identity() const
{
    return _identity;
}

void
IntentRecognizeDispatcher::whenDone(std::function<DoneSignature> callback)
{
    assert(callback);
    _doneCallback = std::move(callback);
}

void
IntentRecognizeDispatcher::dispatch()
{
    readHeader();
}

void
IntentRecognizeDispatcher::readHeader()
{
    _connection->readHeader<http::empty_body>(
        [self = shared_from_this()](auto& buffer, auto& parser, auto error) {
            self->onReadHeaderDone(buffer, parser, error);
        });
}

void
IntentRecognizeDispatcher::onReadHeaderDone(beast::flat_buffer& buffer,
                                            http::request_parser<http::empty_body>& parser,
                                            sys::error_code error)
{
    if (error == http::error::end_of_stream) {
        LOGI("Connection was closed");
        finalize();
        return;
    }
    if (error) {
        LOGE("Failed to read: <{}>", error.what());
        finalize();
        return;
    }

    _handler = getHandler();
    assert(_handler);
    _handler->handle(buffer, parser);
}

void
IntentRecognizeDispatcher::onComplete(Utterances utterances, sys::error_code error)
{
    if (error) {
        LOGE("Request dispatching has failed: <{}> error", error.what());
    } else {
        LOGD("Request dispatching has succeed: <{}> size", utterances.size());
        _performer->perform(std::move(utterances));
    }

    readHeader();
}

IntentRecognizeHandler::Ptr
IntentRecognizeDispatcher::getHandler()
{
    auto handler1 = std::make_unique<IntentRecognizeMessageHandler>(
        _connection, _factory, [this](auto result, sys::error_code error) {
            LOGD("Recognize message handler has finished: success<{}>", !error.failed());
            onComplete(std::move(result), error);
        });
    auto handler2 = std::make_unique<IntentRecognizeSpeechHandler>(
        _connection, _factory, [this](auto result, auto error) {
            LOGD("Recognize speech handler has finished: success<{}>", !error.failed());
            onComplete(std::move(result), error);
        });
    auto handler3 = std::make_unique<IntentRecognizeTerminalHandler>(
        _connection, [this](auto result, auto error) {
            LOGD("Terminal handler has finished");
            onComplete(std::move(result), error);
        });
    handler2->setNext(std::move(handler3));
    handler1->setNext(std::move(handler2));
    return handler1;
}

void
IntentRecognizeDispatcher::finalize()
{
    if (_doneCallback) {
        _doneCallback(_identity);
    }
}

} // namespace jar