
//---------------------------------------------------------------------------------------
/// CLASS TSpriteWorld, wrapping SpriteWorldPtr
/// @ingroup Classes
// Non-virtual destructor ... do not publically derive!
//---------------------------------------------------------------------------------------
class TSpriteWorld {
protected:
    SpriteWorldPtr mWorld;

private:
        // Automagic conversions.
    operator SpriteWorldPtr () throw() { return mWorld; }
    operator SpriteWorldPtr* () throw() { return &mWorld; }

public:
        // Complete access into our secret innards.
    SpriteWorldPtr ptr() throw();

        // CTOR, DTOR, CopyCTOR, Assign.
    TSpriteWorld() throw() : mWorld(NULL) {}
    TSpriteWorld(SpriteWorldPtr inWorld) throw() : mWorld(inWorld) {}
    TSpriteWorld(const TSpriteWorld& inWorld) throw()  : mWorld(inWorld.mWorld) {}
    ~TSpriteWorld() throw() {}
    const TSpriteWorld& operator = (const TSpriteWorld& inWorld) throw();

        // Memory management.
    void dispose() throw();
    void lock() throw();
    void unlock() throw();

        // Change and manipulate canvases.
    void copyBackgroundToWorkArea() throw();

        // Action.
//  void updateWindow() throw();
    void update(bool inUpdateWindow) throw();
    void process() throw();
    void animate() throw();

        // Gestalt states.
    void maxFps(short inFramesPerSec) throw();

        // Layer associations.
    void add(TSpriteLayer& inLayer) throw();
    void remove(TSpriteLayer& inLayer) throw();
    void swap(TSpriteLayer& inSrcLayer, TSpriteLayer& inDsTSpriteLayer) throw();
    TSpriteLayer next(TSpriteLayer& inLayer) throw();

        // Factory methods.
    TSprite createSpriteFromFile(const char *file, long inMaxFrames) throw(std::runtime_error);
    DrawDataPtr createDrawData() throw();

        // Supplemental methods.
    void collide(TSpriteLayer& inLayerSrc, TSpriteLayer& inLayerDst) throw();

        // Accessor methods beyond the opaque veil.
    const short pixelDepth() const throw();
    const unsigned long milliSeconds() const throw();
    TFrame workFrame() const throw();
    const SWRect& backRect() const throw();
    const bool frameHasOccurred() const throw();

        // Class functions.
    static unsigned long GetSpriteWorldVersion() throw();
    static void Enter(bool shouldInit) throw(std::runtime_error);
    static void Exit() throw();
    static TSpriteWorld CreateSpriteWorld(int w, int h, int depth, bool fullscreen, bool opengl) throw(std::runtime_error);
    static TSpriteWorld CreateSpriteWorldFromVideoSurface(SDL_Surface* surface, SWRect& world, SWRect& back, int maxDepth) throw(std::runtime_error);
    static TSpriteWorld CreateSpriteWorldFromSWSurface(SDL_Surface* surface, SWRectPtr world = NULL, SWRectPtr back = NULL, int maxDepth = 0) throw(std::runtime_error);
    static TSpriteWorld CreateSpriteWorldFromGLContext(void* glContext, SWRect& world, GLCallBackPtr glMakeCurrent, GLCallBackPtr glSwapBuffers) throw(std::runtime_error);
    static TSpriteWorld CreateSpriteWorldFromFrames(TFrame& screen, TFrame& back, TFrame& work) throw(std::runtime_error);

    };


inline SpriteWorldPtr TSpriteWorld::ptr()
throw() {
    return mWorld;
    }

inline const TSpriteWorld& TSpriteWorld::operator = (const TSpriteWorld& inWorld)
throw() {
    mWorld = inWorld.mWorld;
    return *this;
    }

/// @see SWGetSpriteWorldVersion
inline unsigned long TSpriteWorld::GetSpriteWorldVersion()
throw() {
    return SWGetSpriteWorldVersion();
    }

/// @see SWEnterSpriteWorld
inline void TSpriteWorld::Enter(bool shouldInit)
throw(std::runtime_error) {
    SWError err = SWEnterSpriteWorld(shouldInit);
    if(err) throw TError("TSpriteWorld::Enter", err);
    }

/// @see SWExitSpriteWorld
inline void TSpriteWorld::Exit()
throw() {
    SWExitSpriteWorld();
    }

/// @see SWCreateSpriteWorld
inline TSpriteWorld TSpriteWorld::CreateSpriteWorld(int w, int h, int depth, bool fullscreen, bool opengl)
throw(std::runtime_error) {
    TSpriteWorld theWorld;
    SWError err = SWCreateSpriteWorld(theWorld, w, h, depth, fullscreen, opengl);
    if(err) throw TError("TSpriteWorld::CreateSpriteWorld", err);
    return theWorld;
    }

/// @see SWCreateSpriteWorldFromVideoSurface
inline TSpriteWorld TSpriteWorld::CreateSpriteWorldFromVideoSurface(SDL_Surface* surface, SWRect& world, SWRect& back, int maxDepth)
throw(std::runtime_error) {
    TSpriteWorld theWorld;
    SWError err = SWCreateSpriteWorldFromVideoSurface(theWorld, surface, &world, &back, maxDepth);
    if(err) throw TError("TSpriteWorld::CreateSpriteWorldFromVideoSurface", err);
    return theWorld;
    }

/// @see SWCreateSpriteWorldFromSWSurface
inline TSpriteWorld TSpriteWorld::CreateSpriteWorldFromSWSurface(SDL_Surface* surface, SWRectPtr world, SWRectPtr back, int maxDepth)
throw(std::runtime_error) {
    TSpriteWorld theWorld;
    SWError err = SWCreateSpriteWorldFromSWSurface(theWorld, surface, world, back, maxDepth);
    if(err) throw TError("TSpriteWorld::CreateSpriteWorldFromSWSurface", err);
    return theWorld;
    }

/// @see SWCreateSpriteWorldFromGLContext
inline TSpriteWorld TSpriteWorld::CreateSpriteWorldFromGLContext(void* glContext, SWRect& world, GLCallBackPtr glMakeCurrent, GLCallBackPtr glSwapBuffers)
throw(std::runtime_error) {
    TSpriteWorld theWorld;
    SWError err = SWCreateSpriteWorldFromGLContext(theWorld, glContext, &world, glMakeCurrent, glSwapBuffers);
    if(err) throw TError("TSpriteWorld::CreateSpriteWorldFromGLContext", err);
    return theWorld;
    }

/// @see SWCreateSpriteWorldFromFrames
inline TSpriteWorld TSpriteWorld::CreateSpriteWorldFromFrames(TFrame& screen, TFrame& back, TFrame& work)
throw(std::runtime_error) {
    TSpriteWorld theWorld;
    SWError err = SWCreateSpriteWorldFromFrames(theWorld, screen.ptr(), back.ptr(), work.ptr());
    if(err) throw TError("TSpriteWorld::CreateSpriteWorldFromFrames", err);
    return theWorld;
    }


/// @see SWDisposeSpriteWorld
inline void TSpriteWorld::dispose()
throw() {
    SWDisposeSpriteWorld(&mWorld);
    }

/// @see SWLockSpriteWorld
inline void TSpriteWorld::lock()
throw() {
    SWLockSpriteWorld(mWorld);
    }

/// @see SWUnlockSpriteWorld
inline void TSpriteWorld::unlock()
throw() {
    SWUnlockSpriteWorld(mWorld);
    }

/// @see SWUpdateSpriteWorld
inline void TSpriteWorld::update(bool inUpdateWindow)
throw() {
    SWUpdateSpriteWorld(mWorld, inUpdateWindow);
    }

/// @see SWProcessSpriteWorld
inline void TSpriteWorld::process()
throw() {
    SWProcessSpriteWorld(mWorld);
    }

/// @see SWAnimateSpriteWorld
inline void TSpriteWorld::animate()
throw() {
    SWAnimateSpriteWorld(mWorld);
    }

inline void TSpriteWorld::maxFps(short inFramesPerSec)
throw() {
    SWSetSpriteWorldMaxFPS(mWorld, inFramesPerSec);
    }

inline void TSpriteWorld::copyBackgroundToWorkArea()
throw() {
    SWCopyBackgroundToWorkArea(mWorld);
    }

inline TSprite TSpriteWorld::createSpriteFromFile(const char *file, long inMaxFrames)
throw(std::runtime_error)
{
    SpritePtr theSprite = NULL;
    SWError err = SWCreateSpriteFromFile(&theSprite, NULL, file, inMaxFrames);
    if(err) throw TError("TSpriteWorld::createSpriteFromFile", err);
    return TSprite(theSprite);
    }

inline DrawDataPtr TSpriteWorld::createDrawData()
throw()
{
    return SWCreateDrawData();
    }

inline void TSpriteWorld::collide(TSpriteLayer& inLayerSrc, TSpriteLayer& inLayerDst)
throw() {
    SWCollideSpriteLayer(mWorld, inLayerSrc.ptr(), inLayerDst.ptr());
    }

inline const short TSpriteWorld::pixelDepth() const
throw() {
    return SWGetSpriteWorldPixelDepth(mWorld);
    }

inline const unsigned long TSpriteWorld::milliSeconds() const
throw() {
    return SWGetMilliseconds(mWorld);
    }

inline TFrame TSpriteWorld::workFrame() const
throw() {
    return TFrame(mWorld->workFrameP);
    }

inline const SWRect& TSpriteWorld::backRect() const
throw() {
    return mWorld->backRect;
    }

inline const bool TSpriteWorld::frameHasOccurred() const
throw() {
    return mWorld->frameHasOccurred;
    }

inline void TSpriteWorld::add(TSpriteLayer& inLayer)
throw() {
    SWAddSpriteLayer(mWorld, inLayer.ptr());
    }

inline void TSpriteWorld::remove(TSpriteLayer& inLayer)
throw() {
    SWRemoveSpriteLayer(mWorld, inLayer.ptr());
    }

inline void TSpriteWorld::swap(TSpriteLayer& inSrcLayer, TSpriteLayer& inDsTSpriteLayer)
throw() {
    SWSwapSpriteLayer(mWorld, inSrcLayer.ptr(), inDsTSpriteLayer.ptr());
    }

inline TSpriteLayer TSpriteWorld::next(TSpriteLayer& inLayer)
throw() {
    return TSpriteLayer(SWGetNextSpriteLayer(mWorld, inLayer.ptr()));
    }

