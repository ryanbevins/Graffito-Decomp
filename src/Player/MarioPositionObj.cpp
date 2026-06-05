#include <Player/MarioPositionObj.hpp>

void TMarioPositionObj::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	char nameBuf[0x50];
	f32 dummy1;
	f32 dummy2;
	f32 dummy3;
	JSUMemoryInputStream& streamY     = stream;
	JSUMemoryInputStream& streamZ     = stream;
	JSUMemoryInputStream& streamRotY  = stream;
	JSUMemoryInputStream& streamRotZ  = stream;
	JSUMemoryInputStream& streamDummy = stream;
	f32* dummy2Ptr                    = &dummy2;
	f32* dummy3Ptr                    = &dummy3;
	u32 i;
	for (i = 0; i < 8
	             && (u32)stream.getLength()
	                    > (u32)stream.getPosition() + 0x24;
	     i++) {
		stream.readString(nameBuf, 0x50);
		stream.read(&unk10[i].x, sizeof(f32));
		streamY.read(&unk10[i].y, sizeof(f32));
		streamZ.read(&unk10[i].z, sizeof(f32));
		stream.read(&unk70[i].x, sizeof(f32));
		streamRotY.read(&unk70[i].y, sizeof(f32));
		streamRotZ.read(&unk70[i].z, sizeof(f32));
		stream.read(&dummy1, sizeof(f32));
		stream.read(dummy2Ptr, sizeof(f32));
		streamDummy.read(dummy3Ptr, sizeof(f32));
	}
	unkD0 = i;
}

void TMarioPositionObj::perform(unsigned long flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}
