#ifndef CK_SCRIPTREFCOUNTED_H
#define CK_SCRIPTREFCOUNTED_H

class ScriptRefCounted {
public:
    void AddRef() const {
        ++m_RefCount;
    }

    void Release() const {
        if (--m_RefCount == 0) {
            delete this;
        }
    }

protected:
    virtual ~ScriptRefCounted() = default;

private:
    mutable int m_RefCount = 1;
};

using RefCounted = ScriptRefCounted;

#endif // CK_SCRIPTREFCOUNTED_H
