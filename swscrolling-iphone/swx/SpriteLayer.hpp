
//---------------------------------------------------------------------------------------
/// CLASS TSpriteLayer, wrapping SpriteLayerPtr
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TSpriteLayer {
protected:
    SpriteLayerPtr mLayer;

private:
        // Automagic conversions.
    operator SpriteLayerPtr () throw() { return mLayer; }
    operator SpriteLayerPtr* () throw() { return &mLayer; }

public:
        // Complete access into our secret innards.
    SpriteLayerPtr ptr() throw();
    
        // CTOR, DTOR, CopyCTOR, Assign.
    TSpriteLayer() throw() : mLayer(NULL) {}
    TSpriteLayer(SpriteLayerPtr inLayer) throw() : mLayer(inLayer) {}
    TSpriteLayer(const TSpriteLayer& inLayer) throw() : mLayer(inLayer.mLayer) {}
    ~TSpriteLayer() throw() {}
    const TSpriteLayer& operator = (const TSpriteLayer& inLayer) throw();

        // Memory management.
    void dispose() throw();
    void lock() throw();
    void unlock() throw();

        // Sprite control.
    void add(TSprite& inNewSprite) throw();
    void remove(TSprite& inOldSprite) throw();
    void removeAllSprites() throw();
    void swap(TSprite& inSrcSprite,TSprite& inDstSprite) throw();
    void insertAfter(TSprite& inNewSprite,TSprite& inDstSprite) throw();
    void insertBefore(TSprite& inNewSprite,TSprite& inDstSprite) throw();
    TSprite findSpriteByPoint(TSprite& inStartSprite, SWPoint inTestPoint) throw();

        // Class functions.
    static TSpriteLayer Create() throw(std::runtime_error);
    };


inline SpriteLayerPtr TSpriteLayer::ptr()
throw() {
    return mLayer;
    }

inline const TSpriteLayer& TSpriteLayer::operator = (const TSpriteLayer& inLayer)
throw() {
    mLayer = inLayer.mLayer;
    return *this;
    }

/// @see SWDisposeSpriteLayer
inline void TSpriteLayer::dispose()
throw() {
    SWDisposeSpriteLayer(&mLayer);
    }

/// @see SWLockSpriteLayer
inline void TSpriteLayer::lock()
throw() {
    SWLockSpriteLayer(mLayer);
    }

/// @see SWUnlockSpriteLayer
inline void TSpriteLayer::unlock()
throw() {
    SWUnlockSpriteLayer(mLayer);
    }

inline void TSpriteLayer::add(TSprite& inSprite)
throw() {
    SWAddSprite(mLayer, inSprite.ptr());
    }

inline void TSpriteLayer::remove(TSprite& inSprite)
throw() {
    SWRemoveSprite(inSprite.ptr());
    }

inline void TSpriteLayer::removeAllSprites()
throw() {
    SWRemoveAllSpritesFromLayer(mLayer);
    }

inline void TSpriteLayer::swap(TSprite& inSrcSprite,TSprite& inDstSprite)
throw() {
    SWSwapSprite(inSrcSprite.ptr(), inDstSprite.ptr());
    }

inline void TSpriteLayer::insertAfter(TSprite& inNewSprite,TSprite& inDstSprite)
throw() {
    SWInsertSpriteAfterSprite(inNewSprite.ptr(), inDstSprite.ptr());
    }

inline void TSpriteLayer::insertBefore(TSprite& inNewSprite,TSprite& inDstSprite)
throw() {
    SWInsertSpriteBeforeSprite(inNewSprite.ptr(), inDstSprite.ptr());
    }

inline TSprite TSpriteLayer::findSpriteByPoint(TSprite& inStartSprite, SWPoint inTestPoint)
throw() {
    return TSprite(SWFindSpriteByPoint(mLayer, inStartSprite.ptr(), inTestPoint.h, inTestPoint.v));
    }

inline TSpriteLayer TSpriteLayer::Create()
throw(std::runtime_error) {
    TSpriteLayer theLayer;
    SWError err = SWCreateSpriteLayer(theLayer);
    if(err) throw TError("TSpriteLayer::Create", err);
    return theLayer;
    }

