#include "Style.h"

#include <masharifcore/layout/Node.h>

void masharif::Style::NotifyOwner() {
    if (m_Owner)
        m_Owner->MarkDirtyToRoot();
}
