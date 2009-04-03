
//---------------------------------------------------------------------------------------
/// CLASS TFrame, wrapping FramePtr
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TFrame {
protected:
    FramePtr mFrame;

private:
        // Automagic conversions.
    operator FramePtr () throw() { return mFrame; }
    operator FramePtr* () throw() { return &mFrame; }

public:
        // Complete access into our secret innards.
    FramePtr ptr() throw();

        // CTOR, DTOR, CopyCTOR, Assign.
    TFrame() throw() : mFrame(NULL) {}
    TFrame(FramePtr inFrame) throw() : mFrame(inFrame) {}
    TFrame(const TFrame& inFrame) throw() : mFrame(inFrame.mFrame) {}
    ~TFrame() throw(){}
    const TFrame& operator = (const TFrame& inFrame) throw();

        // Memory management.
    bool dispose() throw();
    void lock() throw();
    void unlock() throw();

        // Class functions.
    static TFrame Create() throw(std::runtime_error);
    static TFrame CreateBlank(int w, int h, unsigned char depth, bool alpha) throw(std::runtime_error);
    };

inline FramePtr TFrame::ptr()
throw() {
    return mFrame;
    }

inline const TFrame& TFrame::operator = (const TFrame& inFrame)
throw() {
    mFrame = inFrame.mFrame;
    return *this;
    }

/// @see SWDisposeFrame
inline bool TFrame::dispose()
throw() {
    return SWDisposeFrame(&mFrame) ? true : false;
    }

/// @see SWLockFrame
inline void TFrame::lock()
throw() {
    SWLockFrame(mFrame);
    }

/// @see SWUnlockFrame
inline void TFrame::unlock()
throw() {
    SWUnlockFrame(mFrame);
    }

/// @see SWCreateFrame
inline TFrame TFrame::Create()
throw(std::runtime_error) {
    TFrame theFrame;
    SWError err = SWCreateFrame(theFrame);
    if(err) throw TError("TFrame::Create", err);
    return theFrame;
    }

/// @see SWCreateBlankFrame
inline TFrame TFrame::CreateBlank(int w, int h, unsigned char depth, bool alpha)
throw(std::runtime_error) {
    TFrame theFrame;
    SWError err = SWCreateBlankFrame(theFrame, w, h, depth, alpha);
    if(err) throw TError("TFrame::CreateBlank", err);
    return theFrame;
    }

