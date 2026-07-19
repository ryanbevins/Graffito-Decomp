#include <GC2D/MessageLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>

TMessageLoader::TMessageLoader()
    : unk0(0)
    , unk4(0)
{
}

TMessageLoader::TMessageLoader(const char* param_1)
    : unk0(0)
    , unk4(0)
{
	u8* res = (u8*)JKRGetResource(param_1);
	if (res) {
		u32 a;
		u32 b;
		readHeader(&a, &b, res);
		unk4 = parseBlock(a, b, res + 0x20);
		// NOTE: assert but in an if?
		if (unk4)
			(void)unk4;
	}
}

u32 TMessageLoader::loadMessageData(const char* param_1)
{
	u8* res = (u8*)JKRGetResource(param_1);
	if (!res)
		return -1;

	unk4 = parseBlock(*(int*)(res + 0x8) * 32, *(int*)(res + 0xC), res + 0x20);
	if (!unk4)
		return -1;

	return unk2;
}

void* TMessageLoader::parseBlock(u32 param_1, u32 param_2, void* param_3)
{
	JSUMemoryInputStream local_5c(param_3, param_1);

	void* result;

	for (int i = 0; i < param_2; ++i) {
		int local_74;
		local_5c.read(&local_74, 4);

		int local_70;

		switch (local_74) {
		case 'INF1': {
			void* data = local_5c.getCurrent();
			local_70   = readInfoBlock(data);
			local_5c.skip(4);
			break;
		}

		case 'DAT1':
			local_5c.read(&local_70, 4);
			result = local_5c.getCurrent();
			break;

		case 'STR1':
			local_70 = 0;
			break;

		default:
			local_70 = 0;
			break;
		}

		local_5c.skip(local_70 - 8);
	}

	return result;
}

TMessageLoader::EntryInfo* TMessageLoader::getMessageEntry(u32 param_1)
{
	EntryInfo* result;
	if (u16(param_1) >= unk0)
		result = nullptr;
	else
		result = &unk8[param_1];

	return result;
}
