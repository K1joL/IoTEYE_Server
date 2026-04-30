#include <utils/managedObject.hpp>

namespace ioteye::utils {

size_t ManagedObject::m_idSequence = 1;

ManagedObject::ManagedObject() {
    m_id = m_idSequence;
    ++m_idSequence;
}

types::objID ManagedObject::getID() const {
    return m_id;
}

void ManagedObject::process() {
}

}  // namespace ioteye::utils