#include <System/PositionHolder.hpp>

TNameRefAryT<TStagePositionInfo>* gpPositionHolder;

void TStagePositionInfo::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
	stream.read(&unkC.x, sizeof(f32));
	stream.read(&unkC.y, sizeof(f32));
	stream.read(&unkC.z, sizeof(f32));

	f32 unused0;
	f32 unused1;
	f32 unused2;
	f32 unused3;
	f32 unused4;
	f32 unused5;
	stream.read(&unused0, sizeof(f32));
	stream.read(&unused1, sizeof(f32));
	stream.read(&unused2, sizeof(f32));
	stream.read(&unused3, sizeof(f32));
	stream.read(&unused4, sizeof(f32));
	stream.read(&unused5, sizeof(f32));
}
