#pragma once

#include "intent/Types.hpp"
#include "intent/PendingRecognition.hpp"
#include "intent/WitCommon.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace jar {

class WitIntentRecognizer {
public:
    explicit WitIntentRecognizer(net::any_io_executor executor);

    PendingRecognition::Ptr
    recognize(std::string_view message);

    PendingRecognition::Ptr
    recognize(std::string_view message,
              RecognitionCalback callback,
              net::any_io_executor executor = {});

    PendingRecognition::Ptr
    recognize(fs::path filePath);

    PendingRecognition::Ptr
    recognize(fs::path filePath, RecognitionCalback callback, net::any_io_executor executor = {});

private:
    ssl::context _context;
    net::any_io_executor _executor;
};

} // namespace jar