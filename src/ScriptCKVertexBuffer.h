#ifndef CK_SCRIPTCKVERTEXBUFFER_H
#define CK_SCRIPTCKVERTEXBUFFER_H

#include "CKAll.h"

#include "ScriptRefCounted.h"

class CScriptArray;

class ScriptCKVertexBuffer final : public ScriptRefCounted {
public:
    ScriptCKVertexBuffer(CKRenderManager *manager, CKVertexBuffer *buffer);

    bool IsValid() const;

    void Destroy();

    CKVB_STATE Check(CKRenderContext *ctx, CKDWORD maxVertexCount, CKRST_DPFLAGS format, bool dynamic);
    bool Lock(CKRenderContext *ctx,
              CKDWORD startVertex,
              CKDWORD vertexCount,
              VxDrawPrimitiveData &data,
              CKLOCKFLAGS lockFlags);
    void Unlock(CKRenderContext *ctx);
    bool Draw(CKRenderContext *ctx, VXPRIMITIVETYPE pType, CKDWORD startVertex, CKDWORD vertexCount);
    bool DrawIndexed(CKRenderContext *ctx,
                     VXPRIMITIVETYPE pType,
                     const CScriptArray &indices,
                     CKDWORD startVertex,
                     CKDWORD vertexCount);

protected:
    ~ScriptCKVertexBuffer() override;

private:
    friend void InvalidateScriptCKVertexBuffersForManager(CKRenderManager *manager);

    void InvalidateNativeDestroyed();
    CKVertexBuffer *GetNative() const;

    CKRenderManager *m_Manager = nullptr;
    CKVertexBuffer *m_Buffer = nullptr;
};

ScriptCKVertexBuffer *CreateScriptCKVertexBuffer(CKRenderManager *manager);
void DestroyScriptCKVertexBuffer(CKRenderManager *manager, ScriptCKVertexBuffer *buffer);
void InvalidateScriptCKVertexBuffersForManager(CKRenderManager *manager);

#endif // CK_SCRIPTCKVERTEXBUFFER_H
