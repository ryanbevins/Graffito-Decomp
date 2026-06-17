#include <JSystem/JDrama/JDRDrawBufObj.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DDrawBuffer.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DPacket.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <dolphin/os.h>

using namespace JDrama;

static u32 countDrawBufferPackets(J3DDrawBuffer* buffer, u32* nonEmptySlots)
{
	u32 count = 0;
	*nonEmptySlots = 0;

	if (!buffer || !buffer->mBuffer)
		return 0;

	for (u32 i = 0; i < buffer->mSize; ++i) {
		if (buffer->mBuffer[i])
			++*nonEmptySlots;

		for (J3DPacket* packet = buffer->mBuffer[i]; packet;
		     packet            = packet->getNextPacket()) {
			++count;
			if (count >= 4096)
				return count;
		}
	}

	return count;
}

TDrawBufObj::TDrawBufObj()
    : TViewObj("<DrawBufObj>")
    , mDrawBuffer(nullptr)
    , mDrawBufferSize(0)
    , unk18(7)
{
}

TDrawBufObj::TDrawBufObj(u32 param_1, u32 param_2, const char* name)
    : TViewObj(name)
    , mDrawBufferSize(param_2)
    , unk18(param_1)
{
	mDrawBuffer = new J3DDrawBuffer(mDrawBufferSize);
}

void TDrawBufObj::load(JSUMemoryInputStream& stream)
{
	TNameRef::load(stream);
	stream.read(&unk18, sizeof(u32));
	stream.read(&mDrawBufferSize, sizeof(u32));
	mDrawBuffer = new J3DDrawBuffer(mDrawBufferSize);
}

void TDrawBufObj::perform(u32 param_1, TGraphics* param_2)
{
	if ((param_1 & 0x80))
		mDrawBuffer->frameInit();

	if ((param_1 & 0x400)) {
		if ((unk18 & 3))
			j3dSys.mDrawBuffer[0] = mDrawBuffer;

		if ((unk18 & 4))
			j3dSys.mDrawBuffer[1] = mDrawBuffer;
	}

	if ((param_1 & 8)) {
		static s32 sDrawBufProbeCount;

		if (sDrawBufProbeCount < 256) {
			u32 nonEmptySlots;
			u32 packetCount = countDrawBufferPackets(mDrawBuffer, &nonEmptySlots);
			OSReport((char*)"DRAWBUF_PERFORM name=%s buf=%p size=%lu unk18=%08x drawType=%d sortType=%d slots=%lu packets=%lu\n",
			         getName(), mDrawBuffer,
			         mDrawBuffer ? mDrawBuffer->mSize : 0, unk18,
			         mDrawBuffer ? mDrawBuffer->mDrawType : -1,
			         mDrawBuffer ? mDrawBuffer->mSortType : -1, nonEmptySlots,
			         packetCount);
			++sDrawBufProbeCount;
		}

		j3dSys.unk4C = unk18;
		mDrawBuffer->draw();
	}
}
