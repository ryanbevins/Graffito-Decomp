#ifndef ENEMY_KOOPA_HPP
#define ENEMY_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>

class TKoopa : public TSpineEnemy {
public:
	TKoopa(const char*);
	virtual ~TKoopa();

	/* 0x150 */ u8 unk150[0x6C];
};

class TKoopaManager : public TEnemyManager {
public:
	TKoopaManager(const char*);
	virtual ~TKoopaManager();
};

class TKoopaJr : public TSpineEnemy {
public:
	TKoopaJr(const char*);
	virtual ~TKoopaJr();

	/* 0x150 */ u8 unk150[0x20];
};

class TKoopaJrManager : public TEnemyManager {
public:
	TKoopaJrManager(const char*);
	virtual ~TKoopaJrManager();
};

class TKoopaJrSubmarine : public TSpineEnemy {
public:
	TKoopaJrSubmarine(const char*);
	virtual ~TKoopaJrSubmarine();

	/* 0x150 */ u8 unk150[0x5C];
};

class TKoopaJrSubmarineManager : public TEnemyManager {
public:
	TKoopaJrSubmarineManager(const char*);
	virtual ~TKoopaJrSubmarineManager();
};

#endif
