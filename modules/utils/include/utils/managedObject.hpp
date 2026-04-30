#pragma once
#include <common/types.hpp>
#include <shared_mutex>

namespace ioteye::utils {
class ManagedObject {
public:
    ManagedObject();
    virtual ~ManagedObject() = default;
    types::objID getID() const;
    virtual void process();

protected:
    types::objID m_id;
    mutable std::shared_mutex m_sharedMutex;

private:
    static size_t m_idSequence;
};
}  // namespace ioteye::utils