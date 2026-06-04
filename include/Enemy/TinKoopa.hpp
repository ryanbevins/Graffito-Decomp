#ifndef ENEMY_TIN_KOOPA_HPP
#define ENEMY_TIN_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>

class TTinKoopa : public TSpineEnemy {
public:
	TTinKoopa(const char*);
	virtual ~TTinKoopa();

	/* 0x150 */ u8 unk150[0xAC];
};

class TTinKoopaManager : public TEnemyManager {
public:
	TTinKoopaManager(const char*);
	virtual ~TTinKoopaManager();
};

#endif
