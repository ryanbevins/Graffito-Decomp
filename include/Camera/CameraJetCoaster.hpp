#ifndef CAMERA_CAMERAJETCOASTER_HPP
#define CAMERA_CAMERAJETCOASTER_HPP

#include <System/ParamInst.hpp>
#include <System/Params.hpp>

class TCamSaveJetCoaster : public TParams {
public:
	TCamSaveJetCoaster();

	/* 0x08 */ TParamRT<s16> mSLOffsetAngleXLimit;
	/* 0x1C */ TParamRT<s16> mSLOffsetAngleYLimit;
	/* 0x30 */ TParamRT<s16> mSLOffsetAngleXManualSpeed;
	/* 0x44 */ TParamRT<s16> mSLOffsetAngleYManualSpeed;
	/* 0x58 */ TParamRT<s16> mSLOffsetAngleXChase;
	/* 0x6C */ TParamRT<s16> mSLOffsetAngleYChase;
};

#endif
