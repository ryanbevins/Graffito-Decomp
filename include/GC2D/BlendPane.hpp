#ifndef GC2D_BLEND_PANE_HPP
#define GC2D_BLEND_PANE_HPP

#include <GC2D/BoundPane.hpp>
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>

class TBlendPane : public TBoundPane {
public:
	TBlendPane(J2DScreen*, u32);

public:
	/* 0x68 */ f32 mStep;
	/* 0x6C */ f32 mCurrent;
	/* 0x70 */ bool mActive;

public:
	virtual bool update();
	void setPaneBlend(s32 time, JUTTexture* tex0, JUTTexture* tex1)
	{
		if (tex1 == nullptr) {
			((J2DPicture*)unk0)->changeTexture(tex0->mTexInfo, 0);
		} else {
			((J2DPicture*)unk0)->changeTexture(tex0->mTexInfo, 0);
			const ResTIMG* texInfo = tex1->mTexInfo;
			((J2DPicture*)unk0)->changeTexture(texInfo, 1);
		}
		mActive  = true;
		mStep    = 1.0f / (f32)time;
		mCurrent = 0.0f;
	}
};

#endif
