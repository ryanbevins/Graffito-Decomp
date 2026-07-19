#ifndef GC2D_MESSAGE_LOADER_HPP
#define GC2D_MESSAGE_LOADER_HPP

#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <dolphin/types.h>

class TMessageLoader {
public:
	struct EntryInfo {
		/* 0x0 */ u32 unk0;
		/* 0x4 */ s16 unk4;
		/* 0x6 */ s16 unk6;
		/* 0x8 */ char unk8[0x4];
	};

	TMessageLoader();
	TMessageLoader(const char*);

	u32 loadMessageData(const char*);
	void readHeader(u32* size, u32* count, void* header)
	{
		u32* words = (u32*)header;
		*size      = words[2] * 32;
		*count     = words[3];
	}
	void* parseBlock(u32, u32, void*);
	EntryInfo* getMessageEntry(u32);
	int readInfoBlock(void* data)
	{
		int length = *(int*)data;
		data       = (u8*)data + 4;
		JSUMemoryInputStream stream(data, length - 8);
		stream.read(&unk0, 2);
		stream.readU16();
		unk2 = stream.readU16();
		stream.skip(2);

		for (int i = 0; i < unk0; ++i)
			stream.read(&unk8[i], sizeof(EntryInfo));

		return length;
	}

	u16 getMessageNum() const { return unk0; }
	const u8* getMessageData() const { return (u8*)unk4; }

public:
	/* 0x0 */ u16 unk0;
	/* 0x2 */ u16 unk2;
	/* 0x4 */ void* unk4;
	/* 0x8 */ EntryInfo unk8[255];
};

#endif
