#include <Enemy/AreaCylinder.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <JSystem/JGeometry.hpp>

static inline void listPushBack(JGadget::TList<TAreaCylinder*>& list,
                                TAreaCylinder* cyl)
{
	list.insert(list.end(), cyl);
}

TAreaCylinder::TAreaCylinder(const char* name)
    : JDrama::TViewObj(name)
{
	unk18 = 0.0f;
	unk14 = 0.0f;
	unk10 = 0.0f;
	unk1C = 0.0f;
}

void TAreaCylinder::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	stream.read(&unk10, 4);
	stream.read(&unk14, 4);
	stream.read(&unk18, 4);

	JGeometry::TVec3<f32> tmp;
	stream.read(&tmp.x, 4);
	stream.read(&tmp.y, 4);
	stream.read(&tmp.z, 4);

	stream.read(&unk1C, 4);
	stream.read(&unk20, 4);
	stream.read(&tmp.z, 4);

	unk1C *= 50.0f;
	unk20 *= 50.0f;

	stream.readString();

	int count;
	stream.read(&count, 4);
	int loopMax = count;
	for (int i = 0; i < loopMax; ++i) {
		int v;
		stream.read(&v, 4);
		stream.readString();
	}

	const char* groupName = stream.readString();
	TConductor* conductor = gpConductor;
	TAreaCylinderManager* mgr
	    = (TAreaCylinderManager*)conductor->searchF(
	        JDrama::TNameRef::calcKeyCode(groupName), groupName);
	if (mgr == nullptr) {
		mgr = new TAreaCylinderManager(groupName);
		gpConductor->registerAreaCylinderManager(mgr);
	}
	listPushBack(mgr->mList, this);

	int rawAngle;
	stream.read(&rawAngle, 4);
	unk24 = (f32)rawAngle / 100.0f;
}

void TAreaCylinder::perform(u32, JDrama::TGraphics*) { }

static inline BOOL insideCylinder(const TAreaCylinder* cyl,
                                  const JGeometry::TVec3<f32>& pos)
{
	if (pos.y < cyl->unk14 || cyl->unk14 + cyl->unk20 < pos.y)
		return FALSE;
	f32 dx   = pos.x - cyl->unk10;
	f32 dxSq = dx * dx;
	f32 dz   = pos.z - cyl->unk18;
	f32 dzSq = dz * dz;
	f32 rSq  = cyl->unk1C * cyl->unk1C;
	return dxSq + dzSq <= rSq;
}

BOOL TAreaCylinderManager::contain(const JGeometry::TVec3<f32>& pos)
{
	for (JGadget::TList<TAreaCylinder*>::iterator it = mList.begin();
	     it != mList.end(); ++it) {
		TAreaCylinder* cyl = *it;
		if (insideCylinder(cyl, pos)) {
			return TRUE;
		}
	}
	return FALSE;
}

TAreaCylinder*
TAreaCylinderManager::getCylinderContains(const JGeometry::TVec3<f32>& pos)
{
	for (JGadget::TList<TAreaCylinder*>::iterator it = mList.begin();
	     it != mList.end(); ++it) {
		TAreaCylinder* cyl = *it;
		if (insideCylinder(cyl, pos)) {
			return cyl;
		}
	}
	return nullptr;
}

void TAreaCylinderManager::perform(u32, JDrama::TGraphics*) { }
