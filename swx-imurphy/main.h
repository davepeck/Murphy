///--------------------------------------------------------------------------------------
// Scrolling Demo.h
//
// By: Vern Jensen, Updated 7/5/2003 for SWX
///--------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif
	
	
	int main(int argc, char *argv[]);
	void	CreateSpriteWorld( void );
	void	SetUpTiling( void )	;
	void	CreateBallSprite( void );
	void	CreateDiamondMeterSprite( void );
	void	CreateStatsNumberSprite( void );
	void	SetUpAnimation( void );
	
	void	RunAnimation( void );
	void	ShutDown( void );
	
	void TileChangeProc(SpriteWorldPtr spriteWorldP);
	void KeySpriteMoveProc(SpritePtr srcSpriteP);
	void DiamondMeterSpriteMoveProc(SpritePtr srcSpriteP);
	void StatsSpriteMoveProc(SpritePtr srcSpriteP);
	void UpdateDiamondMeter( void );
	
	void SmoothScrollingWorldMoveProc(
									  SpriteWorldPtr spriteWorldP,
									  SpritePtr followSpriteP);
	
	void	UpdateKeys( void );
	
	
#ifdef __cplusplus
}
#endif
