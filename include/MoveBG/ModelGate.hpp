#ifndef MOVE_BG_MODEL_GATE
#define MOVE_BG_MODEL_GATE

#include <Strategic/TakeActor.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SampleCtrlModel.hpp>
#include <dolphin/mtx.h>

class TModelGate : public TTakeActor {
public:
	TModelGate(const char* name) : TTakeActor(name) { }

	virtual MtxPtr getTakingMtx();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void loadAfter();

	void screenBlur(JDrama::TGraphics*);
	void startOpen();

public:
	/* 0x70 */ u8 unk70;          // flag byte: bit0=enable, bit1=opened, bit2=screenBlur enable, bit3=takeable
	/* 0x71 */ u8 unk71;          // selected destination stage index 0..4
	/* 0x72 */ u16 unk72;         // mtx index ("center" bone)
	/* 0x74 */ s16 unk74;         // particle rotate angle
	/* 0x76 */ s16 unk76;         // pad
	/* 0x78 */ MActor* unk78;     // gate model actor
	/* 0x7C */ Mtx unk7C;         // local matrix
	/* 0xAC */ JGeometry::TVec3<f32> unkAC; // Mario hold offset
	/* 0xB8 */ u8 unkB8;          // anim state
	/* 0xB9 */ u8 unkB9;          // frame mod index 0..7
	/* 0xBA */ u8 unkBA;          // last index
	/* 0xBB */ u8 unkBB;          // pad
	/* 0xBC */ u16 unkBC;         // wait counter
	/* 0xBE */ u16 unkBE;         // wait reset
	/* 0xC0 */ SampleCtrlModelData* unkC0;
	/* 0xC4 */ u8 unkC4;          // open state
	/* 0xC5 */ u8 unkC5;
	/* 0xC6 */ u8 unkC6;
	/* 0xC7 */ u8 unkC7;          // pad
	/* 0xC8 */ s16 unkC8;         // angle initial
	/* 0xCA */ s16 unkCA;         // angle current
	/* 0xCC */ s16 unkCC;         // shock counter
	/* 0xCE */ s16 unkCE;         // shock max
	/* 0xD0 */ f32 unkD0;         // current rotation
	/* 0xD4 */ f32 unkD4;         // rotation add
	/* 0xD8 */ f32 unkD8;         // rotation sub when normal
	/* 0xDC */ f32 unkDC;         // rotation sub when shaking
	/* 0xE0 */ u8 unkE0;
	/* 0xE1 */ u8 unkE1;          // pad
	/* 0xE2 */ u8 unkE2;          // pad
	/* 0xE3 */ u8 unkE3;          // pad
	/* 0xE4 */ f32 unkE4;         // blur progress
	/* 0xE8 */ f32 unkE8;         // blur lerp
	/* 0xEC */ f32 unkEC;         // blur target
	/* 0xF0 */ f32 unkF0;         // blur start dist
	/* 0xF4 */ f32 unkF4;         // blur end dist
	/* 0xF8 */ f32 unkF8;         // (unused)
	/* 0xFC */ f32 unkFC;         // bbox half x
	/* 0x100 */ f32 unk100;       // (unused?)
	/* 0x104 */ f32 unk104;       // bbox half x (~150)
	/* 0x108 */ f32 unk108;       // bbox min y (~-1000)
	/* 0x10C */ f32 unk10C;       // bbox max y (~150)
	/* 0x110 */ f32 unk110;       // bbox min z (~0)
	/* 0x114 */ f32 unk114;       // bbox max z (~300)
};

#endif
