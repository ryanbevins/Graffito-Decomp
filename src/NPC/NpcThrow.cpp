#include <NPC/NpcThrow.hpp>
#include <Strategic/HitActor.hpp>
#include <Player/MarioAccess.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>

void TNpcThrow::throwMario(THitActor* mario)
{
	JGeometry::TVec3<f32> dir;
	if (unk4 >= 90.0f) {
		dir.set(0.0f, 1.0f, 0.0f);
	} else if (unk4 <= 0.0f) {
		dir.set(0.0f, 0.0f, -1.0f);
	} else {
		s16 angle = CLBDegToShortAngle(unk4);
		dir.set(0.0f, JMASSin(angle), -JMASCos(angle));
	}

	s16 yawShort = CLBDegToShortAngle(mario->mRotation.y);
	f32 sin_a    = JMASSin(yawShort);
	f32 cos_a    = JMASCos(yawShort);
	f32 oldX     = dir.x;
	dir.x        = oldX * cos_a + dir.z * sin_a;
	dir.z        = -oldX * sin_a + dir.z * cos_a;
	SMS_SendMessageToMario(mario, HIT_MESSAGE_UNK7);
	SMS_ThrowMario(dir, unk0);
}
