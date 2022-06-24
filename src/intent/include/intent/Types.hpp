#pragma once

#include "intent/Constants.hpp"

#include <string>
#include <vector>
#include <functional>
#include <system_error>

namespace jar {

/**
 * The intent class representation
 */
struct Intent {
    std::string name;
    float confidence{0.0f};
};

/** The bunch of intents */
using Intents = std::vector<Intent>;

/**
 * The utterance class representation
 */
struct Utterance {
    std::string text;
    Intents intents;
};

/** The bunch of utterances */
using Utterances = std::vector<Utterance>;

/**
 * Recognition outcome callback
 */
using RecognitionCalback = std::function<void(Utterances result, std::error_code error)>;

bool
operator==(const Intent& lhs, const Intent& rhs);

std::ostream&
operator<<(std::ostream& s, const Intent& input);

} // namespace jar