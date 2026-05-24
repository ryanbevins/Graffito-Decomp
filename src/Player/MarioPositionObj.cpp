#include <Player/MarioPositionObj.hpp>

void TMarioPositionObj::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	f32 dummy1;
	f32 dummy2;
	f32 dummy3;
	char nameBuf[0x50];
	u32 i;
	for (i = 0; i < 8
	             && (u32)stream.getLength()
	                    > (u32)stream.getPosition() + 0x24;
	     i++) {
		stream.readString(nameBuf, 0x50);
		stream.read(&unk10[i].x, sizeof(f32));
		stream.read(&unk10[i].y, sizeof(f32));
		stream.read(&unk10[i].z, sizeof(f32));
		stream.read(&unk70[i].x, sizeof(f32));
		stream.read(&unk70[i].y, sizeof(f32));
		stream.read(&unk70[i].z, sizeof(f32));
		stream.read(&dummy1, sizeof(f32));
		stream.read(&dummy2, sizeof(f32));
		stream.read(&dummy3, sizeof(f32));
	}
	unkD0 = i;
}

void TMarioPositionObj::perform(unsigned long flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}
