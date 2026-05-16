#ifndef MAP_POLLUTION_TEST_HPP
#define MAP_POLLUTION_TEST_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>

class TPollutionTest : public JDrama::TViewObj {
public:
	TPollutionTest(const char* name)
	    : TViewObj(name)
	{
	}

	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
};

#endif
