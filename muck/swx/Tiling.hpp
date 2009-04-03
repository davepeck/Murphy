
//---------------------------------------------------------------------------------------
/// CLASS TTileMap, wrapping TileMapStructPtr
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TTileMap {
private:
    TileMapStructPtr mTileMap;

private:
        // Automagic conversions.
    operator TileMapStructPtr () throw() { return mTileMap; }
    operator TileMapStructPtr* () throw() { return &mTileMap; }

public:
        // Complete access into our secret innards.
    TileMapStructPtr ptr() throw();

        // CTOR, CopyCTOR, DTOR, Assign.
    TTileMap() throw() : mTileMap(NULL) {}
    TTileMap(TileMapStructPtr inTileMap) throw() : mTileMap(inTileMap) {}
    TTileMap(const TTileMap& inTileMap) throw() : mTileMap(inTileMap.mTileMap) {}
    ~TTileMap() throw() {}
    const TTileMap& operator = (const TTileMap& inTTileMap) throw();

        // Memory management.
    void create(short numTileMapRows, short numTileMapCols) throw(std::runtime_error);
    void dispose() throw();
    void lock() throw();
    void unlock() throw();

        // I/O management.
    void load(char *file) throw();
    void save(char *file) throw();
    };


inline TileMapStructPtr TTileMap::ptr()
throw() {
    return mTileMap;
    }

inline const TTileMap& TTileMap::operator = (const TTileMap& inTileMap)
throw() {
    mTileMap = inTileMap.mTileMap;
    return *this;
    }

inline void TTileMap::create(short inRows, short inCols)
throw(std::runtime_error) {
    SWError err = SWCreateTileMap(&mTileMap, inRows, inCols);
    if(err) throw std::runtime_error("TTileMap::CreateTileMap");
    }

inline void TTileMap::dispose()
throw() {
    SWDisposeTileMap(&mTileMap);
    }

inline void TTileMap::lock()
throw() {
    SWLockTileMap(mTileMap);
    }

inline void TTileMap::unlock()
throw() {
    SWUnlockTileMap(mTileMap);
    }

inline void TTileMap::load(char *file)
throw() {
    SWLoadTileMap(file, &mTileMap);
    }

inline void TTileMap::save(char *file)
throw() {
    SWSaveTileMap(file, mTileMap);
    }


//---------------------------------------------------------------------------------------
/// CLASS TSpriteWorldTiling, wrapping SpriteWorldPtr (tiling)
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TSpriteWorldTiling : protected TSpriteWorld {
protected:
    TTileMap mTileMap;

public:
        // TO DO - CTOR, DTOR, CopyCTOR, Assign.
        // (This area is only slightly finished.)

        // Book ends.
    void initTiling(short inWidth, short inHeight, short inMaxNumTiles) throw(std::runtime_error);
    void exitTiling() throw();

        // Various routines.
    void installTileMap(TTileMap& inTileMap, short inTileLayer) throw();
    void setTilingOn(bool inTilingIsOn) throw();
    void drawTilesInBackground() throw();
    void resetTilingCache() throw();
    void drawTile(short inRow,short inCol, short inTileId, short inTileLayer) throw();
    void updateTileOnScreen(short inTileId) throw();
    void drawTilesInRect(const SWRect& inChangedRect, bool inFaster) throw();
    void addChangedRect(const SWRect& inChangedRect) throw();
    void drawTilesAboveSprite(const SWRect& inUpdateRect, short inTileLayer) throw();
    short returnTileUnderPixel(short inTileLayer, short inPixelCol, short inPixelRow) throw();
    void wrapRectToWorkArea(const SWRect& inDstRect) throw();
    bool checkSpriteWithTiles(TSprite& inSrcSprite, SWTileSearchType inSearchType, const SWRect& inInsetRect, short inFirstTileLayer, short inLastTileLayer, short inFirstTileId, short inLastTileId, bool inFixPosition) throw();

        // Memory management.
    void dispose(short inTileId) throw();
    void lock() throw();
    void unlock() throw();
    };

inline void TSpriteWorldTiling::initTiling(short inTileWidth, short inTileHeight, short inMaxNumTiles)
throw(std::runtime_error) {
    SWError err = SWInitTiling(mWorld, inTileHeight, inTileWidth, inMaxNumTiles);
    if(err) throw std::runtime_error("TSpriteWorldTiling::InitTiling");
    }

inline void TSpriteWorldTiling::exitTiling()
throw() {
    SWExitTiling(mWorld);
    }

inline void TSpriteWorldTiling::installTileMap(TTileMap& inTileMap, short inTileLayer)
throw() {
    SWInstallTileMap(mWorld, inTileMap.ptr(), inTileLayer);
    mTileMap = inTileMap;
    }

inline void TSpriteWorldTiling::setTilingOn(bool inTilingIsOn)
throw() {
    SWSetTilingOn(mWorld, inTilingIsOn);
    }

inline void TSpriteWorldTiling::drawTilesInBackground()
throw() {
    SWDrawTilesInBackground(mWorld);
    }

inline void TSpriteWorldTiling::resetTilingCache()
throw() {
    SWResetTilingCache(mWorld);
    }

inline void TSpriteWorldTiling::drawTile(short inTileRow,short inTileCol, short inTileId, short inTileLayer)
throw() {
    SWDrawTile(mWorld, inTileRow, inTileCol, inTileId, inTileLayer);
    }

inline void TSpriteWorldTiling::updateTileOnScreen(short inTileId)
throw() {
    SWUpdateTileOnScreen(mWorld, inTileId);
    }

inline void TSpriteWorldTiling::drawTilesInRect(const SWRect& inChangedRect, bool inFaster)
throw() {
    SWDrawTilesInRect(mWorld, const_cast<SWRect*>(&inChangedRect), inFaster);
    }

inline void TSpriteWorldTiling::addChangedRect(const SWRect& inChangedRect)
throw() {
    SWAddChangedRect(mWorld, const_cast<SWRect*>(&inChangedRect));
    }

inline void TSpriteWorldTiling::drawTilesAboveSprite(const SWRect& inUpdateRect, short inTileLayer)
throw() {
    SWDrawTilesAboveSprite(mWorld, const_cast<SWRect*>(&inUpdateRect), inTileLayer);
    }

inline short TSpriteWorldTiling::returnTileUnderPixel(short inTileLayer, short inPixelCol, short inPixelRow)
throw() {
    return SWReturnTileUnderPixel(mWorld, inTileLayer, inPixelCol, inPixelRow);
    }

inline void TSpriteWorldTiling::wrapRectToWorkArea(const SWRect& inDstRect)
throw() {
    SWWrapRectToWorkArea(mWorld, const_cast<SWRect*>(&inDstRect));
    }

inline void TSpriteWorldTiling::dispose(short inTileId)
throw() {
    SWDisposeTile(mWorld, inTileId);
    }

inline void TSpriteWorldTiling::lock()
throw() {
    SWLockTiles(mWorld);
    }

inline void TSpriteWorldTiling::unlock()
throw() {
    SWUnlockTiles(mWorld);
    }

inline bool TSpriteWorldTiling::checkSpriteWithTiles(TSprite& inSrcSprite, SWTileSearchType inSearchType, const SWRect& inInsetRect, short inFirstTileLayer, short inLastTileLayer, short inFirstTileID, short inLastTileID, bool inFixPosition)
throw() {
    return SWCheckSpriteWithTiles(mWorld, inSrcSprite.ptr(), inSearchType, const_cast<SWRect*>(&inInsetRect), inFirstTileLayer, inLastTileLayer, inFirstTileID, inLastTileID, inFixPosition);
    }

