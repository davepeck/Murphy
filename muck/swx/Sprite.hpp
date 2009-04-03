
//--------------------------------------------------------------------------------------
/// CLASS TSprite, wrapping SpritePtr
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//--------------------------------------------------------------------------------------
class TSprite {
protected:
    SpritePtr mSprite;

private:
        // Magic conversions.
    operator SpritePtr () throw() { return mSprite; }
    operator SpritePtr* () throw() { return &mSprite; }

public:
        // Complete access into our secret innards.
    SpritePtr ptr() throw();

        // CTOR, DTOR, CopyCTOR
    TSprite() throw() : mSprite(NULL) {}
    TSprite(SpritePtr inSprite) throw() : mSprite(inSprite) {}
    TSprite(const TSprite& inSprite) throw() : mSprite(inSprite.mSprite) {}
    ~TSprite() throw() {}

        // Memory management.
    void create(short maxFrames) throw(std::runtime_error);
    void createFromFile(const char * file, short maxFrames) throw(std::runtime_error);

    void dispose() throw();
    void lock() throw();
    void unlock() throw();
    TSprite clone() const throw(std::runtime_error);

        // Frame Controls
    void add(TFrame& newFrameP) throw();
    void insert(TFrame& newFrameP , short index) throw();
    void remove(TFrame& oldFrameP) throw();
    void current(TFrame& curFrameP) throw();
    void frameAdvance(short inFrameAdvance) throw();
    void frameAdvanceMode(AdvanceType inFrameAdvanceMode) throw();
    void frameCurrent(short inFrameIndex) throw();
    void frameRange(short inFirstFrameIndex, short inLastFrameIndex) throw();
    void frameTime(long inFrameTimeInterval) throw();
    
        // Manipulators.
    void move(SWMovementType inHorizLoc, SWMovementType inVertLoc) throw();
    void offset(SWMovementType inHorizOffset, SWMovementType inVertOffset) throw();
    void location(SWMovementType inHorizLoc, SWMovementType inVertLoc) throw();
    void moveBounds(const SWRect& inMoveBoundsRect) throw();
    void moveDelta(SWMovementType inHorizDelta, SWMovementType inVertDelta) throw();
    void moveTime(long inTimeInterval) throw();
    void bounce() throw();
    void wrap() throw();
    bool radiusCollision(TSprite& inDstSprite) throw();
    bool pixelCollision(TSprite& inDstSprite) throw();
    void visible(bool inIsVisible) throw();
    bool within(const SWRect& inTestRect) throw();
    bool fullyWithin(const SWRect& inTestRect) throw();
    bool contain(const SWPoint& inTestPoint) throw();

        // Set callbacks.
    void drawProc(DrawProcPtr inDrawProc) throw();
    void moveProc(MoveProcPtr inMoveProc) throw();

        // Accessor methods beyond the opaque veil.
    const SWMovementType horizMoveDelta() const throw();
    const SWMovementType vertMoveDelta() const throw();
    };

inline SpritePtr TSprite::ptr()
throw() {
    return mSprite;
    }

inline void TSprite::create(short inMaxFrames)
throw(std::runtime_error) {
    SWError err = SWCreateSprite(&mSprite, NULL, inMaxFrames);
    if(err) throw TError("TSprite::CreateSprite", err);
    }


inline void TSprite::createFromFile(const char * file, short inMaxFrames)
throw(std::runtime_error) {
    SWError err = SWCreateSpriteFromFile(&mSprite, NULL, file, inMaxFrames);
    if(err) throw TError("TSprite::CreateSpriteFromFile", err);
    }


inline void TSprite::dispose()
throw() {
    SWDisposeSprite(&mSprite);
    }

/// @see SWLockSprite
inline void TSprite::lock()
throw() {
    SWLockSprite(mSprite);
    }

/// @see SWUnlockSprite
inline void TSprite::unlock()
throw() {
    SWUnlockSprite(mSprite);
    }

    // Deep copy.
inline TSprite TSprite::clone() const
throw(std::runtime_error) {
    TSprite theNewSprite;  // The destination, duplicated sprite.
    SWError err = SWCloneSprite(mSprite, theNewSprite, NULL);
    if(err) throw TError("TSprite::clone", err);
    return theNewSprite;
    }

inline void TSprite::add(TFrame& inNewFrame)
throw() {
    SWAddFrame(mSprite, inNewFrame.ptr());
    }

inline void TSprite::insert(TFrame& inNewFrame , short index)
throw() {
    SWInsertFrame(mSprite, inNewFrame.ptr() , index);
    }

inline void TSprite::remove(TFrame& inOldFrame)
throw() {
    SWRemoveFrame(mSprite, inOldFrame.ptr());
    }

inline void TSprite::current(TFrame& inFrame)
throw() {
    SWSetCurrentFrame(mSprite, inFrame.ptr());
    }


inline void TSprite::frameAdvance(short inFrameAdvance)
throw() {
    SWSetSpriteFrameAdvance(mSprite, inFrameAdvance);
    }

inline void TSprite::frameAdvanceMode(AdvanceType inFrameAdvanceMode)
throw() {
    SWSetSpriteFrameAdvanceMode(mSprite, inFrameAdvanceMode);
    }

inline void TSprite::frameCurrent(short inFrameIndex)
throw() {
    SWSetCurrentFrameIndex(mSprite, inFrameIndex);
    }

inline void TSprite::frameRange(short inFirstFrameIndex, short inLastFrameIndex)
throw() {
    SWSetSpriteFrameRange(mSprite, inFirstFrameIndex, inLastFrameIndex);
    }

inline void TSprite::frameTime(long inFrameTimeInterval)
throw() {
    SWSetSpriteFrameTime(mSprite, inFrameTimeInterval);
    }

inline void TSprite::move(SWMovementType inHorizLoc, SWMovementType inVertLoc)
throw() {
    SWMoveSprite(mSprite, inHorizLoc, inVertLoc);
    }

inline void TSprite::offset(SWMovementType inHorizOffset, SWMovementType inVertOffset)
throw() {
    SWOffsetSprite(mSprite, inHorizOffset, inVertOffset);
    }

inline void TSprite::location(SWMovementType inHorizLoc, SWMovementType inVertLoc)
throw() {
    SWSetSpriteLocation(mSprite, inHorizLoc, inVertLoc);
    }

inline void TSprite::moveBounds(const SWRect& inMoveBoundsRect)
throw() {
    SWSetSpriteMoveBounds(mSprite, const_cast<SWRect*>(&inMoveBoundsRect));
    }

inline void TSprite::moveDelta(SWMovementType inHorizDelta, SWMovementType inVertDelta)
throw() {
    SWSetSpriteMoveDelta(mSprite, inHorizDelta, inVertDelta);
    }

inline void TSprite::moveTime(long inTimeInterval)
throw() {
    SWSetSpriteMoveTime(mSprite, inTimeInterval);
    }

inline void TSprite::bounce()
throw() {
    SWBounceSprite(mSprite);
    }

inline void TSprite::wrap()
throw() {
    SWWrapSprite(mSprite);
    }

inline bool TSprite::radiusCollision(TSprite& inDstSprite)
throw() {
    return SWRadiusCollision(mSprite, inDstSprite);
    }

inline bool TSprite::pixelCollision(TSprite& inDstSprite)
throw() {
    return SWPixelCollision(mSprite, inDstSprite);
    }

inline void TSprite::visible(bool inIsVisible)
throw() {
    SWSetSpriteVisible(mSprite, inIsVisible);
    }

inline bool TSprite::within(const SWRect& inTestRect)
throw() {
    return SWIsSpriteInRect(mSprite, const_cast<SWRect*>(&inTestRect));
    }

inline bool TSprite::fullyWithin(const SWRect& inTestRect)
throw() {
    return SWIsSpriteFullyInRect(mSprite, const_cast<SWRect*>(&inTestRect));
    }

inline bool TSprite::contain(const SWPoint& inTestPoint)
throw() {
    return SWIsPointInSprite(mSprite, inTestPoint.h, inTestPoint.v);
    }

inline void TSprite::drawProc(DrawProcPtr inDrawProc)
throw() {
    SWSetSpriteDrawProc(mSprite, inDrawProc);
    }

inline void TSprite::moveProc(MoveProcPtr inMoveProc)
throw() {
    SWSetSpriteMoveProc(mSprite, inMoveProc);
    }

inline const SWMovementType TSprite::horizMoveDelta() const
throw() {
    return ((SpritePtr) mSprite)->drawData->horizMoveDelta;
    }

inline const SWMovementType TSprite::vertMoveDelta() const
throw() {
    return ((SpritePtr) mSprite)->drawData->vertMoveDelta;
    }

