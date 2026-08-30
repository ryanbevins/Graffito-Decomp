#include <GC2D/Talk2D2.hpp>
#include <Camera/Camera.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/MessageLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DOrthoGraph.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryOutputStream.hpp>
#include <JSystem/JUtility/JUTResFont.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <MSound/MSoundSE.hpp>
#include <MoveBG/MapObjHide.hpp>
#include <NPC/NpcBase.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

class MSound {
public:
	bool gateCheck(u32);
	void talkModeIn(bool);
	void talkModeOut();
};

extern MSound* gpMSound;

class TFlagManager {
public:
	s32 getFlag(u32) const;

	static TFlagManager* smInstance;
};

class RumbleMgr {
public:
	void finishPause();
	void startPause();
};

extern RumbleMgr* SMSRumbleMgr;

TTalk2D2* gpTalk2D;

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";
static const char* MtxCalcTypeName_Basic
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char* MtxCalcTypeName_Softimage
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char* MtxCalcTypeName_MotionBlend
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char* MtxCalcTypeName_User
    = "MActorMtxCalcType_User ユーザー定義";

GXColor TTalk2D2::cColorTable[6] = {
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0xff, 0xb4, 0x8c, 0xff },
	{ 0x6e, 0xe6, 0xff, 0xff },
	{ 0xff, 0xff, 0x00, 0xff },
	{ 0xaa, 0xff, 0x50, 0xff },
};

static const u32 scTalkSoundList[] = {
	0x00008850, 0x00008851, 0x00008852, 0x00008853, 0x00008854,
	0x00008855, 0x00008856, 0x00008857, 0x00008858, 0x00008859,
	0x0000885A, 0x0000885B, 0x0000885C, 0x0000885D, 0x0000885E,
	0x0000885F, 0x00008860, 0x00008861, 0x00008862, 0x00008863,
	0x00008866, 0x00008867, 0x00008868, 0x00008869, 0x0000886A,
	0x0000886B, 0x0000886C, 0x0000886D, 0x0000886E, 0x0000886F,
	0x00008870, 0x00008871, 0x00008872, 0x00008873, 0x00008874,
	0x00008875, 0x00008876, 0x00008877, 0x00008878, 0x00008879,
	0x0000887A, 0x0000887B, 0x0000887C, 0x0000887D, 0x0000887E,
	0x0000887F, 0x00008880, 0x00008881, 0x00008882, 0x00008883,
	0x00008884, 0x00008885, 0x00008886, 0x00008887, 0x00008888,
	0x00008889, 0x0000888A, 0x0000888B, 0x0000888C, 0x0000888D,
	0x0000888E, 0x0000888F, 0x00008890, 0x00008891, 0x00008892,
	0x00008893, 0x00008894, 0x00008895, 0x00008896, 0xFFFFFFFF,
	0x00008899, 0x0000889A, 0x0000889B, 0x0000889C, 0x0000889D,
	0x0000889E, 0x0000889F, 0x000088A0, 0x000088A1, 0x000088A2,
	0x000088A3, 0x000088C0, 0x000088C1, 0x000088C2, 0x000088C3,
	0x000088C4, 0x000088C5, 0x000088C6, 0x000088C7, 0x000088C8,
	0x000088C9, 0x000088CA, 0x000088CB, 0x000088CC, 0x000088CD,
	0x000088CE, 0x000088CF, 0x000088E5, 0x000088E6, 0x000088E7,
	0x000088E8, 0x000088E9, 0x000088EA, 0x000088EB, 0x000088D0,
	0x000088EC, 0x000088ED, 0x000088D1, 0x000088EE, 0x000088D2,
	0x000088EF, 0x000088D3, 0x000088D4, 0x000088D5, 0x000088D6,
	0x000088D7, 0x000088D8, 0x000088D9, 0x000088DA, 0x000088DB,
	0x000088DC, 0x000088DD, 0x000088DE, 0x000088DF, 0x000088E0,
	0x000088E1, 0x000088E2, 0x000088E3, 0x000088E4, 0x00004849,
	0x80010025, 0x0000483D, 0x000088A6, 0x00008864, 0x00008865,
};

TTalk2D2::TTalk2D2(const char* name)
    : JDrama::TViewObj(name)
{
	unk28  = 0;
	unk2C  = 0;
	unk90  = 0;
	unk94  = 0.0f;
	unk214 = -1;
	unk220 = 0x1a;
	unk222 = 0;
	unk248 = 0;
	unk24C = 0;
	unk250 = 1;
	unk251 = 0;
	unk252 = 0;
	unk254 = 0;
	unk264 = 3;
	unk26A = 0;
	unk26B = 0;
	unk26C = 0;
	unk26D = 0;
	unk270 = 1;
	unk274 = 0;
	unk278 = 0;
	*(u32*)&unk27C = 0xffffffff;
	unk280 = 0;
	unk2DC = 0;
	unk330 = 0;
	unk332 = 0;
	unk334 = 0;
	unk338 = 0.04f;
	unk33C = 1.0f;
	unk340 = 100;
	gpTalk2D = this;

	for (int i = 0; i < 90; ++i)
		unk9C[i] = 0;

	unk224 = 0;
	unk6C[0] = 0;
	unk78[0] = 0;
	unk228[0] = 0;
	unk225 = 0;
	unk6C[1] = 0;
	unk78[1] = 0;
	unk228[1] = 0;
	unk226 = 0;
	unk6C[2] = 0;
	unk78[2] = 0;
	unk228[2] = 0;
}

void TTalk2D2::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	JKRArchive* archive = (JKRArchive*)JKRFileLoader::getVolume("game_6");

	unk2C = new J2DSetScreen("message_2.blo", archive);

	for (int i = 0; i < 3; ++i) {
		unk30[i] = unk2C->search('me_1' + i);
		unk3C[i] = unk2C->search('bac1' + i);
		unk48[i] = unk2C->search('\0f_1' + i * 3);
		unk54[i] = unk2C->search('\0f_2' + i * 3);
		unk60[i] = unk2C->search('\0f_3' + i * 3);
		unk6C[i] = unk2C->search('cu_1' + i);
		unk78[i] = unk2C->search('cc_1' + i);
		unk84[i] = unk2C->search('cs_1' + i);
	}

	unk244 = new JUTTexture(
	    (const ResTIMG*)JKRFileLoader::getGlbResource(
	        "/game_6/timg/message_back_1.bti"));
	unk90 = unk2C->search('me_0');

	unk258 = new TMessageLoader;
	unk258->loadMessageData("/scene/map/message.bmg");

	unk25C = new TMessageLoader;
	unk25C->loadMessageData("/common/2d/sys_message.bmg");

	unk204 = unk2C->search('me_4');
	unk208 = (J2DTextBox*)unk2C->search('slct');
	unk208->setFont((JUTFont*)gpSystemFont);

	for (int i = 0; i < 2; ++i) {
		unk20C[i] = (J2DTextBox*)unk2C->search('sc_1' + i);
		unk218[i] = new char[0x11];
	}

	unk10 = new J2DSetScreen("message_board_1.blo", archive);
	unk14 = new TBoundPane(unk10, 'mb_0');
	unk18 = (J2DTextBox*)unk10->search('text');
	SMSMakeTextBuffer(unk18, 0x200);
	unk18->setFont((JUTFont*)gpSystemFont);
	unk1C = unk10->search('cu_1');
	unk20 = unk10->search('cc_1');
	unk24 = unk10->search('cs_1');
}

void TTalk2D2::loadAfter()
{
	JDrama::TViewObj::loadAfter();

	J2DPane* start   = unk48[1];
	J2DPane* control = unk54[1];
	J2DPane* end     = unk60[1];

	f32 startX = start->mBounds.x1;
	f32 startY = start->mBounds.y1;
	f32 ctrlX  = 0.0f;
	f32 ctrlY  = control->mBounds.y1 - unk220;
	f32 endX   = end->mBounds.x2 - 10;
	f32 endY   = end->mBounds.y1;
	f32 prevX  = startX;
	f32 prevY  = startY;
	f32 length = 0.0f;

	for (f32 t = 0.01f; t <= 1.0f; t += 0.01f) {
		f32 inv = 1.0f - t;
		f32 x   = inv * inv * startX + 2.0f * t * inv * ctrlX
		    + t * t * endX;
		f32 y = inv * inv * startY + 2.0f * t * inv * ctrlY
		    + t * t * endY;
		f32 dx = x - prevX;
		f32 dy = y - prevY;

		length += JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy);
		prevX = x;
		prevY = y;
	}

	unk94 = 1.0f / length;

	for (int i = 0; i < 3; ++i) {
		unk48[i]->mVisible = false;
		unk54[i]->mVisible = false;
		unk60[i]->mVisible = false;
	}

	for (int i = 0; i < 90; ++i) {
		JUTRect bounds(0, 0, 20, 20);
		unk9C[i] = new J2DTextBox(0, bounds, gpSystemFont->getResFont(),
		                          "あ", HBIND_LEFT, VBIND_CENTER);
		unk9C[i]->mBlack   = 0xffffff00;
		unk9C[i]->mWhite   = 0xffffffff;
		unk9C[i]->mVisible = false;
	}

	unk234             = 1.0f;
	unk6C[0]->mVisible = false;
	unk238             = 2.0f;
	unk6C[1]->mVisible = false;
	unk23C             = 3.0f;
	unk6C[2]->mVisible = false;

	unk244->mWrapT = 1;
	unk244->mWrapS = 0;

	unk90->move(0x189, 0x73);
	unk90->mRotation = -18.0f;
	unk330           = unk90->mBounds.x1;
	unk332           = unk90->mBounds.y1;
	unk334           = unk90->mRotation;
	unk204->mVisible = false;

	char* text = new char[0x5e];
	for (int i = 0; i < 0x5d; ++i)
		text[i] = ' ';
	text[0x5d] = '\0';
	unk208->setString(text);

	void* data = unk25C->unk4;
	JMSMesgEntry* entry
	    = (JMSMesgEntry*)unk25C->getMessageEntry(3);
	setupTextBox(data, entry);
	unk260 = unk25C;

	const char* nameRefs[10] = { "空港沈みモンテ", "モンテ1", "", "", "", "",
		                         "",             "",        "", "" };
	u32 messageIDs[10]       = { 0x26, 0x23, 0, 0, 0, 0, 0, 0, 0, 0 };

	for (int i = 0; i < 10; ++i) {
		unk2E0[i].unk0
		    = JDrama::TNameRefGen::getInstance()->getRootNameRef()->search(
		        nameRefs[i]);
		unk2E0[i].unk4 = messageIDs[i];
	}
}

void TTalk2D2::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		if (gpMarDirector->unk124 == 2) {
			switch (unk248) {
			case 2: {
				CPolarSubCamera* camera = gpCamera;
				bool ready              = false;
				if (camera->isTalkCameraSpecifyMode(camera->mMode)
				    && !camera->isNowInbetween())
					ready = true;

				if (ready ? true : false) {
					unk251 = 0x14;
					unk248 = 3;
				}
				break;
			}
			case 4:
				if (unk28) {
					if (openBoardWindow())
						unk248 = 5;
				} else {
					if (openNormalWindow())
						unk248 = 5;
				}
				break;
			case 5:
				if (unk28) {
					moveBoardWindow();
					checkBoardControler();
				} else {
					moveTalkWindow();
					checkControler();
				}
				break;
			case 6: {
				bool closed = false;
				if (unk28) {
					if (unk14->update())
						closed = true;
				} else {
					closed = closeNormalWindow();
				}

				if (closed) {
					if (unk270 & 1)
						unk248 = 0;
					else
						unk248 = 1;
				}
				break;
			}
			case 7:
				if (unk28) {
					if (eraseBoardWindow())
						unk248 = 8;
				} else {
					if (eraseNormalWindow())
						unk248 = 4;
				}
				break;
			case 8: {
				bool complete = false;
				int alpha     = unk18->mAlpha + 4;
				if ((u16)alpha > 0xff) {
					alpha    = 0xff;
					complete = true;
				}

				unk18->mAlpha = alpha;
				if (complete)
					unk248 = 5;
				break;
			}
			}
		}
	}

	if (flags & 2) {
		if (gpMarDirector->unk124 == 2) {
			u32 mode = unk248;
			if (mode == 3) {
				--unk251;
				if ((s8)unk251 < 0) {
					unk2DE = 0;
					unk248 = 4;
				}
			} else if (mode == 7) {
				int alpha = unk90->mAlpha - 0x10;
				if ((s16)alpha < 0) {
					unk234 = 1.0f;
					unk238 = 2.0f;
					unk23C = 3.0f;

					for (int i = 0; i < 3; ++i) {
						unk3C[i]->mVisible = false;
						unk6C[i]->mVisible = false;
						(&unk224)[i]       = 0;
					}

					for (int i = 0; i < 90; ++i)
						if (unk9C[i])
							unk9C[i]->mVisible = false;

					TMessageLoader* loader = unk260;
					void* data             = loader->unk4;
					JMSMesgEntry* entry
					    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
					setupTextBox(data, entry);
					unk26C = 0;

					if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
						unk340 = 0x20;
					else
						unk340 = 0x40;

					*(u32*)&unk27C = 0xffffffff;
					*(u32*)&unk27C = 0xffffffff;
					unk2DE         = 0;
					unk2DC         = 0;
					unk248         = 4;
					alpha          = 0xff;
				}

				unk90->mAlpha = alpha;
			}
		}
	}

	if (flags & 8) {
		if (gpMarDirector->unk124 == 2) {
			ReInitializeGX();
			SMS_DrawInit();

			J2DOrthoGraph graph(graphics->mViewportRect);
			graph.setup2D();

			if (unk250) {
				unk3C[0]->mVisible = true;
				unk3C[1]->mVisible = true;
				unk3C[2]->mVisible = true;

				J2DPane* root = unk2C->search('ROOT');
				root->mAlpha  = 0;
				unk2C->draw(0, 0, &graph);
				root->mAlpha = 0xff;

				graph.setup2D();
				unk250 = 0;

				unk3C[0]->mVisible = false;
				unk3C[1]->mVisible = false;
				unk3C[2]->mVisible = false;
			}

			u32 mode = unk248;
			if (mode == 4) {
				for (s8 i = 0; i <= unk274; ++i)
					openWindow(i, (&unk234)[i]);
			}

			if (mode >= 4 && mode < 8) {
				graph.setup2D();
				if (unk28) {
					unk10->draw(0, 0, &graph);
				} else {
					unk90->move(unk330, unk332);
					unk90->mRotation = (f32)unk334;
					unk2C->draw(0, 0, &graph);
				}
			}
		}
	}
}
void TTalk2D2::openWindow(s8 line, f32 offset)
{
	Mtx mtx;
	Mtx work;

	f32 pivotX = unk90->mGlobalBounds.x1 + 5;
	f32 pivotY = unk90->mGlobalBounds.y1 + 5;
	PSMTXTrans(mtx, -pivotX, -pivotY, 0.0f);
	f32 rotation = -unk90->mRotation;
	PSMTXRotRad(work, 'Z', 0.017453292f * rotation);
	PSMTXConcat(work, mtx, mtx);
	PSMTXTrans(work, pivotX, pivotY, 0.0f);
	PSMTXConcat(work, mtx, mtx);
	GXLoadPosMtxImm(mtx, GX_PNMTX0);

	GXSetCullMode(GX_CULL_BACK);
	GXSetNumTexGens(2);
	GXSetNumTevStages(2);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S8, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, 0,
	    GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	    GX_DF_NONE, GX_AF_NONE);
	GXSetChanAmbColor(GX_COLOR0A0, (GXColor) { 0xff, 0xff, 0xff, 0xff });

	J2DPane** paneSlot = &unk3C[line];
	J2DPicture* pane   = (J2DPicture*)*paneSlot;
	pane->getTexture(0)->load(GX_TEXMAP1);
	unk244->load(GX_TEXMAP0);

	GXSetTevColor(GX_TEVREG0, JUtility::TColor(0x0000ff00));
	GXSetTevColor(GX_TEVREG1, JUtility::TColor(0x0000ffa0));
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_C1, GX_CC_TEXC,
	    GX_CC_ZERO);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_A0, GX_CA_A1, GX_CA_TEXA,
	    GX_CA_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	    GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	    GX_TRUE, GX_TEVPREV);

	PSMTXTrans(mtx, offset, 0.0f, 0.0f);
	GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0,
	    GX_FALSE, GX_PTIDENTITY);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);

	GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO,
	    GX_CC_ZERO);
	GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GX_CA_TEXA,
	    GX_CA_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	    GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	    GX_TRUE, GX_TEVPREV);

	PSMTXIdentity(mtx);
	GXLoadTexMtxImm(mtx, GX_TEXMTX1, GX_MTX2x4);
	GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX1,
	    GX_FALSE, GX_PTIDENTITY);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);

	JUTRect rect((*paneSlot)->mGlobalBounds);
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2f32((f32)rect.x1, (f32)rect.y1);
	GXTexCoord2s8(0, 0);
	GXPosition2f32((f32)rect.x2, (f32)rect.y1);
	GXTexCoord2s8(1, 0);
	GXPosition2f32((f32)rect.x2, (f32)rect.y2);
	GXTexCoord2s8(1, 1);
	GXPosition2f32((f32)rect.x1, (f32)rect.y2);
	GXTexCoord2s8(0, 1);
}
void TTalk2D2::setTagParam(JSUMemoryInputStream& stream, J2DTextBox& textBox,
                            int* charIndex, int* line)
{
	u8 tagLength;
	u8 tagType;
	u16 tagId;
	stream.read(&tagLength, 1);
	stream.read(&tagType, 1);
	stream.read(&tagId, 2);

	switch (tagType) {
	case 0:
		switch (tagId) {
		case 0: {
			u8 delay;
			stream.read(&delay, 1);
			unk280 = delay;
			break;
		}
		case 1:
			unk26D = 1;
			break;
		default:
			stream.skip(tagLength - 5);
			break;
		}
		break;
	case 1:
		switch (tagId) {
		case 0:
			if (unk214 == -1) {
				unk214            = 0;
				unk20C[1]->mAlpha = 0xfe;
				unk20C[1]->mVisible = false;
				unk20C[0]->mVisible = true;
			}
			{
				int length = tagLength - 4;
				if (length >= 0x11)
					length = 0x11;
				snprintf(unk218[0], length, "%s", (char*)stream.getCurrent());
			}
			stream.skip(tagLength - 5);
			break;
		case 1:
			if (unk214 == -1) {
				unk214            = 1;
				unk20C[0]->mAlpha = 0xfe;
				unk20C[0]->mVisible = false;
				unk20C[1]->mVisible = true;
			}
			{
				int length = tagLength - 4;
				if (length >= 0x11)
					length = 0x11;
				snprintf(unk218[1], length, "%s", (char*)stream.getCurrent());
			}
			stream.skip(tagLength - 5);
			break;
		default:
			stream.skip(tagLength - 5);
			break;
		}
		break;
	case 2:
		switch (tagId) {
		case 0:
		case 1:
		case 6: {
			int time;
			if (tagId == 0)
				time = TFlagManager::smInstance->getFlag(0x20003);
			else if (tagId == 1)
				time = TFlagManager::smInstance->getFlag(0x20002);
			else
				time = TFlagManager::smInstance->getFlag(0x20014);

			if (time > 599999)
				time = 599999;
			if (time < 0)
				time = 0;

			int minutes = time / 6000;
			int rest    = time - minutes * 6000;
			int seconds = (int)(rest * 0.01);
			int frames  = rest - seconds * 100;
			int index   = *line * 30 + *charIndex;

			snprintf(unk9C[index]->getStringPtr(), 2, "%d", minutes / 10);
			snprintf(unk9C[index + 1]->getStringPtr(), 2, "%d",
			    minutes % 10);
			snprintf(unk9C[index + 2]->getStringPtr(), 2, ":");
			snprintf(unk9C[index + 3]->getStringPtr(), 2, "%d",
			    seconds / 10);
			snprintf(unk9C[index + 4]->getStringPtr(), 2, "%d",
			    seconds % 10);
			snprintf(unk9C[index + 5]->getStringPtr(), 2, ":");
			snprintf(unk9C[index + 6]->getStringPtr(), 2, "%d", frames / 10);
			snprintf(unk9C[index + 7]->getStringPtr(), 2, "%d", frames % 10);

			for (int i = 0; i < 8; ++i) {
				J2DTextBox* box = unk9C[index + i];
				box->mCharColor = unk27C;
				box->mGradColor = unk27C;
				box->mWhite     = unk27C;
				box->mBlack.set((*(u32*)&unk27C) & 0xffffff00);
				unk281[index + i] = unk280;
			}

			*charIndex += 8;
			break;
		}
		case 2: {
			int count = TFlagManager::smInstance->getFlag(0x20004);
			int value = (int)((count + 99) * 0.01f);
			int index = *line * 30 + *charIndex;

			if (value < 10) {
				snprintf(unk9C[index]->getStringPtr(), 2, "%d", value);
				*charIndex += 1;
			} else {
				snprintf(unk9C[index]->getStringPtr(), 2, "%d", value / 10);
				snprintf(unk9C[index + 1]->getStringPtr(), 2, "%d",
				    value % 10);
				*charIndex += 2;
			}
			break;
		}
		case 3: {
			int rest = TFlagManager::smInstance->getFlag(0x40001);
			int cleared = 0;
			for (int flag = 0x46; flag < 0x56; ++flag)
				if (TFlagManager::smInstance->getFlag(0x10000 + flag))
					++cleared;
			for (int flag = 0x6c; flag <= 0x73; ++flag)
				if (TFlagManager::smInstance->getFlag(0x10000 + flag))
					++cleared;

			rest -= cleared * 10;
			int index = *line * 30 + *charIndex;
			if (rest < 100) {
				snprintf(unk9C[index]->getStringPtr(), 2, "%d", rest / 10);
				*charIndex += 1;
			} else {
				int hundreds = rest / 100;
				snprintf(unk9C[index]->getStringPtr(), 2, "%d", hundreds);
				snprintf(unk9C[index + 1]->getStringPtr(), 2, "%d",
				    (rest - hundreds * 100) / 10);
				*charIndex += 2;
			}
			break;
		}
		case 4: {
			u8 basket;
			stream.read(&basket, 1);

			TFruitBasketEvent* event = 0;
			int fruitKind           = 0;
			int target              = 3;
			switch (basket) {
			case 0:
				event = JDrama::TNameRefGen::search<TFruitBasketEvent>(
				    "フルーツかごＡ");
				fruitKind = 0;
				break;
			case 1:
				event = JDrama::TNameRefGen::search<TFruitBasketEvent>(
				    "フルーツかごＢ");
				fruitKind = 4;
				break;
			case 2:
				event = JDrama::TNameRefGen::search<TFruitBasketEvent>(
				    "フルーツかごＣ");
				fruitKind = 3;
				break;
			case 3:
				event = JDrama::TNameRefGen::search<TFruitBasketEvent>(
				    "フルーツかごＤ");
				fruitKind = 1;
				break;
			}

			if (event) {
				int rest = target - event->getFruitNum(fruitKind);
				if (rest < 0 || rest > 9)
					rest = 0;

				int index = *line * 30 + *charIndex;
				snprintf(unk9C[index]->getStringPtr(), 2, "%d", rest);
				snprintf(unk9C[index + 1]->getStringPtr(), 2, " ");
				*charIndex += 2;
			}
			break;
		}
		default:
			break;
		}
		break;
	case 0xff:
		if (tagId == 0) {
			u8 color;
			stream.read(&color, 1);
			unk27C = cColorTable[color];
		}
		break;
	default:
		stream.skip(tagLength - 5);
		break;
	}
}
void TTalk2D2::setupTextBox(const void* data, JMSMesgEntry* entry)
{
	if (unk28) {
		setupBoardTextBox(data, entry);
		return;
	}

	TMessageLoader::EntryInfo* info = (TMessageLoader::EntryInfo*)entry;
	JSUMemoryInputStream input((const u8*)data + info->unk0 + unk278, 0x400);
	unk254 = entry;

	int charIndex = 0;
	int line      = 0;
	unk274       = 0;
	unk2DE       = 0;

	while (line < 3) {
		J2DTextBox* textBox = unk9C[line * 30 + charIndex];
		char* out           = textBox->getStringPtr();

		u8 c;
		input.read(&c, 1);

		switch ((s8)c) {
		case '\n': {
			int last = charIndex == 0 ? 0 : charIndex - 1;
			unk228[line] = last;
			charIndex    = 0;
			makeBoxLine((s8)line, 0);
			++line;
			break;
		}
		case 0:
			if (charIndex != 0) {
				unk228[line] = charIndex - 1;
				charIndex    = 0;
				makeBoxLine((s8)line, 0);
			}
			line   = 3;
			unk26A = 1;
			break;
		case 0x1a:
			setTagParam(input, *textBox, &charIndex, &line);
			break;
		default: {
			if (unk274 != line)
				unk274 = line;

			input.skip(-1);
			input.read(&c, 1);
			out[0] = c;

			if ((u8)out[0] >= 0x80) {
				input.read(&c, 1);
				out[1] = c;
			} else {
				out[1] = 0;
			}

			textBox->mCharColor = unk27C;
			textBox->mGradColor = unk27C;
			textBox->mWhite = unk27C;
			textBox->mBlack.set((*(u32*)&unk27C) & 0xffffff00);

			unk281[unk2DE] = unk280;
			unk2DE         = line * 30 + charIndex;
			++charIndex;
			break;
		}
		}
	}

	if (!unk26A) {
		u8 c;
		input.peek(&c, 1);
		if ((s8)c == 0)
			unk26A = 1;
	}

	if (unk214 != -1) {
		if (unk214 == 0) {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "%s\n\033CC[7f7f7f]\033GC[7f7f7f]%s", unk218[0],
			    unk218[1]);
		} else {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "\033CC[7f7f7f]\033GC[7f7f7f]%s"
			    "\033CC[ffffff]\033GC[ffffff]\n%s",
			    unk218[0], unk218[1]);
		}
	}

	unk278 += input.getPosition();
}
void TTalk2D2::setupBoardTextBox(const void* data, JMSMesgEntry* entry)
{
	TMessageLoader::EntryInfo* info = (TMessageLoader::EntryInfo*)entry;
	JSUMemoryInputStream input((const u8*)data + info->unk0 + unk278, 0x400);
	JSUMemoryOutputStream output(unk18->getStringPtr(), 0x200);
	unk254 = entry;

	int lineCount = 0;
	while (lineCount < 6) {
		u8 c;
		input.read(&c, 1);

		switch (c) {
		case '\n': {
			u8 newline = '\n';
			output.write(&newline, 1);
			++lineCount;
			break;
		}
		case 0:
			unk26A    = 1;
			lineCount = 6;
			break;
		case 0x1a:
			break;
		default:
			input.skip(-1);
			u8 readByte;
			input.read(&readByte, 1);
			u8 writeByte = readByte;
			output.write(&writeByte, 1);
			if (readByte >= 0x80) {
				u8 continuation;
				input.read(&continuation, 1);
				u8 continuationOutput = continuation;
				output.write(&continuationOutput, 1);
			}
			break;
		}
	}

	if (!unk26A) {
		u8 c;
		input.peek(&c, 1);
		if ((s8)c == 0) {
			unk26A = 1;
			input.skip(1);
			c = 0;
			output.write(&c, 1);
		}
	}

	unk278 += input.getPosition();
}
bool TTalk2D2::eraseBoardWindow()
{
	bool result = false;
	u16 alpha   = unk18->mAlpha - 4;

	if ((s16)alpha < 0) {
		alpha                  = 0;
		TMessageLoader* loader = unk260;
		void* data             = loader->unk4;
		JMSMesgEntry* entry
		    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupTextBox(data, entry);
		*(u32*)&unk27C = 0xffffffff;
		unk2DE = 0;
		unk2DC = 0;
		result = true;
	}

	unk18->mAlpha = alpha;
	return result;
}
bool TTalk2D2::eraseNormalWindow()
{
	bool result = false;
	s16 alpha   = unk90->mAlpha - 0x10;

	if (alpha < 0) {
		alpha  = 0xff;
		unk234 = 1.0f;
		unk3C[0]->mVisible = false;
		unk224            = 0;
		unk6C[0]->mVisible = false;
		unk238             = 2.0f;
		unk3C[1]->mVisible = false;
		unk225            = 0;
		unk6C[1]->mVisible = false;
		unk23C             = 3.0f;
		unk3C[2]->mVisible = false;
		unk226            = 0;
		unk6C[2]->mVisible = false;

		for (int i = 0; i < 90; ++i)
			if (unk9C[i])
				unk9C[i]->mVisible = false;

		TMessageLoader* loader = unk260;
		void* data             = loader->unk4;
		JMSMesgEntry* entry
		    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupTextBox(data, entry);
		unk26C = 0;

		if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
			unk340 = 0x20;
		else
			unk340 = 0x40;

		*(u32*)&unk27C = 0xffffffff;
		unk2DE = 0;
		unk2DC = 0;
		result = true;
	}

	unk90->mAlpha = alpha;

	return result;
}
bool TTalk2D2::closeNormalWindow()
{
	bool result = false;
	int alpha   = unk90->mAlpha - 0x10;

	if ((s16)alpha < 0) {
		alpha  = 0;
		unk234 = 1.0f;
		unk3C[0]->mVisible = false;
		unk224            = 0;
		unk6C[0]->mVisible = false;
		unk238             = 2.0f;
		unk3C[1]->mVisible = false;
		unk225            = 0;
		unk6C[1]->mVisible = false;
		unk23C             = 3.0f;
		unk3C[2]->mVisible = false;
		unk226            = 0;
		unk6C[2]->mVisible = false;

		for (int i = 0; i < 90; ++i)
			if (unk9C[i])
				unk9C[i]->mVisible = false;

		if (unk204->mVisible)
			unk204->mVisible = false;

		result = true;
	}

	unk90->mAlpha = alpha;
	if (unk204->mVisible)
		unk204->mAlpha = (u8)alpha;

	return result;
}
void TTalk2D2::checkControler()
{
	if (unk6C[unk274]->mVisible) {
		if (unk26A) {
			if (!unk26D) {
				u32 trigger = *(u32*)((u8*)unk24C + 0xd4);
				if (!(trigger & 0x20000) && !(trigger & 0x40000))
					return;
			}

			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);

			if (unk270 & 1) {
				if (unk264 == 0x19) {
					if (gpMSound->gateCheck(0x4851))
						MSoundSESystem::MSoundSE::startSoundSystemSE(
						    0x4851, 0, 0, 0);
				} else if (unk28) {
					if (gpMSound->gateCheck(0x481a))
						MSoundSESystem::MSoundSE::startSoundSystemSE(
						    0x481a, 0, 0, 0);
					gpMSound->talkModeOut();
				} else {
					gpMSound->talkModeOut();
				}

				gpCamera->makeMtxForPrevTalk();
				gpMarDirector->mConsole->startAppearTelop(false);
				SMSRumbleMgr->finishPause();
				unk252 = 0;
			}

			unk248 = 6;
		} else {
			u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

			if (trigger & 0x60000) {
				if (gpMSound->gateCheck(0x481c))
					MSoundSESystem::MSoundSE::startSoundSystemSE(
					    0x481c, 0, 0, 0);
				unk248 = 7;
			}
		}
	} else if (unk204->mVisible) {
		u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

		if ((trigger & 0x80000) && unk214 == 1) {
			if (gpMSound->gateCheck(0x481e))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481e, 0, 0,
				    0);

			unk214            = 0;
			unk20C[1]->mAlpha = 0xfe;
			unk20C[1]->mVisible = false;
			unk20C[0]->mVisible = true;
		} else if ((trigger & 0x100000) && unk214 == 0) {
			if (gpMSound->gateCheck(0x481e))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481e, 0, 0,
				    0);

			unk214            = 1;
			unk20C[0]->mAlpha = 0xfe;
			unk20C[0]->mVisible = false;
			unk20C[1]->mVisible = true;
		} else if (unk26A) {
			if (trigger & 0x60000) {
				if (unk270 & 1)
					gpCamera->makeMtxForPrevTalk();

				if (gpMSound->gateCheck(0x481c))
					MSoundSESystem::MSoundSE::startSoundSystemSE(
					    0x481c, 0, 0, 0);
				unk248 = 6;
			}
		} else if (trigger & 0x60000) {
			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);
			unk248 = 7;
		}
	}
}
void TTalk2D2::moveTalkWindow()
{
	for (int i = 0; i < 3; ++i) {
		u8& lineCount = (&unk224)[i];
		int textIndex = i * 30 + lineCount;

		if (lineCount != 0 && lineCount <= unk228[i]) {
			J2DTextBox** textBoxSlot = &unk9C[textIndex];
			J2DTextBox* textBox      = *textBoxSlot;

			if (textBox->mVisible) {
				int alpha = textBox->mAlpha + unk340;
				int value = (s16)alpha;
				if (value > 0xff)
					value = 0xff;

				textBox->mAlpha = value;

				if ((s16)alpha >= 0xff)
					++lineCount;
			} else if (unk2DC <= 0) {
				unk2DC                     = unk281[textIndex];
				(*textBoxSlot)->mVisible = true;
				(*textBoxSlot)->mAlpha   = 0;
			} else {
				--unk2DC;
			}
		}
	}

	int line = unk274;
	u8 lineCount = (&unk224)[line];
	if (lineCount < 30 && lineCount <= unk228[line])
		return;

	J2DPane* pane;
	J2DPane* cursor;
	if (unk214 == -1) {
		pane = unk6C[line];

		if (unk26A) {
			unk78[line]->mVisible = false;
			cursor                = unk84[line];
			unk84[line]->mVisible = true;
		} else {
			cursor                = unk78[line];
			unk78[line]->mVisible = true;
			unk84[line]->mVisible = false;
		}
	} else {
		pane   = unk204;
		cursor = unk20C[unk214];
	}

	if (pane->mVisible) {
		int alpha = pane->mAlpha;
		if ((s16)alpha < 0xff) {
			alpha += 0x10;
			if ((s16)alpha >= 0xff)
				alpha = 0xff;
			pane->mAlpha = alpha;
		}

		int cursorAlpha = cursor->mAlpha;
		if (unk26B) {
			cursorAlpha += 2;
			if ((s16)cursorAlpha > 0xff) {
				unk26B      = 0;
				cursorAlpha = 0xff;
			}
		} else {
			cursorAlpha -= 4;
			if ((s16)cursorAlpha < 0x3c) {
				unk26B      = 1;
				cursorAlpha = 0x3c;
			}
		}
		cursor->mAlpha = cursorAlpha;

		u8 alphaByte = cursorAlpha;
		if (unk214 == 1) {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "\033CC[ffffff60]\033GC[ffffff60]%s"
			    "\033CC[ffffff%02x]\033GC[ffffff%02x]\n%s",
			    unk218[0], alphaByte, alphaByte, unk218[1]);
		} else if (unk214 == 0) {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "\033CC[ffffff%02x]\033GC[ffffff%02x]%s\n"
			    "\033CC[ffffff60]\033GC[ffffff60]%s",
			    alphaByte, alphaByte, unk218[0], unk218[1]);
		}
	} else {
		pane->mVisible   = true;
		pane->mAlpha     = 0;
		cursor->mAlpha   = 0xff;
	}
}
void TTalk2D2::checkBoardControler()
{
	if (unk26A) {
		if (!unk26D) {
			u32 trigger = *(u32*)((u8*)unk24C + 0xd4);
			if (!(trigger & 0x20000) && !(trigger & 0x40000))
				return;
		}

		if (gpMSound->gateCheck(0x481c))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0, 0);

		JUTPoint initial(0, -0x258);
		JUTPoint current(0, 0);
		JUTPoint target(0, 0);
		unk14->setPanePosition(0x3c, target, current, initial);

		if (unk270 & 1) {
			if (unk264 == 0x19) {
				if (gpMSound->gateCheck(0x4851))
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x4851, 0,
					    0, 0);
			} else if (unk28) {
				if (gpMSound->gateCheck(0x481a))
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x481a, 0,
					    0, 0);
				gpMSound->talkModeOut();
			} else {
				gpMSound->talkModeOut();
			}

			gpCamera->makeMtxForPrevTalk();
			gpMarDirector->mConsole->startAppearTelop(false);
			SMSRumbleMgr->finishPause();
			unk252 = 0;
		}

		unk248 = 6;
	} else {
		u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

		if (trigger & 0x60000) {
			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);
			unk248 = 7;
		}
	}
}
void TTalk2D2::moveBoardWindow()
{
	int alpha = unk1C->mAlpha;

	if (alpha < 0xff) {
		alpha += 4;
		if (alpha > 0xff) {
			unk24->mAlpha = 0;
			alpha         = 0xff;
			unk20->mAlpha = 0;
			unk26B        = 1;
		}

		unk1C->mAlpha = alpha;
		return;
	}

	J2DPane* pane = unk26A ? unk24 : unk20;
	alpha         = pane->mAlpha;

	if (unk26B) {
		alpha += 4;
		if (alpha > 0xff) {
			unk26B = 0;
			alpha  = 0xff;
		}
	} else {
		alpha -= 4;
		if (alpha < 0) {
			unk26B = 1;
			alpha  = 0;
		}
	}

	pane->mAlpha = alpha;
}
bool TTalk2D2::openNormalWindow()
{
	bool result = false;

	if (unk2DE > 2 && (*(u32*)((u8*)unk24C + 0xd0) & 0x20000)) {
		unk26C = 1;
		if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
			unk340 = 0x5a;
		else
			unk340 = 0x80;
	}

	bool allSettled = true;

	for (int i = 0; i <= unk274; ++i) {
		f32* lineMove = &unk234 + i;

		if (*lineMove > -0.109f) {
			*lineMove -= unk338;
			allSettled = false;
		}

		if (*lineMove < 1.0f && i != 0) {
			int prevIndex = unk228[i - 1] + (i - 1) * 30;
			if (!unk9C[prevIndex]->mVisible)
				*lineMove = 1.0f;
		}

		if ((&unk224)[i] == 0 && *lineMove < unk33C) {
			if (gpMSound->gateCheck(0x4827))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x4827, 0, 0,
				                                             0);

			(&unk224)[i]             = 1;
			unk9C[i * 30]->mVisible = true;
		}
	}

	if (allSettled) {
		for (int i = 0; i <= unk274; ++i)
			unk3C[i]->mVisible = true;
		result = true;
	}

	for (int line = 0; line < 3; ++line) {
		if ((&unk224)[line] == 0)
			continue;

		int textIndex = (&unk224)[line] + line * 30;

		while ((&unk224)[line] <= unk228[line]) {
			J2DTextBox** textBoxSlot = &unk9C[textIndex];
			J2DTextBox* textBox      = *textBoxSlot;

			if (textBox->mVisible) {
				int alphaSum = textBox->mAlpha + unk340;
				s16 alpha    = alphaSum;
				if (alpha > 0xff)
					alpha = 0xff;

				textBox->mAlpha = alpha;
				unk2DE          = textIndex;

				if ((s16)alphaSum < 0xff)
					break;

				++textIndex;
				++(&unk224)[line];
				continue;
			}

			if (unk2DC <= 0) {
				if (unk26C)
					unk2DC = 0;
				else
					unk2DC = unk281[textIndex];

				(*textBoxSlot)->mVisible = true;
				(*textBoxSlot)->mAlpha   = 0;
			} else {
				--unk2DC;
			}
			break;
		}
	}

	return result;
}
bool TTalk2D2::openBoardWindow()
{
	bool result = false;

	switch (unk29[0]) {
	case 0:
		if (unk14->update()) {
			unk14->setPanePosition(0x19, JUTPoint(0, 0x50),
			                       JUTPoint(0, 0x50), JUTPoint(0, 0));
			++unk29[0];
		}
		break;
	case 1:
		if (unk14->update()) {
			unk1C->mAlpha   = 0;
			unk1C->mVisible = true;
			result          = true;
			++unk29[0];
		}
		break;
	}

	return result;
}
void TTalk2D2::makeBoxLine(s8 line, char* text)
{
	int lineIndex = line;
	int baseIndex = lineIndex * 30;
	int textIndex = 0;

	J2DPane* linePane = unk30[lineIndex];
	J2DPane* start    = unk48[lineIndex];
	J2DPane* control  = unk54[lineIndex];
	J2DPane* end      = unk60[lineIndex];

	f32 startX = start->mBounds.x1;
	f32 startY = start->mBounds.y1;
	f32 ctrlX  = control->mBounds.x1;
	f32 ctrlY  = control->mBounds.y1 - unk220;
	f32 endX   = end->mBounds.x2 - 10;
	f32 endY   = end->mBounds.y1;

	f32 prevX = startX;
	f32 prevY = startY;
	f32 t     = 0.0f;

	for (int i = 0; i < 30; ++i) {
		J2DTextBox* textBox = unk9C[baseIndex + i];

		if (text) {
			char* dst = textBox->getStringPtr();
			u8 c      = text[textIndex];
			dst[0]    = c;
			if (c >= 0x80) {
				dst[1] = text[textIndex + 1];
				textIndex += 2;
			} else {
				dst[1] = 0;
				++textIndex;
			}
		}

		char* string = textBox->getStringPtr();
		u8 c         = string[0];
		if ((s8)c == 0)
			return;

		JUTFont::TWidth width;
		gpSystemFont->getWidthEntry((u16)(s8)c, &width);

		if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100) {
			t += unk94 * width.field_0x1;
		} else {
			t += unk94 * (0.7f * width.field_0x1 + 4.0f);
		}

		f32 inv  = 1.0f - t;
		f32 endXt2 = t * t * endX;
		f32 endYt2 = t * t * endY;
		f32 x = endXt2 + (inv * inv * startX + 2.0f * t * inv * ctrlX);
		f32 y = endYt2 + (inv * inv * startY + 2.0f * t * inv * ctrlY);

		f32 dx    = x - prevX;
		f32 dy    = y - prevY;
		f32 angle = fabsf(atan2f(dy, dx));
		if (x > 0.0f)
			angle = angle * -1.0f;

		f32 cosV = cosf(angle);
		f32 sinV = sinf(angle);
		f32 sumX = prevX + x;
		f32 sumY = prevY + y;
		f32 mvX
		    = -0.5f * sumX + (prevX * cosV - prevY * sinV) + 0.5f * sumX;
		f32 mvY
		    = -0.5f * sumY + (prevX * sinV + prevY * cosV) + 0.5f * sumY;

		int moveX = mvX > 0.0f ? mvX + 0.5f : mvX - 0.5f;
		int moveY = mvY > 0.0f ? mvY + 0.5f : mvY - 0.5f;
		textBox->move((s16)moveX, (s16)(-0x25 - moveY));
		textBox->setBasePosition(J2DBasePosition_4);
		textBox->mRotation = angle * 180.0f / 3.1415927f;
		linePane->mPaneTree.appendChild(&textBox->mPaneTree);

		if (t > 1.1f) {
			if (unk228[lineIndex] > i)
				unk228[lineIndex] = i;
			return;
		}

		prevX = x;
		prevY = y;
	}
}
void TTalk2D2::openTalkWindow(TBaseNPC* npc)
{
	if (npc)
		gpCamera->makeMtxForTalk(npc);

	if (unk28) {
		JUTPoint end(0, 0x50);
		JUTPoint current(0, 0x50);
		JUTPoint start(0, -0x320);
		unk14->setPanePosition(0x3c, start, current, end);
		unk14->update();

		unk1C->mAlpha = 0;
		unk248        = 4;
	} else {
		unk248 = 2;
	}

	unk90->mAlpha = 0xff;

	switch (gpCamera->mMode) {
	case 0x2d:
		unk330 = 0xa0;
		unk332 = 0x87;
		unk334 = 0x14;
		break;
	case 0xc:
	default:
		unk330 = 0x184;
		unk332 = 0x73;
		unk334 = -0x12;
		break;
	}

	unk90->move(unk330, unk332);
	unk90->mRotation = unk334;
	unk250           = 1;

	gpMarDirector->mConsole->startDisappearTelop();
	{
		TGCConsole2* console = gpMarDirector->mConsole;
		console->startDisappearBalloon(console->unk3E0, true);
	}
	gpMarDirector->mConsole->startDisappearMario();

	if (unk264 == 0x19) {
		if (gpMSound->gateCheck(0x4848))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4848, 0, 0, 0);
	} else if (unk28) {
		if (gpMSound->gateCheck(0x4819))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4819, 0, 0, 0);
		gpMSound->talkModeIn(false);
	} else {
		gpMSound->talkModeIn(true);
	}

	SMSRumbleMgr->startPause();
}
void TTalk2D2::forceCloseTalk()
{
	gpCamera->makeMtxForPrevTalk();

	if (unk28) {
		if (gpMSound->gateCheck(0x4851))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4851, 0, 0, 0);
	} else {
		gpMSound->talkModeOut();
	}

	gpMarDirector->mConsole->startAppearTelop(false);

	if (unk248 == 1)
		unk248 = 0;
	else
		unk248 = 6;
}
void TTalk2D2::setMessageID(u32 message_id, u32 flags)
{
	TBaseNPC* npc = gpMarDirector->unkA0;

	if (npc->mActionFlag & 0x200) {
		bool isMonte = true;
		bool isMonteType = isMonte;

		if (!npc->isNormalMonteM() && !npc->isNormalMonteW())
			isMonteType = false;

		if (!isMonteType) {
			isMonteType = true;
			if (!npc->isSpecialMonteM() && !npc->isSpecialMonteW())
				isMonteType = false;
			if (!isMonteType)
				isMonte = false;
		}

		if (isMonte) {
			if (npc->isNormalMonteW() || npc->isSpecialMonteW()) {
				if (npc->isChild())
					unk264 = 0x2c;
				else
					unk264 = 0x27;
			} else {
				if (npc->isChild())
					unk264 = 0x29;
				else
					unk264 = 0x23;
			}
		} else {
			bool isMare = true;
			bool isMareType = isMare;

			if (!npc->isNormalMareM() && !npc->isNormalMareW())
				isMareType = false;

			if (!isMareType) {
				isMareType = true;
				if (!npc->isSpecialMareM() && !npc->isSpecialMareW())
					isMareType = false;
				if (!isMareType)
					isMare = false;
			}

			if (isMare) {
				if (npc->isNormalMareW() || npc->isSpecialMareW()) {
					if (npc->isChild())
						unk264 = 0x2d;
					else
						unk264 = 0x28;
				} else {
					if (npc->isChild())
						unk264 = 0x2a;
					else
						unk264 = 0x24;
				}
			} else if (npc->mActorType == 0x04000016) {
				unk264 = 0x25;
			} else if (npc->mActorType == 0x04000010) {
				unk264 = 0x2b;
			}
		}

		for (int i = 0; i < 10; ++i) {
			if (unk2E0[i].unk0 == npc) {
				unk264 = unk2E0[i].unk4;
				break;
			}
		}
	} else {
		unk264 = message_id;
	}

	if (npc->mActorType == 0x0400001d)
		unk28 = 1;
	else
		unk28 = 0;

	unk270 = flags;
	unk278 = 0;
	unk214 = -1;
	unk26A = 0;
	*(u32*)&unk27C = 0xffffffff;
	unk280 = 0;
	unk26C = 0;
	unk26D = 0;
	unk254 = 0;
	unk29[0] = 0;

	if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
		unk340 = 0x20;
	else
		unk340 = 0x40;

	TMessageLoader* loader;
	if (unk264 & 0xffff0000)
		loader = unk258;
	else
		loader = unk25C;

	JMSMesgEntry* entry;
	if (loader->unk4) {
		entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		if (!entry) {
			unk264 = 4;
			loader = unk25C;
			entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		}
		setupTextBox(loader->unk4, entry);
	} else {
		unk264 = 3;
		loader = unk25C;
		entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupTextBox(loader->unk4, entry);
	}

	unk260 = loader;
	unk2DC = 0;

	if (unk254) {
		s32 sound = scTalkSoundList[((u8*)unk254)[8]];
		if (sound != -1 && gpMSound->gateCheck(sound)) {
			if ((u32)sound & 0x80000000)
				MSBgm::startBGM(sound);
			else
				MSoundSESystem::MSoundSE::startSoundSystemSE(sound, 0, 0, 0);
		}
	}

	if (unk248 == 1) {
		unk248 = 3;
		unk90->mAlpha = 0xff;
	}

	unk252 = 1;
}
