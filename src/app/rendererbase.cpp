#include "app/rendererbase.h"

// 0x00139d70
RendererBase::Router::~Router() {
}

// 0x00139f50
void RendererBase::Router::Handle(Message *pMsg) {
    mTarget->HandleMessage(pMsg);
}

// 0x00139f48
void RendererBase::Router::HandleMessage(Message *) {
}
