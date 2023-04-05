#include "intent/IntentRegistry.hpp"

#include "intent/Intent.hpp"

#include <boost/assert.hpp>

#include <stdexcept>

namespace jar {

bool
IntentRegistry::has(const std::string& name) const
{
    return _intents.contains(name);
}

std::shared_ptr<Intent>
IntentRegistry::get(const std::string& name)
{
    BOOST_ASSERT(!name.empty());
    if (auto intentIt = _intents.find(name); intentIt == _intents.cend()) {
        throw std::runtime_error{"No intent with given name"};
    } else {
        return std::get<1>(*intentIt)->clone();
    }
}

void
IntentRegistry::add(std::shared_ptr<Intent> intent)
{
    BOOST_ASSERT(intent);
    if (has(intent->name())) {
        throw std::runtime_error{"Intent is already present"};
    } else {
        _intents.emplace(intent->name(), std::move(intent));
    }
}

void
IntentRegistry::remove(const std::string& name)
{
    BOOST_ASSERT(!name.empty());
    _intents.erase(name);
}

} // namespace jar