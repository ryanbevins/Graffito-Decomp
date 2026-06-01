#include <M3DUtil/MActorUtil.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>

MActor* SMS_MakeMActorFromSDLModelData(SDLModelData* param_1,
                                       MActorAnmData* param_2, u32 param_3)
{
	SDLModel* model = new SDLModel(param_1, param_3, 1);
	MActor* actor   = new MActor(param_2);
	actor->setModel(model, 0);
	return actor;
}

SDLModelData* SMS_MakeSDLModelData(const char* param_1, u32 param_2)
{
	void* res = JKRGetResource(param_1);

	J3DModelData* j3ddata = J3DModelLoaderDataBase::load(res, param_2);
	SDLModelData* sdlData = new SDLModelData(j3ddata);

	return sdlData;
}

#pragma dont_inline on
MActor** SMS_MakeMActorsWithAnmData(const char* param_1, MActorAnmData* param_2,
                                    int param_3, u32 param_4, u32 param_5)
{
	SDLModelData* sdlData = SMS_MakeSDLModelData(param_1, param_5);

	MActor** actors = new MActor*[param_3];
	for (int i = 0; i < param_3; ++i)
		actors[i] = SMS_MakeMActorFromSDLModelData(sdlData, param_2, param_4);

	return actors;
}
#pragma dont_inline off

MActor* SMS_MakeMActorWithAnmData(const char* param_1, MActorAnmData* param_2,
                                  u32 param_3, u32 param_4)
{
	SDLModelData* sdlData = SMS_MakeSDLModelData(param_1, param_4);
	MActor** actors       = new MActor*[1];
	actors[0] = SMS_MakeMActorFromSDLModelData(sdlData, param_2, param_3);

	return actors[0];
}

MActor* SMS_MakeMActor(const char* param_1, const char* param_2, u32 param_3,
                       u32 param_4)
{
	MActorAnmData* anm = new MActorAnmData;
	anm->init(param_1, nullptr);
	MActor** actors = SMS_MakeMActorsWithAnmData(param_2, anm, 1, param_3,
	                                             param_4);

	return actors[0];
}
