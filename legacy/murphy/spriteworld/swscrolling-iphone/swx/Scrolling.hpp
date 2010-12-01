
//---------------------------------------------------------------------------------------
/// CLASS TSpriteWorldScrolling, wrapping SpriteWorldPtr (scrolling)
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TSpriteWorldScrolling : protected TSpriteWorldTiling {
public:
        // TO DO - CTOR, DTOR, CopyCTOR, Assign.
        // (This area is only slightly finished.)

    void update() throw();
    void updateWorld(bool inUpdateWindow) throw();
    void process() throw();
    void animate() throw();
    void fastAnimate() throw();
    void wrapRectToScreen(const SWRect& inSrcRect) throw();
    void moveBounds(const SWRect& inScrollRectMoveBounds) throw();
    void scrollDelta(short inHorizDelta, short inVertDelta) throw();
    void moveVisScrollRect(short inHorizPos, short inVertPos) throw();
    void offsetVisScrollRect(short inHorizOffset, short inVertOffset) throw();
    void resizeVisScrollRect(short inHorizSize, short inVertSize) throw();
    void flagScrollingRectAsChanged(const SWRect& inChangedRect) throw();
    };

inline void TSpriteWorldScrolling::update()
throw() {
    SWUpdateScrollingWindow(mWorld);
    }

inline void TSpriteWorldScrolling::updateWorld(bool inUpdateWindow)
throw() {
    SWUpdateScrollingSpriteWorld(mWorld, inUpdateWindow);
    }

inline void TSpriteWorldScrolling::process()
throw() {
    SWProcessScrollingSpriteWorld(mWorld);
    }

inline void TSpriteWorldScrolling::animate()
throw() {
    SWAnimateScrollingSpriteWorld(mWorld);
    }

inline void TSpriteWorldScrolling::fastAnimate()
throw() {
    SWFastAnimateScrollingSpriteWorld(mWorld);
    }

inline void TSpriteWorldScrolling::wrapRectToScreen(const SWRect& inSrcRect)
throw() {
    SWWrapRectToScreen(mWorld, const_cast<SWRect*>(&inSrcRect));
    }

inline void TSpriteWorldScrolling::moveBounds(const SWRect& inScrollRectMoveBounds)
throw() {
    SWSetScrollingWorldMoveBounds(mWorld, const_cast<SWRect*>(&inScrollRectMoveBounds));
    }

inline void TSpriteWorldScrolling::scrollDelta(short inHorizDelta, short inVertDelta)
throw() {
    SWSetSpriteWorldScrollDelta(mWorld, inHorizDelta, inVertDelta);
    }

inline void TSpriteWorldScrolling::moveVisScrollRect(short inHorizPos, short inVertPos)
throw() {
    SWMoveVisScrollRect(mWorld, inHorizPos, inVertPos);
    }

inline void TSpriteWorldScrolling::offsetVisScrollRect(short inHorizOffset, short inVertOffset)
throw() {
    SWOffsetVisScrollRect(mWorld, inHorizOffset, inVertOffset);
    }

inline void TSpriteWorldScrolling::resizeVisScrollRect(short inHorizSize, short inVertSize)
throw() {
    SWResizeVisScrollRect(mWorld, inHorizSize, inVertSize);
    }


