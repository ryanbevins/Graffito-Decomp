#ifndef GC2D_BOUND_PANE_HPP
#define GC2D_BOUND_PANE_HPP

#include <JSystem/JUtility/JUTRect.hpp>
#include <JSystem/JUtility/JUTPoint.hpp>
#include <dolphin/types.h>

class J2DScreen;
class J2DPane;

struct TBoundPaneRect {
	TBoundPaneRect()
	{
		x1 = 0;
		y1 = 0;
		x2 = 0;
		y2 = 0;
	}

	int x1;
	int y1;
	int x2;
	int y2;
};

class TBoundPane {
public:
	TBoundPane(J2DScreen*, u32);
	void setPanePosition(s32, const JUTPoint&, const JUTPoint&,
	                     const JUTPoint&);
	void setPaneSize(s32, const JUTPoint&, const JUTPoint&, const JUTPoint&);
	bool update();

	// fabricated
	J2DPane* getPane() const { return unk0; }

public:
	/* 0x0 */ J2DPane* unk0;
	/* 0x4 */ JUTRect unk4;
	/* 0x14 */ TBoundPaneRect unk14;
	/* 0x24 */ bool unk24;
	/* 0x25 */ bool unk25;
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
	/* 0x34 */ f32 unk34;
	/* 0x38 */ JUTPoint unk38;
	/* 0x40 */ JUTPoint unk40;
	/* 0x48 */ JUTPoint unk48;
	/* 0x50 */ JUTPoint unk50;
	/* 0x58 */ JUTPoint unk58;
	/* 0x60 */ JUTPoint unk60;
};

#endif
