#include "ScriptCKVertexBuffer.h"

#include <algorithm>
#include <limits.h>
#include <mutex>
#include <vector>

#include "add_on/scriptarray/scriptarray.h"

namespace {

std::mutex g_CKVertexBufferMutex;
std::vector<ScriptCKVertexBuffer *> g_CKVertexBuffers;

} // namespace

ScriptCKVertexBuffer::ScriptCKVertexBuffer(CKRenderManager *manager, CKVertexBuffer *buffer)
    : m_Manager(manager), m_Buffer(buffer) {
    std::lock_guard<std::mutex> lock(g_CKVertexBufferMutex);
    g_CKVertexBuffers.push_back(this);
}

ScriptCKVertexBuffer::~ScriptCKVertexBuffer() {
    Destroy();
    std::lock_guard<std::mutex> lock(g_CKVertexBufferMutex);
    g_CKVertexBuffers.erase(std::remove(g_CKVertexBuffers.begin(), g_CKVertexBuffers.end(), this),
                            g_CKVertexBuffers.end());
}

bool ScriptCKVertexBuffer::IsValid() const {
    return m_Manager != nullptr && m_Buffer != nullptr;
}

CKVertexBuffer *ScriptCKVertexBuffer::GetNative() const {
    return IsValid() ? m_Buffer : nullptr;
}

void ScriptCKVertexBuffer::Destroy() {
    CKRenderManager *manager = m_Manager;
    CKVertexBuffer *buffer = m_Buffer;
    InvalidateNativeDestroyed();

    if (manager && buffer) {
        manager->DestroyVertexBuffer(buffer);
    }
}

void ScriptCKVertexBuffer::InvalidateNativeDestroyed() {
    m_Manager = nullptr;
    m_Buffer = nullptr;
}

CKVB_STATE ScriptCKVertexBuffer::Check(CKRenderContext *ctx,
                                       CKDWORD maxVertexCount,
                                       CKRST_DPFLAGS format,
                                       bool dynamic) {
    CKVertexBuffer *buffer = GetNative();
    if (!buffer || !ctx) {
        return CK_VB_FAILED;
    }
    return buffer->Check(ctx, maxVertexCount, format, dynamic ? TRUE : FALSE);
}

bool ScriptCKVertexBuffer::Lock(CKRenderContext *ctx,
                                CKDWORD startVertex,
                                CKDWORD vertexCount,
                                VxDrawPrimitiveData &data,
                                CKLOCKFLAGS lockFlags) {
    data = VxDrawPrimitiveData();
    CKVertexBuffer *buffer = GetNative();
    if (!buffer || !ctx) {
        return false;
    }

    VxDrawPrimitiveData *locked = buffer->Lock(ctx, startVertex, vertexCount, lockFlags);
    if (!locked) {
        return false;
    }
    data = *locked;
    return true;
}

void ScriptCKVertexBuffer::Unlock(CKRenderContext *ctx) {
    CKVertexBuffer *buffer = GetNative();
    if (buffer && ctx) {
        buffer->Unlock(ctx);
    }
}

bool ScriptCKVertexBuffer::Draw(CKRenderContext *ctx,
                                VXPRIMITIVETYPE pType,
                                CKDWORD startVertex,
                                CKDWORD vertexCount) {
    CKVertexBuffer *buffer = GetNative();
    if (!buffer || !ctx) {
        return false;
    }
    return buffer->Draw(ctx, pType, nullptr, 0, startVertex, vertexCount) != FALSE;
}

bool ScriptCKVertexBuffer::DrawIndexed(CKRenderContext *ctx,
                                       VXPRIMITIVETYPE pType,
                                       const CScriptArray &indices,
                                       CKDWORD startVertex,
                                       CKDWORD vertexCount) {
    CKVertexBuffer *buffer = GetNative();
    if (!buffer || !ctx) {
        return false;
    }
    if (indices.GetElementTypeId() != asTYPEID_UINT16) {
        return false;
    }

    const asUINT count = indices.GetSize();
    if (count == 0 || count > static_cast<asUINT>(INT_MAX)) {
        return false;
    }

    CKWORD *nativeIndices = static_cast<CKWORD *>(const_cast<CScriptArray &>(indices).GetBuffer());
    return buffer->Draw(ctx, pType, nativeIndices, static_cast<int>(count), startVertex, vertexCount) != FALSE;
}

ScriptCKVertexBuffer *CreateScriptCKVertexBuffer(CKRenderManager *manager) {
    if (!manager) {
        return nullptr;
    }

    CKVertexBuffer *buffer = manager->CreateVertexBuffer();
    if (!buffer) {
        return nullptr;
    }
    return new ScriptCKVertexBuffer(manager, buffer);
}

void DestroyScriptCKVertexBuffer(CKRenderManager *, ScriptCKVertexBuffer *buffer) {
    if (buffer) {
        buffer->Destroy();
    }
}

void InvalidateScriptCKVertexBuffersForManager(CKRenderManager *manager) {
    if (!manager) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_CKVertexBufferMutex);
    for (ScriptCKVertexBuffer *buffer : g_CKVertexBuffers) {
        if (buffer && buffer->m_Manager == manager) {
            buffer->InvalidateNativeDestroyed();
        }
    }
}
