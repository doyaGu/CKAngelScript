#ifndef CK_SCRIPTREFCOUNTED_H
#define CK_SCRIPTREFCOUNTED_H

class ScriptRefCounted {
public:
    void AddRef() const {
        m_GCFlag = false;
        ++m_RefCount;
    }

    void Release() const {
        m_GCFlag = false;
        if (--m_RefCount == 0) {
            delete this;
        }
    }

    int GetRefCount() const {
        return m_RefCount;
    }

    void SetGCFlag() const {
        m_GCFlag = true;
    }

    bool GetGCFlag() const {
        return m_GCFlag;
    }

protected:
    virtual ~ScriptRefCounted() = default;

private:
    mutable int m_RefCount = 1;
    mutable bool m_GCFlag = false;
};

using RefCounted = ScriptRefCounted;

#endif // CK_SCRIPTREFCOUNTED_H
