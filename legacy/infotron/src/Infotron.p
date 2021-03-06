program Infotron;{began 12/18/93}	uses		objintf, sound, picker, gammaPasLibIntf, PSST, keyDowns, ColorOffScreen, QuickDrawProcs, myRGBColors, windows, MenuBars;{---Begin PeckEvents Declarations---}	const		WNE_TRAP_NUM = $60;		UNIMPLEMENTED_TRAP_NUM = $9F;		MaxWind = 15;		MaxMenu = 10;	var		dummy: boolean;		aRecta: rect;		windows: array[0..MaxWind] of WindowPtr;		menus: array[0..MaxMenu] of MenuHandle;		curNumWindows: integer;		curNumMenus: integer;		wantsToQuit: boolean;		gWNEImplemented: boolean;		hisApple: MenuHandle;		currentWindow: integer;		fstPtr: grafPtr;{-----------COPY THE FOLLOWING ROUTINES AT THE FRONT OF YOUR PROGRAM---------}	procedure peckInit (howMany: integer);	forward;	procedure peckQuit;	forward;	procedure peckNewWindow (var aWindow: windowPtr);	forward;	procedure peckKillWindow (var aWindow: windowPtr);	forward;	procedure peckNewMenu (var aMenu: menuHandle; disp: boolean);	forward;	procedure peckKillMenu (var aMenu: menuHandle);	forward;	procedure peckApple (aboutName: str255);	forward;	procedure peckMain;	forward;	procedure peckHalt;	forward;{-------------------This is where infotron begins----------}	procedure ________________;	forward;	const	{ipated}		kMainWindID = 1000;	{winds}		kFileMenuID = 128;		{menus}		kGameMenuID = 129;		iNew = 1;		iOpen = 2;		iRename = 3;		iDelete = 4;		iQuit = 6;		iLevel = 1;		iKeys = 2;		iStart = 4;		iPause = 5;		iFX = 7;		iMusic = 8;		iHelp = 10;		{I help lots of people.}		kSMB = 1;			{not really sure what this is for}		kBaseNum = 128;		kTopX = 96;		kTopY = 56;		skip = 0;		shiftMod = 512;		optionMod = 2048;		cmdMod = 256;		kMaxThings = 110;		kUp = 0;			{directions}		kRight = 2;		kDown = 4;		kLeft = 6;	type		boardArray = array[1..150, 1..150] of integer;		ThingArray = array[1..kMaxThings] of integer;		twoZeroArray = array[1..120, 1..120] of integer;		oneZeroArray = array[0..0] of integer;{User interface description}		bcp = ^bcpp;		bcpp = ^CompressedBoard;		CompressedBoard = record				name: str255;				sizeX, sizeY: integer;									{How far right and down does the level extend?}				murphy, term: integer;											{where does murphy start}				numScissors: integer;				numQuarks: integer;				numInfotrons: integer;				numBugs: integer;				numComp: integer;			{how many run-length compression thingys did we use up?}				theBoard: oneZeroArray;			end;		bap = ^bapp;		bapp = ^LevelRecord;		LevelRecord = record				name: str255;				sizeX, sizeY: integer;									{How far right and down does the level extend?}				murphy, term: integer;											{where does murphy start}				numScissors: integer;				numQuarks: integer;				numInfotrons: integer;				numBugs: integer;				scissors, quarks, bugs: ThingArray;				theBoard: twoZeroArray;			end;		MurphyType = record				x, y: integer;				lastMove: longint;				time: longint;				lastDir: integer;				numBombs: integer;				numInfos: integer;			end;	var {globals}		gNewDial, gRenDial, gKeysDial, gHelpDial: CDialog;		{dialogs}		gLevelDial, gOpenDial, gDeleteDial: CDlogWithScroll;		gOffScreen: GWorldPtr;		{something}		gBoard: bap;					{something else}		gMainWind: WindowPtr;		{window}		gFileMenu, gGameMenu: MenuHandle;	{menus}		gPlayerInMemory: boolean;		{info about game status}		gLevelInMemory: boolean;		gPlaying: boolean;		gPaused: boolean;		gMusic: boolean;		gFX: boolean;		gCanUseMusic: boolean;		gPlayer: PlayerType;			{the player info}		gMurphy: MurphyType;		gBoardX, gBoardY: integer;		{where they are}		gBoardRect: rect;				{???}		gXOffset, gYOffset: integer;		gUpdateBoard: boolean;		gLastTime: longint;		soldGW: GWorldPtr;		soldGD: GDHandle;	procedure SOG;			{set the world to the off screen world}	begin		GetGWorld(soldGW, soldGD);		SetGWorld(gOffScreen, nil);	end;	procedure SOW;			{set it back to what it was}	begin		SetGWorld(soldGW, soldGD);	end;	function PlotIconID (TheRect: Rect; Align: integer; Transform: integer; TheResID: INTEGER): OSErr;	inline		$303C, $0500, $ABC9;	procedure DrawOne (x, y: integer; what: integer);		var			r: rect;			err: OSErr;			oldGW: GWorldPtr;			oldGD: GDHandle;	begin		GetGWorld(oldGW, oldGD);		SetGWorld(gOffscreen, nil);		if not LockPixels(gOffScreen^.portpixmap) then			exittoshell;		SetRect(r, 0, 0, 32, 32);		OffsetRect(r, 96 + ((x - gBoardX) * 32), 56 + ((y - gBoardY) * 32));		err := PlotIconID(r, 0, 0, what);		UnlockPixels(gOffScreen^.portpixmap);		SetGWorld(oldGW, oldGD);	end;	procedure DrawOIconAt (x, y, xo, yo: integer; what: integer);		var			r: rect;			err: OSErr;			oldGW: GWorldPtr;			oldGD: GDHandle;	begin		GetGWorld(oldGW, oldGD);		SetGWorld(gOffscreen, nil);		if not LockPixels(gOffScreen^.portpixmap) then			exittoshell;		SetRect(r, 0, 0, 32, 32);		OffsetRect(r, 96 + ((x - gBoardX) * 32) + xo, 56 + ((y - gBoardY) * 32) + yo);		err := PlotIconID(r, 0, 0, what);		UnlockPixels(gOffScreen^.portpixmap);		SetGWorld(oldGW, oldGD);	end;	function PtToInt (x, y: integer): integer;	begin		PtToInt := integer((x * 256) + y);	end;	function IntToPt (i: integer): point;	begin		IntToPt.h := i div 256;		IntToPt.v := i mod 256;	end;	procedure CenterMurphy;	begin		gBoardX := gMurphy.x - 7;		gBoardY := gMurphy.y - 5;		if gBoardY < 1 then			gBoardY := 1		else if (gBoardY > (gBoard^^.sizeY - 10)) then			gBoardY := gBoard^^.sizeY - 10;		if gBoardX < 1 then			gBoardX := 1		else if (gBoardX > (gBoard^^.sizeX - 14)) then			gBoardX := gBoard^^.sizeX - 14;	end;	procedure initMurphy;		var			at: point;	begin		at := intToPt(gBoard^^.murphy);		gMurphy.x := at.h;		gMurphy.y := at.v;		gMurphy.numBombs := 0;		gMurphy.numInfos := 0;		gMurphy.lastMove := tickCount;		gMurphy.time := tickCount;		gMurphy.lastDir := kRight;	end;	procedure fixSizes;		var			c: integer;	begin		c := gBoard^^.sizeX;		while gBoard^^.theBoard[c, 2] = 128 do			c := c - 1;		gBoard^^.sizeX := c;		c := gBoard^^.sizeY;		while gBoard^^.theBoard[2, c] = 128 do			c := c - 1;		gBoard^^.sizeY := c;	end;	procedure DrawTwoBoxes;		var			at: point;	begin		at.h := 494;		at.v := 428;		drawPic(141, at);		at.h := 608;		at.v := 437;		drawPic(142, at);	end;	procedure UndrawTwoBoxes;		var			at: point;	begin		at.h := 494;		at.v := 428;		drawPic(139, at);		at.h := 608;		at.v := 437;		drawPic(140, at);	end;	procedure BlankPlayerBoxes;		var			r: rect;	begin		SetRect(r, 189, 431, 338, 447);		FillRect(r, black);		SetRect(r, 359, 431, 379, 447);		FillRect(r, black);		SetRect(r, 397, 431, 417, 447);		FillRect(r, black);		SetRect(r, 435, 431, 455, 447);		FillRect(r, black);	end;	procedure BlankLevelBoxes;		var			r: rect;	begin		SetRect(r, 189, 455, 455, 471);		FillRect(r, black);		SetRect(r, 488, 455, 518, 471);		FillRect(r, black);	end;	procedure blankMurphyBoxes;	begin		if gPlaying then			drawTwoBoxes;	end;	procedure BlankBoxes;	begin		blankMurphyBoxes;		BlankPlayerBoxes;		BlankLevelBoxes;	end;	procedure BoardDump;		var			tempRect: rect;	begin		tempRect := gBoardRect;		CopyWorldBits2(gOffScreen, gMainWind, POffsetRect(tempRect, 32 - gXOffset, 32 - gYOffset), gBoardRect);	end;	procedure UpdateBoard;		type			gArr = array[1..40, 1..40] of byte;		var			x, y: integer;			locX, locY: integer;			theRect: rect;			oldWind: windowPtr;			rect1, rect2: rect;			worked: boolean;			err: QDerr;			m: boolean;			oldGW: GWorldPtr;			oldGD: GDHandle;	begin		ForeColor(blackColor);		BackColor(whiteColor);		GetGWorld(oldGW, oldGD);		if not LockPixels(gOffScreen^.portpixmap) then			exittoshell;		SetGWorld(gOffScreen, nil);		penMode(patCopy);		setRect(theRect, 64, 24, 96, 56);		locX := 64;		for x := gBoardX - 1 to gBoardX + 17 do			begin				for y := gBoardY - 1 to gBoardY + 12 do					begin{$PUSH}{$R-}{    err := plotIconID(theRect, 0, 0, gBoard^^.theBoard[x, y]);}						drawIconAt(x, y, gBoard^^.theBoard[x, y]);{$POP}{    offsetRect(theRect, 0, 32);}					end;				Locx := locX + 32;				setRect(theRect, Locx, 24, Locx + 32, 56);			end;		SetGWorld(oldGw, oldGD);		UnlockPixels(gOffScreen^.portpixmap);		BoardDump;	end;	procedure drawLevelStats;	begin		if gLevelInMemory then			begin				RGBForeColor(myGreenColor);				centerString(gPlayer.level, 466, 192, 449);				moveTo(490, 466);				drawinteger(gPlayer.current, 3);				RGBForeColor(myBlackColor);			end;	end;	procedure drawTime (time: longint);		var			r: rect;	begin		setRect(r, 360, 432, 378, 442);		RGBForeColor(myBlackColor);		fillRect(r, black);		moveTo(360, 442);		RGBForeColor(myRedColor);		drawInteger((time) div 3600, 2);		offSetRect(r, 38, 0);		if gLastTime div 60 <> time div 60 then			begin				RGBForeColor(myBlackColor);				fillRect(r, black);				moveTo(398, 442);				RGBForeColor(myRedColor);				drawInteger((time) div 60, 2);			end;		offSetRect(r, 38, 0);		if gLastTime mod 60 <> time mod 60 then			begin				RGBForeColor(myBlackColor);				fillRect(r, black);				moveTo(436, 442);				RGBForeColor(myRedColor);				drawInteger((time) mod 60, 2);			end;		RGBForeColor(myBlackColor);	end;	procedure drawMurphyStats;	begin		drawTwoBoxes;		moveTo(499, 442);		if gMurphy.numInfos > 0 then			RGBForeColor(myYellowColor)		else			RGBForeColor(myGreenColor);		drawInteger(gBoard^^.numInfotrons - gMurphy.numInfos, 3);		moveTo(614, 451);		RGBForeColor(myRedColor);		drawInteger(gMurphy.numBombs, 2);		RGBForeColor(myBlackColor);	end;	procedure drawPlayerStats;	begin		if gPlayerInMemory then			begin				RGBForeColor(myRedColor);				centerString(gPlayer.name, 442, 192, 332);				if gPlaying then					DrawTime(tickCount - gMurphy.time)				else					drawTime(gplayer.time);				if gLevelInMemory then					drawLevelStats;				RGBForeColor(myBlackColor);			end;	end;	procedure UpdateMainWind;		var			at: point;			p: GrafPtr;	begin		GetPort(p);		SetPort(gMainWind);		at.h := 203;		at.v := 398;		drawPic(144, at);		at.h := 423;		drawPic(144, at);		at.h := 313;		drawPic(144, at);		at.h := 40;		at.v := 28;		drawPic(128, at);		at.h := 1;		at.v := 419;		drawPic(129, at);		drawPlayerStats;		if gPlaying then			DrawMurphyStats;		if gLevelInMemory then			BoardDump;		if gLevelInMemory and gPlayerInMemory then			DrawTwoBoxes		else			UndrawTwoBoxes;		SetPort(p);	end;	function NewBoardDataStructure: bap;		var			qd: bap;			x, y: integer;	begin		qd := bap(NewHandleClear(sizeOf(LevelRecord)));		MoveHHi(handle(qd));		HLock(handle(qd));		qd^^.sizeX := 120;		qd^^.sizeY := 120;		for x := 1 to qd^^.sizeX do			for y := 1 to qd^^.sizeY do{$PUSH}{$R-}				qd^^.theBoard[x, y] := kBaseNum;{$POP}		NewBoardDataStructure := qd;	end;	procedure DisposeBoardDataStructure (var b: bap);	begin		HUnlock(handle(b));		Disposehandle(handle(b));	end;	procedure DecompressLevel (c: bcp; var b: bap);		var			atx, aty, atbn: integer;			q: integer;			lastVal: integer;			cnt: integer;	begin		b := NewBoardDataStructure;		b^^.name := c^^.name;		b^^.sizeX := c^^.sizeX;		b^^.sizeY := c^^.sizeY;		b^^.murphy := c^^.murphy;		b^^.term := c^^.term;		b^^.numScissors := c^^.numScissors;		b^^.numQuarks := c^^.numQuarks;		b^^.numInfotrons := c^^.numInfotrons;		b^^.numBugs := c^^.numBugs;		atx := 1;		aty := 1;		atbn := 0;{$PUSH}{$R-}		for cnt := 1 to c^^.numScissors do			begin				b^^.scissors[cnt] := c^^.theBoard[atbn];				atbn := atbn + 1;			end;		for cnt := 1 to c^^.numQuarks do			begin				b^^.quarks[cnt] := c^^.theBoard[atbn];				atbn := atbn + 1;			end;		for cnt := 1 to c^^.numBugs do			begin				b^^.bugs[cnt] := c^^.theBoard[atbn];				atbn := atbn + 1;			end;{$R+}		lastVal := 128;		while (atbn < c^^.numComp) do			begin				atbn := atbn + 1;{$PUSH}{$R-}				if c^^.theBoard[atbn] < 0 then					for q := 1 to (-c^^.theBoard[atbn] - 1) do{$R+}						begin							b^^.theBoard[atx, aty] := lastVal;							atx := atx + 1;							if atx > c^^.sizeX then								begin									aty := aty + 1;									atx := 1;								end;						end				else					begin{$PUSH}{$R-}						b^^.theBoard[atx, aty] := c^^.theBoard[atbn];						lastVal := c^^.theBoard[atbn];{$R+}						atx := atx + 1;						if atx > c^^.sizeX then							begin								aty := aty + 1;								atx := 1;							end;					end;			end;	end;	procedure RidOfLevel;	begin		DisposeBoardDataStructure(gBoard);	end;	procedure LoadInLevel (name: Str255);		var			i: Handle;			com: bcp;			err: OSErr;	begin		if gLevelInMemory then			RidOfLevel;		i := GetNamedResource(levelResType, name);		HLock(i);		com := bcp(i);		DetachResource(i);		MoveHHi(handle(com));		HLock(handle(com));		DecompressLevel(com, gBoard);		HUnlock(handle(com));		DisposeHandle(handle(com));		gLevelInMemory := true;		BlankLevelBoxes;		fixSizes;		initMurphy;		gXOffset := 16;		gYOffset := 16;		gPlaying := false;		gLastTime := maxLongInt;		centerMurphy;		UpdateBoard;	end;	procedure SavePlayer;		var			data: handle;			tempData: handle;			c, x: integer;	begin		tempData := GetNamedResource(playerResType, gPlayer.name);		if (tempData <> nil) and (resError = noErr) then			begin				RmveResource(tempData);				ReleaseResource(tempData);			end;		data := NewHandleClear(sizeOf(PlayerType));		PlayerHandle(data)^^ := gPlayer;		AddResource(data, playerResType, uniqueID(playerResType), gPlayer.name);		WriteResource(data);		ChangedResource(data);		UpdateResFile(CurResFile);		x := Count1Resources(playerResType);		for c := 1 to x do			begin				tempData := GetIndResource(playerResType, c);		{Get each resource, and release it}				ReleaseResource(tempData);									{Thereby forcing an instantaneous update...}			end;		UpdateResFile(CurResFile);	end;	procedure OpenPlayer;		var			data: handle;			theName: Str255;	begin		if OpenChoice(theName, gOpenDial) then			begin				data := GetNamedResource(playerResType, theName);				if (ResError = noErr) and (data <> nil) then					begin						DetachResource(data);						gPlayer := PlayerHandle(data)^^;						gPlayerInMemory := true;						if gPlayer.level <> '' then							LoadInLevel(gPlayer.level);						DisposeHandle(data);					end;				BlankPlayerBoxes;				drawPlayerStats;			end;	end;	function ResName (typ: ResType; num: integer): Str255;		var			h: Handle;			d: integer;			name: Str255;	begin		name := '';		h := GetResource(typ, num);		if (h <> nil) and (ResError = noErr) then			begin				getResInfo(h, d, typ, name);				ReleaseResource(h);			end;		ResName := name;	end;	procedure NewPlayer;		var			dname: Str255;	begin		if GetName(dname, gNewDial) then			begin				gPlayerInMemory := true;				with gPlayer do					begin						skippedLev := [];						highLev := 1;						current := 1;						numSkipped := 0;						name := dname;						level := ResName(levelResType, 301);		{the first level}						time := 0;						keySet[1] := 'i';		{these are the standard keys. so there.}						keySet[2] := 'e';						keySet[3] := 'g';						keySet[4] := 'c';						keySet[5] := '_';					end;				SavePlayer;		{save me to disk}				LoadInLevel(gPlayer.level);				blankPlayerBoxes;				drawPlayerStats;			end;	end;	procedure RenamePlayer;		var			dname: Str255;			data: Handle;			oldName: str255;	begin		if gPlayerInMemory then			begin				oldName := gPlayer.name;				if GetName(dname, gRenDial) then					begin						gPlayer.name := dname;						data := GetNamedResource(playerResType, oldName);						if (ResError = noErr) and (data <> nil) then							begin								RmveResource(data);								ReleaseResource(data);							end;						SavePlayer;						blankPlayerBoxes;						drawPlayerStats;					end;			end		else			DoError('You idiot. There is no player to rename!');	end;	procedure DeletePlayer;		var			data: handle;			tempData: handle;			c, x: integer;			name: Str255;	begin		if DeleteChoice(name, gDeleteDial) then			begin				if name <> gPlayer.name then					begin						tempData := GetNamedResource(playerResType, name);						if (tempData <> nil) and (resError = noErr) then							begin								RmveResource(tempData);								ReleaseResource(tempData);							end;					end				else					DoError('This game does not condone suicide.  Please call Dr. Jack Kevorkian.');			end;	end;	procedure HandleFileMenu (i: integer);	begin		case i of			iQuit: 				PeckQuit;			iNew: 				NewPlayer;			iRename: 				RenamePlayer;			iOpen: 				OpenPlayer;			iDelete: 				DeletePlayer;			otherwise				DoError('Sorry, this has not been implemented yet.');		end;	end;	procedure DoKeys;	begin		if gPlayerInMemory then			begin				DoSetKey(gPlayer, gKeysDial);				SavePlayer;			end		else			DoError('There must be a player in memory in order to set the keys!');	end;	procedure DoLevel;	begin		if gPlayerInMemory then			begin				if LevelChoice(gPlayer, gLevelDial) then					begin						LoadInLevel(gPlayer.level);						SavePlayer;						drawLevelStats;					end;			end		else			DoError('There must be a player in memory in order to load a level!');	end;	procedure HandleGameMenu (i: integer);	begin		case i of			iHelp: 				DoHelp(gHelpDial);			iKeys: 				DoKeys;			iLevel: 				DoLevel;			iFX: 				gFX := not gFX;			iMusic: 				gMusic := not gMusic;			iStart: 				begin					gPlaying := true;					drawTwoBoxes;					GetDateTime(gMurphy.time);				end;			otherwise				DoError('No!');		end;	end;	procedure UpdateMenus;	begin{level/keys}		if gPlayerInMemory then			begin				EnableItem(gGameMenu, iLevel);				EnableItem(gGameMenu, iKeys);				EnableItem(gFileMenu, iRename);			end		else			begin				DisableItem(gGameMenu, iLevel);				DisableItem(gGameMenu, iKeys);				DisableItem(gFileMenu, iRename);			end;		if gLevelInMemory and gPlayerInMemory then			begin				EnableItem(gGameMenu, iStart);				EnableItem(gGameMenu, iPause);			end		else			begin				DisableItem(gGameMenu, iStart);				DisableItem(gGameMenu, iPause);			end;		if gCanUseMusic then			begin				EnableItem(gGameMenu, iMusic);				CheckItem(gGameMenu, iMusic, gMusic);			end		else			DisableItem(gGameMenu, iMusic);		CheckItem(gGameMenu, iFX, gFX);	end;	function showLogo (pic: integer): boolean;		var			stop: longint;	begin		DrawPicCenterGraf(pic);		FadeFromBlack(2);		stop := tickCount + 220;		while not button and (tickCount < stop) do			;		showLogo := button;		FadeToBlack(2);		FillRect(gMainWind^.portRect, black);	end;	procedure Init;		var			oe: OSErr;			at: point;			stop: longint;			continue: boolean;			tempRect: rect;	begin		PeckInit(15);		InitMenuBars;		oe := SetupGammaTools;		FadeToBlack(1);{    SoundsInit(TRUE);}		HideCursor;		gMainWind := GetNewCWindow(kMainWindID, nil, WindowPtr(-1));		PeckNewWindow(gMainWind);		ShowWindow(gMainWind);		SetPort(gMainWind);		HideMenuBar(gMainWind);		RGBForeColor(myBlackColor);		continue := ShowLogo(147);		gPlaying := false;		gFX := true;		gMusic := false;		gCanUseMusic := false;		gBoardRect.top := 56;		gBoardRect.bottom := 374;		gBoardRect.left := 96;		gBoardRect.right := 542;		tempRect := gBoardRect;		InsetRect(tempRect, -40, -40);		gOffScreen := MakeGWorld(tempRect);		SetPort(gMainWind);		if not continue then			continue := ShowLogo(148);		ShowMenuBar(gMainWind);		PeckApple('About Infotron�...');		gFileMenu := GetMenu(kFileMenuID);		PeckNewMenu(gFileMenu, false);		gGameMenu := GetMenu(kGameMenuID);		PeckNewMenu(gGameMenu, true);		UpdateMenus;		UpdateMainWind;		setUpDials(gNewDial, gRenDial, gKeysDial, gHelpDial, gLevelDial, gOpenDial, gDeleteDial);		FadeFromBlack(2);		InitCursor;	end;	procedure UnInit;		var			oe: OSErr;	begin		oe := DisposeGammaTools;		DestroyGWorld(gOffScreen);	end;	function nateMuffed (dir: integer): integer;	begin		case dir of			kUp: 				nateMuffed := kDown;			kDown: 				nateMuffed := kUp;			kLeft: 				nateMuffed := kRight;			kRight: 				nateMuffed := kLeft;		end;	end;	procedure TakeThing (what, x, y: integer);	begin		gBoard^^.theBoard[x, y] := 129;		if what = 135 then			gMurphy.numBombs := gMurphy.numBombs + 1		else			gMurphy.numInfos := gMurphy.numInfos + 1;		DrawMurphyStats;	end;	function MurphyCanGo (x, y: integer; dir: integer): boolean;		var			what: integer;	begin		what := gBoard^^.theBoard[x, y];		if what in [128, 129, 132] then			begin				MurphyCanGo := true;				gBoard^^.theBoard[x, y] := 129;			end		else if (what = 900 + nateMuffed(dir)) or (what = 950 + dir mod 4) then			case dir of				kUp: 					MurphyCanGo := gBoard^^.theBoard[x, y - 1] = 129;				kDown: 					MurphyCanGo := gBoard^^.theBoard[x, y + 1] = 129;				kLeft: 					MurphyCanGo := gBoard^^.theBoard[x - 1, y] = 129;				kRight: 					MurphyCanGo := gBoard^^.theBoard[x + 1, y] = 129;			end		else if (what = 850) or (what = 135) then			begin				TakeThing(what, x, y);				MurphyCanGo := true;			end		else			MurphyCanGo := false;	end;	function CanScroll (dir: integer): boolean;	begin		if ((dir = kUp) and (gMurphy.y > 5) and (gMurphy.y < gBoard^^.sizeY - 5)) then			CanScroll := true		else if ((dir = kRight) and (gMurphy.x < GBoard^^.sizeX - 6) and (gMurphy.x > 8)) then			CanScroll := true		else if ((dir = kDown) and (gMurphy.y > 6) and (gMurphy.y < gBoard^^.sizeY - 4)) then			CanScroll := true		else if ((dir = kLeft) and (gMurphy.x > 7) and (gMurphy.x < gBoard^^.sizeX - 7)) then			CanScroll := true		else			canScroll := false;	end;	procedure MoveUp;		var			count: integer;			e: integer;	begin		if MurphyCanGo(gMurphy.x, gMurphy.y - 1, kUp) then			begin				DrawIconAt(gMurphy.x, gMurphy.y, gBoard^^.theBoard[gMurphy.x, gMurphy.y]);				gMurphy.y := gMurphy.y - 1;				if CanScroll(kUp) then					begin						gBoardY := gBoardY - 1;						CopyBits(GrafPtr(gOffScreen)^.portBits, GrafPtr(gOffScreen)^.portBits, gOffScreen^.portRect, POffsetRect(gOffScreen^.portRect, 0, 32), srcCopy, nil);						for count := gBoardX - 1 to gBoardX + 15 do{$PUSH}{$R-}							DrawIconAt(count, gBoardY, gBoard^^.theBoard[count, gBoardY]);{$POP}					end;				for count := -8 to 8 do					begin						gYOffset := 2 * count;						if count <> -8 then							DrawOIconAt(gMurphy.x, gMurphy.y, 0, -count * 2 + 18, gBoard^^.theBoard[gMurphy.x, gMurphy.y]);						if odd(abs(count)) then							DrawOIconAt(gMurphy.x, gMurphy.y, 0, -count * 2 + 16, 202)						else							DrawOIconAt(gMurphy.x, gMurphy.y, 0, -count * 2 + 16, 204);						BoardDump;					end;			end;	end;	procedure MoveRight;		var			count: integer;	begin		if MurphyCanGo(gMurphy.x + 1, gMurphy.y, kRight) then			begin				DrawIconAt(gMurphy.x, gMurphy.y, gBoard^^.theBoard[gMurphy.x, gMurphy.y]);				gMurphy.x := gMurphy.x + 1;				if CanScroll(kRight) then					begin						gBoardX := gBoardX + 1;						CopyBits(GrafPtr(gOffScreen)^.portBits, GrafPtr(gOffScreen)^.portBits, gOffScreen^.portRect, POffsetRect(gOffScreen^.portRect, -32, 0), srcCopy, nil);						for count := gBoardY - 1 to gBoardY + 11 do{$PUSH}{$R-}							DrawIconAt(gBoardX + 14, count, gBoard^^.theBoard[gBoardX + 14, count]);{$POP}					end;				DrawIconAt(gMurphy.x, gMurphy.y, 198);				BoardDump;			end;	end;	procedure MoveDown;		var			count: integer;			r: rect;	begin		if MurphyCanGo(gMurphy.x, gMurphy.y + 1, kDown) then			begin				DrawIconAt(gMurphy.x, gMurphy.y, gBoard^^.theBoard[gMurphy.x, gMurphy.y]);				gMurphy.y := gMurphy.y + 1;				if CanScroll(kDown) then					begin						gBoardY := gBoardY + 1;						r := gOffScreen^.portRect;						r.bottom := r.bottom + 2;						CopyBits(GrafPtr(gOffScreen)^.portBits, GrafPtr(gOffScreen)^.portBits, r, POffsetRect(r, 0, -32), srcCopy, nil);						for count := gBoardX - 1 to gBoardX + 15 do{$PUSH}{$R-}							DrawIconAt(count, gBoardY + 10, gBoard^^.theBoard[count, gBoardY + 10]);{$POP}					end;				DrawIconAt(gMurphy.x, gMurphy.y, 198);				BoardDump;			end;	end;	procedure MoveLeft;		var			count: integer;	begin		if MurphyCanGo(gMurphy.x - 1, gMurphy.y, kLeft) then			begin				DrawIconAt(gMurphy.x, gMurphy.y, gBoard^^.theBoard[gMurphy.x, gMurphy.y]);				gMurphy.x := gMurphy.x - 1;				if CanScroll(kLeft) then					begin						gBoardX := gBoardX - 1;						CopyBits(GrafPtr(gOffScreen)^.portBits, GrafPtr(gOffScreen)^.portBits, gOffScreen^.portRect, POffsetRect(gOffScreen^.portRect, 32, 0), srcCopy, nil);						for count := gBoardY - 1 to gBoardY + 11 do{$PUSH}{$R-}							DrawIconAt(gBoardX, count, gBoard^^.theBoard[gBoardX, count]);{$POP}					end;				DrawIconAt(gMurphy.x, gMurphy.y, 198);				BoardDump;			end;	end;	procedure ActionKey;	begin	end;	procedure HandleKey (k: char);	begin		if k = gPlayer.keySet[1] then			MoveUp		else if k = gPlayer.keySet[3] then			MoveRight		else if k = gPlayer.keySet[4] then			MoveDown		else if k = gPlayer.keySet[2] then			MoveLeft		else if k = gPlayer.keySet[5] then			ActionKey;		gMurphy.lastMove := tickCount;	end;	procedure ________________;	begin	end;{-------------------MAIN EVENT DOING CODE GOES HERE-------------------------}	procedure doMenu (theMenu: menuHandle; theItem: integer);	begin		if theMenu = gFileMenu then			HandleFileMenu(theItem)		else			HandleGameMenu(theItem);		UpdateMenus;	end;	procedure doMouseDown (theWindow: windowPtr; where: point; when: longint; mods: integer);	begin	end;	procedure doIdle (theWindow: windowPtr);	begin	end;	procedure DoEvery;		var			km: KeyMap;			theKey: char;			dorky: boolean;			s: longint;	begin		GetKeys(km);		theKey := ChangeKey(km, dorky);		if gPlaying then			begin				HandleKey(theKey);				GetDateTime(s);				if gLastTime <> s then					begin						DrawTime(s - gMurphy.time);						gLastTime := s					end;			end		else if theKey in [gPlayer.keySet[1], gPlayer.keySet[2], gPlayer.keySet[3], gPlayer.keySet[4], gPlayer.keySet[5]] then			begin				gPlaying := true;				drawTwoBoxes;				GetDateTime(gMurphy.time);			end;	end;	procedure doKeyDown (theWindow: windowPtr; theKey: char; mods: integer);	begin	end;	procedure doCloseWindow (theWindow: windowPtr);	begin	end;	procedure doAbout;	begin		SysBeep(10);	end;	procedure doUpdate (theWindow: windowPtr; resized: boolean);	begin		if theWindow = gMainWind then			UpdateMainWind;	end;	procedure doKillMenu (theMenu: menuHandle);	begin	end;	procedure doActivate (theWindow: windowPtr);	begin	end;{------------------------- Peck Event Main Routines -------------------------------}	procedure peckInit (howMany: integer);		var			counter: integer;	begin		for counter := 1 to howMany do						{Get enough pointer/heap memory}			moreMasters;		getPort(fstPtr);		curNumWindows := 0;									{Initalize These Variables}		curNumMenus := 0;		wantsToQuit := false;		currentWindow := 0;		for counter := 0 to MaxWind do								{Clear window array}			windows[counter] := nil;		for counter := 0 to MaxMenu do								{Clear menu array}			menus[counter] := nil;		initCursor;												{Arrow Cursor}	end;	procedure peckQuit;	begin		wantsToQuit := true;				{Will halt execution next time main peckEvents gets called}	end;	procedure peckNewWindow (var aWindow: windowPtr);	begin		curNumWindows := curNumWindows + 1;		if curNumWindows > MaxWind then			curNumWindows := MaxWind		else			begin				windows[curNumWindows] := aWindow;				setPort(aWindow);				currentWindow := curNumWindows;			end;	end;	function findAWindow (wFind: windowPtr): integer;		var			winSearchCount: integer;	begin		winSearchCount := 1;		while (windows[winSearchCount] <> wFind) and (winSearchCount < curNumWindows) do			winSearchCount := winSearchCount + 1;		if winSearchCount <= curNumWindows then			findAWindow := winSearchCount		else			findAWindow := 0;	end;	procedure peckKillWindow (var aWindow: windowPtr);		var			recordNum: integer;			wKcounter: integer;	begin		recordNum := findAWindow(aWindow);				{Search array for the window}		doCloseWindow(windows[recordNum]);		if recordNum > 0 then			begin				curNumWindows := curNumWindows - 1;				if curNumWindows > 0 then					begin						for wKcounter := recordNum to curNumWindows do			{Fix List}							windows[wKcounter] := windows[wKcounter + 1];						windows[curNumWindows + 1] := nil;						setPort(windows[1]);					end				else					setPort(fstPtr);													{Whatever it was before}			end;	end;	procedure peckNewMenu (var aMenu: menuHandle; disp: boolean);	begin		curNumMenus := curNumMenus + 1;		if curNumMenus > MaxMenu then			curNumMenus := MaxMenu		else			begin				menus[curNumMenus] := aMenu;				insertMenu(menus[curNumMenus], 0);			end;		if disp then			drawMenuBar;	end;	function findMenu (mFind: menuHandle): integer;		var			menSearchCount: integer;	begin		menSearchCount := 1;		while (menus[menSearchCount] <> mFind) and (menSearchCount <= curNumMenus) do			menSearchCount := menSearchCount + 1;		if menSearchCount <= curNumMenus then			findMenu := menSearchCount		else			findMenu := 0;	end;	function findMenuByID (mFind: integer): integer;		var			msc: integer;	begin		msc := 1;		while (menus[msc]^^.menuID <> mFind) and (msc <= curNumMenus) do			msc := msc + 1;		if msc <= curNumMenus then			findMenuByID := msc		else			findMenuByID := 0;	end;	procedure peckKillMenu (var aMenu: menuHandle);		var			recordNum: integer;			wKcounter: integer;	begin		recordNum := findMenu(aMenu);					{Search array for the menu}		if recordNum > 0 then			begin				doKillMenu(menus[recordNum]);				curNumMenus := curNumMenus - 1;				if curNumMenus > 0 then					begin						for wKcounter := recordNum to curNumMenus do			{Fix List}							menus[wKcounter] := menus[wKcounter + 1];						menus[curNumMenus + 1] := nil;					end;			end;	end;	procedure appleSelect (theItem: integer);	begin		doAbout;	end;	procedure peckApple (aboutName: str255);		var			appleTitle: Str255;			appleID: integer;			dummy: boolean;	begin		appleTitle := ' ';		appleTitle[1] := char($14);		appleID := 1;		hisApple := NewMenu(appleID, appleTitle);		appendMenu(hisApple, aboutName);		appendMenu(hisApple, '(-');		addResMenu(hisApple, 'DRVR');		peckNewMenu(hisApple, false);	end;	procedure doEvent (theEvent: eventRecord);		var			evnWhat: integer;			wCIn: windowPtr;			winNum: integer;			resultCode: integer;			aBrect: rect;			theSize: longint;			menuID, item: integer;			menNum: integer;			wCntr: integer;			evnChar: char;			evnMods: integer;			itemName: str255;			oldPort: grafPtr;			aDummy: integer;			isActive: boolean;	begin		evnWhat := theEvent.what;		if evnWhat = nullEvent then			if currentWindow > 0 then				for wCntr := 1 to curNumWindows do					doIdle(windows[wCntr]);		if evnWhat = mouseDown then			begin				resultCode := findWindow(theEvent.where, wCIn);				if resultCode = inContent then					begin						winNum := findAWindow(wCIn);						if currentWindow <> winNum then							begin											{Make sure it was current wind}								setPort(wCIn);	{Set it to the wind it was clicked}								currentWindow := winNum;					{In!}								selectWindow(wCIn);							end						else							begin								globalToLocal(theEvent.where);								if winNum > 0 then									doMouseDown(windows[winNum], theEvent.where, theEvent.when, theEvent.modifiers);							end;					end;				if resultCode = inDrag then					begin						aBrect := screenBits.bounds;						dragWindow(wCIn, theEvent.where, aBrect);						setPort(wCIn);						selectWindow(wCIn);					end;				if resultCode = inSysWindow then					systemClick(theEvent, wCIn);				if resultCode = inGoAway then					if trackGoAway(wCIn, theEvent.where) then						peckKillWindow(wCIn);				if resultCode = inGrow then					begin						aBrect := screenBits.bounds;						setPort(wCIn);						theSize := growWindow(wCIn, theEvent.where, aBrect);						sizeWindow(wCIn, loWord(theSize), hiWord(theSize), true);						eraseRect(wCIn^.portRect);						drawGrowIcon(wCIn);						winNum := findAWindow(wCIn);						doUpdate(windows[winNum], true)					end;				if (resultCode = inZoomIn) or (resultCode = inZoomOut) then					begin						winNum := findAWindow(wCIn);						setPort(wCIn);						zoomWindow(wCIn, resultCode, true);						eraseRect(wCIn^.portRect);						doUpdate(windows[winNum], true)					end;				if resultCode = inMenuBar then					begin						theSize := menuSelect(theEvent.where);						hiliteMenu(0);						menuID := hiWord(theSize);						if menuID <> 0 then							begin								item := loWord(theSize);								menNum := findMenuByID(menuID);								if (menNum <> 1) then									doMenu(menus[menNum], item)								else if item = 1 then									doAbout								else									begin										getItem(menus[menNum], item, itemName);										aDummy := openDeskAcc(itemName);										setPort(windows[currentWindow]);									end;							end;					end;			end;		if evnWhat = updateEvt then			begin				getPort(oldPort);				setPort(windowPtr(theEvent.message));				beginUpdate(windowPtr(theEvent.message));				winNum := findAWindow(windowPtr(theEvent.message));				if winNum > 0 then					doUpdate(windowPtr(theEvent.message), false);				endUpdate(windowPtr(theEvent.message));				setPort(oldPort);			end;		if evnWhat = activateEvt then			begin				isActive := (BitAnd(theEvent.modifiers, activeFlag) <> 0);				if isActive then					begin						wCIn := windowPtr(theEvent.message);						winNum := findAWindow(wCIn);						setPort(wCIn);						selectWindow(wCIn);						doActivate(wCIn);					end;				currentWindow := winNum;			end;		if evnWhat = keyDown then			begin				evnChar := char(BitAnd(theEvent.message, charCodeMask));				evnMods := theEvent.modifiers;				if BitAnd(evnMods, cmdKey) > 0 then					begin						theSize := menuKey(evnChar);						hiliteMenu(0);						menuID := hiWord(theSize);						if menuID = 0 then							doKeyDown(windows[currentWindow], evnChar, evnMods)						else							begin								item := loWord(theSize);								menNum := findMenuByID(menuID);								doMenu(menus[menNum], item);							end;					end				else					doKeyDown(windows[currentWindow], evnChar, evnMods);			end;		DoEvery;	end;	procedure peckMain;		var			isMine: boolean;			allMask: integer;			anEvent: eventRecord;			evnCode: integer;	begin		gWNEImplemented := (NGetTrapAddress(WNE_TRAP_NUM, ToolTrap) <> NGetTrapAddress(UNIMPLEMENTED_TRAP_NUM, ToolTrap));		while not wantsToQuit do										{Make sure user doesn't wanna leave}			begin				if gWNEImplemented then					isMine := waitNextEvent(everyEvent, anEvent, 10, nil)				else					begin						systemTask;						isMine := getNextEvent(everyEvent, anEvent);					end;				if (isMine) or (anEvent.what = nullEvent) then				{Null event returns false}					doEvent(anEvent);			end;	end;	procedure peckHalt;		var			winCountr, menCountr: integer;	begin		for winCountr := 1 to curNumWindows do			disposeWindow(windows[winCountr]);		for menCountr := 1 to curNumMenus do			disposeMenu(menus[menCountr]);	end;{---------- End of Peck Event Routines -----------}begin	Init;	PeckMain;	UnInit;	PeckHalt;end.