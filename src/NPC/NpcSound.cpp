#include <NPC/NpcBase.hpp>

static const u32 s1490[] = { 0, 0, 0 };

static const char s1526[] = "\203\201\203\202\203\212\202\252\221\253\202\350\202\334\202\271"
      "\202\361\n";

static const char s1755[] = "MActorMtxCalcType_Basic \203N\203\211\203V\203b\203N\203X\203P"
      "\201[\203\213\202n\202m";

static const char s1756[] = "MActorMtxCalcType_Softimage \203N\203\211\203V\203b\203N\203X"
      "\203P\201[\203\213\202n\202e\202e";

static const char s1757[] = "MActorMtxCalcType_MotionBlend \203\202\201[\203V\203\207\203\223"
      "\203u\203\214\203\223\203h";

static const char s1758[] = "MActorMtxCalcType_User \203\206\201[\203U\201[\222\350\213`";

static const char s1938[] = "/scene/monteMCommon/bas/mom_appear.bas";

static const char s1939[] = "/scene/monteMCommon/bas/mom_dance.bas";

static const char s1940[] = "/scene/monteMCommon/bas/mom_fall.bas";

static const char s1941[] = "/scene/monteMCommon/bas/mom_happy.bas";

static const char s1942[] = "/scene/monteMCommon/bas/mom_mad.bas";

static const char s1943[] = "/scene/monteMCommon/bas/mom_madloop.bas";

static const char s1944[] = "/scene/monteMCommon/bas/mom_recover.bas";

static const char s1945[] = "/scene/monteMCommon/bas/mom_run.bas";

static const char s1946[] = "/scene/monteMCommon/bas/mom_sitwet.bas";

static const char s1947[] = "/scene/monteMCommon/bas/mom_throw.bas";

static const char s1948[] = "/scene/monteMCommon/bas/mom_wait_yogore.bas";

static const char s1949[] = "/scene/monteMCommon/bas/mom_walk.bas";

static const char s1950[] = "/scene/monteMCommon/bas/mom_wet.bas";

static const char s1951[] = "/scene/monteMCommon/bas/mom_wet_A.bas";

static const char s1952[] = "/scene/monteMCommon/bas/mom_wet_yogore.bas";

static const char s1953[] = "/scene/monteMCommon/bas/mom_appear_c.bas";

static const char s1954[] = "/scene/monteMCommon/bas/mom_dance_c.bas";

static const char s1955[] = "/scene/monteMCommon/bas/mom_fall_c.bas";

static const char s1956[] = "/scene/monteMCommon/bas/mom_happy_c.bas";

static const char s1957[] = "/scene/monteMCommon/bas/mom_mad_c.bas";

static const char s1958[] = "/scene/monteMCommon/bas/mom_madloop_c.bas";

static const char s1959[] = "/scene/monteMCommon/bas/mom_recover_c.bas";

static const char s1960[] = "/scene/monteMCommon/bas/mom_run_c.bas";

static const char s1961[] = "/scene/monteMCommon/bas/mom_sitwet_c.bas";

static const char s1962[] = "/scene/monteMCommon/bas/mom_throw_c.bas";

static const char s1963[] = "/scene/monteMCommon/bas/mom_wait_yogore_c.bas";

static const char s1964[] = "/scene/monteMCommon/bas/mom_walk_c.bas";

static const char s1965[] = "/scene/monteMCommon/bas/mom_wet_c.bas";

static const char s1966[] = "/scene/monteMCommon/bas/mom_wet_A_c.bas";

static const char s1967[] = "/scene/monteMCommon/bas/mom_wet_yogore_c.bas";

static const char s1970[] = "/scene/monteME/bas/momE_down.bas";

static const char s1971[] = "/scene/monteME/bas/momE_up.bas";

static const char s1972[] = "/scene/monteME/bas/momE_wet.bas";

static const char s1975[] = "/scene/monteMF/bas/momF_swim.bas";

static const char s1976[] = "/scene/monteMF/bas/momF_swimmad.bas";

static const char s1977[] = "/scene/monteMF/bas/momF_swimtalk.bas";

static const char s1980[] = "/scene/monteMG/bas/momG_wait_cleanup.bas";

static const char s1981[] = "/scene/monteMG/bas/momG_walk_cleanup.bas";

static const char s1984[] = "/scene/monteMH/bas/momH_play.bas";

static const char s1985[] = "/scene/monteMH/bas/momH_walk_play.bas";

static const char s1986[] = "/scene/monteMH/bas/momH_wet_play.bas";

static const char s1989[] = "/scene/monteWCommon/bas/mow_appear.bas";

static const char s1990[] = "/scene/monteWCommon/bas/mow_dance.bas";

static const char s1991[] = "/scene/monteWCommon/bas/mow_fall.bas";

static const char s1992[] = "/scene/monteWCommon/bas/mow_happy.bas";

static const char s1993[] = "/scene/monteWCommon/bas/mow_mad.bas";

static const char s1994[] = "/scene/monteWCommon/bas/mow_madloop.bas";

static const char s1995[] = "/scene/monteWCommon/bas/mow_run.bas";

static const char s1996[] = "/scene/monteWCommon/bas/mow_sitwet.bas";

static const char s1997[] = "/scene/monteWCommon/bas/mow_throw.bas";

static const char s1998[] = "/scene/monteWCommon/bas/mow_wait_arrow.bas";

static const char s1999[] = "/scene/monteWCommon/bas/mow_wait_yogore.bas";

static const char s2000[] = "/scene/monteWCommon/bas/mow_walk.bas";

static const char s2001[] = "/scene/monteWCommon/bas/mow_wet.bas";

static const char s2002[] = "/scene/monteWCommon/bas/mow_wet_A.bas";

static const char s2003[] = "/scene/monteWCommon/bas/mow_wet_yogore.bas";

static const char s2004[] = "/scene/monteWCommon/bas/mow_appear_c.bas";

static const char s2005[] = "/scene/monteWCommon/bas/mow_dance_c.bas";

static const char s2006[] = "/scene/monteWCommon/bas/mow_fall_c.bas";

static const char s2007[] = "/scene/monteWCommon/bas/mow_happy_c.bas";

static const char s2008[] = "/scene/monteWCommon/bas/mow_mad_c.bas";

static const char s2009[] = "/scene/monteWCommon/bas/mow_madloop_c.bas";

static const char s2010[] = "/scene/monteWCommon/bas/mow_run_c.bas";

static const char s2011[] = "/scene/monteWCommon/bas/mow_sitwet_c.bas";

static const char s2012[] = "/scene/monteWCommon/bas/mow_throw_c.bas";

static const char s2013[] = "/scene/monteWCommon/bas/mow_wait_arrow_c.bas";

static const char s2014[] = "/scene/monteWCommon/bas/mow_wait_yogore_c.bas";

static const char s2015[] = "/scene/monteWCommon/bas/mow_walk_c.bas";

static const char s2016[] = "/scene/monteWCommon/bas/mow_wet_c.bas";

static const char s2017[] = "/scene/monteWCommon/bas/mow_wet_A_c.bas";

static const char s2018[] = "/scene/monteWCommon/bas/mow_wet_yogore_c.bas";

static const char s2021[] = "/scene/monteWC/bas/mowC_mad.bas";

static const char s2022[] = "/scene/monteWC/bas/mowC_wet.bas";

static const char s2023[] = "/scene/monteWC/bas/mowC_wet_A.bas";

static const char s2026[] = "/scene/mareM/bas/mareM_appear.bas";

static const char s2027[] = "/scene/mareM/bas/mareM_dance.bas";

static const char s2028[] = "/scene/mareM/bas/mareM_fall.bas";

static const char s2029[] = "/scene/mareM/bas/mareM_happy.bas";

static const char s2030[] = "/scene/mareM/bas/mareM_jump.bas";

static const char s2031[] = "/scene/mareM/bas/mareM_recover.bas";

static const char s2032[] = "/scene/mareM/bas/mareM_run.bas";

static const char s2033[] = "/scene/mareM/bas/mareM_stand.bas";

static const char s2034[] = "/scene/mareM/bas/mareM_stand_A.bas";

static const char s2035[] = "/scene/mareM/bas/mareM_wait_yogore.bas";

static const char s2036[] = "/scene/mareM/bas/mareM_walk.bas";

static const char s2037[] = "/scene/mareM/bas/mareM_wash.bas";

static const char s2038[] = "/scene/mareM/bas/mareM_wet.bas";

static const char s2039[] = "/scene/mareM/bas/mareM_wet_A.bas";

static const char s2040[] = "/scene/mareM/bas/mareM_appear_c.bas";

static const char s2041[] = "/scene/mareM/bas/mareM_dance_c.bas";

static const char s2042[] = "/scene/mareM/bas/mareM_fall_c.bas";

static const char s2043[] = "/scene/mareM/bas/mareM_happy_c.bas";

static const char s2044[] = "/scene/mareM/bas/mareM_jump_c.bas";

static const char s2045[] = "/scene/mareM/bas/mareM_recover_c.bas";

static const char s2046[] = "/scene/mareM/bas/mareM_run_c.bas";

static const char s2047[] = "/scene/mareM/bas/mareM_stand_c.bas";

static const char s2048[] = "/scene/mareM/bas/mareM_stand_A_c.bas";

static const char s2049[] = "/scene/mareM/bas/mareM_walk_c.bas";

static const char s2050[] = "/scene/mareM/bas/mareM_wash_c.bas";

static const char s2051[] = "/scene/mareM/bas/mareM_wet_c.bas";

static const char s2052[] = "/scene/mareM/bas/mareM_wet_A_c.bas";

static const char s2057[] = "/scene/mareMB/bas/mareMB_wet.bas";

static const char s2059[] = "/scene/mareMC/bas/mareMC_stand.bas";

static const char s2060[] = "/scene/mareMC/bas/mareMC_stand_A.bas";

static const char s2061[] = "/scene/mareMC/bas/mareMC_wet.bas";

static const char s2062[] = "/scene/mareMC/bas/mareMC_wet_A.bas";

static const char s2065[] = "/scene/mareMD/bas/mareMD_hue.bas";

static const char s2066[] = "/scene/mareMD/bas/mareMD_wet.bas";

static const char s2069[] = "/scene/mareW/bas/mareW_appear.bas";

static const char s2070[] = "/scene/mareW/bas/mareW_dance.bas";

static const char s2071[] = "/scene/mareW/bas/mareW_fall.bas";

static const char s2072[] = "/scene/mareW/bas/mareW_happy.bas";

static const char s2073[] = "/scene/mareW/bas/mareW_recover.bas";

static const char s2074[] = "/scene/mareW/bas/mareW_run.bas";

static const char s2075[] = "/scene/mareW/bas/mareW_stand.bas";

static const char s2076[] = "/scene/mareW/bas/mareW_stand_A.bas";

static const char s2077[] = "/scene/mareW/bas/mareW_wait_yogore.bas";

static const char s2078[] = "/scene/mareW/bas/mareW_walk.bas";

static const char s2079[] = "/scene/mareW/bas/mareW_wash.bas";

static const char s2080[] = "/scene/mareW/bas/mareW_wet.bas";

static const char s2081[] = "/scene/mareW/bas/mareW_wet_A.bas";

static const char s2082[] = "/scene/mareW/bas/mareW_appear_c.bas";

static const char s2083[] = "/scene/mareW/bas/mareW_dance_c.bas";

static const char s2084[] = "/scene/mareW/bas/mareW_fall_c.bas";

static const char s2085[] = "/scene/mareW/bas/mareW_happy_c.bas";

static const char s2086[] = "/scene/mareW/bas/mareW_recover_c.bas";

static const char s2087[] = "/scene/mareW/bas/mareW_run_c.bas";

static const char s2088[] = "/scene/mareW/bas/mareW_stand_c.bas";

static const char s2089[] = "/scene/mareW/bas/mareW_stand_A_c.bas";

static const char s2090[] = "/scene/mareW/bas/mareW_walk_c.bas";

static const char s2091[] = "/scene/mareW/bas/mareW_wash_c.bas";

static const char s2092[] = "/scene/mareW/bas/mareW_wet_c.bas";

static const char s2093[] = "/scene/mareW/bas/mareW_wet_A_c.bas";

static const char s2098[] = "/scene/mareWB/bas/mareWBbaby_hold.bas";

static const char s2099[] = "/scene/mareWB/bas/mareWBbaby_holdTalk.bas";

static const char s2100[] = "/scene/mareWB/bas/mareWBbaby_holdWet.bas";

static const char s2101[] = "/scene/mareWB/bas/mareWB_holdTalk.bas";

static const char s2102[] = "/scene/mareWB/bas/mareWB_holdWet.bas";

static const char s2105[] = "/scene/kinopio/bas/kinopio_appear.bas";

static const char s2106[] = "/scene/kinopio/bas/kinopio_fall.bas";

static const char s2107[] = "/scene/kinopio/bas/kinopio_happy.bas";

static const char s2108[] = "/scene/kinopio/bas/kinopio_recover.bas";

static const char s2109[] = "/scene/kinopio/bas/kinopio_run.bas";

static const char s2110[] = "/scene/kinopio/bas/kinopio_sitshake.bas";

static const char s2111[] = "/scene/kinopio/bas/kinopio_sitwet.bas";

static const char s2112[] = "/scene/kinopio/bas/kinopio_stumble.bas";

static const char s2113[] = "/scene/kinopio/bas/kinopio_wait_yogore.bas";

static const char s2114[] = "/scene/kinopio/bas/kinopio_walk.bas";

static const char s2115[] = "/scene/kinopio/bas/kinopio_wet.bas";

static const char s2116[] = "/scene/kinopio/bas/kinopio_wet_A.bas";

static const char s2117[] = "/scene/kinopio/bas/kinopio_wet_yogore.bas";

static const char s2120[] = "/scene/kinojii/bas/kinoji_fall.bas";

static const char s2121[] = "/scene/kinojii/bas/kinoji_recover.bas";

static const char s2122[] = "/scene/kinojii/bas/kinoji_sitshake.bas";

static const char s2123[] = "/scene/kinojii/bas/kinoji_sitwet.bas";

static const char s2124[] = "/scene/kinojii/bas/kinoji_stumble.bas";

static const char s2125[] = "/scene/kinojii/bas/kinoji_walk.bas";

static const char s2126[] = "/scene/kinojii/bas/kinoji_wet.bas";

static const char s2127[] = "/scene/kinojii/bas/kinoji_wet_A.bas";

static const char s2130[] = "/scene/peach/bas/peach_anger_wait.bas";

static const char s2131[] = "/scene/peach/bas/peach_carry.bas";

static const char s2132[] = "/scene/peach/bas/peach_fear_wait.bas";

static const char s2133[] = "/scene/peach/bas/peach_parasol_wet.bas";

static const char s2134[] = "/scene/peach/bas/peach_recover.bas";

static const char s2135[] = "/scene/peach/bas/peach_tired_wait.bas";

static const char s2136[] = "/scene/peach/bas/peach_wet.bas";

static const char s2139[] = "/scene/raccoonDog/bas/tanuki_wait_A.bas";

static const char s2140[] = "/scene/raccoonDog/bas/tanuki_wait_B.bas";

static const char s2141[] = "/scene/raccoonDog/bas/tanuki_wet.bas";

static const char s2144[] = "/scene/sunflowerL/bas/sunflower_stand.bas";

static const char s2145[] = "/scene/sunflowerL/bas/sunflower_wet.bas";

static const char s2146[] = "/scene/sunflowerL/bas/sunflower_wet_B.bas";

static const char s2150[] = "/scene/sunflowerS/bas/sunflower_s_stand.bas";

static const char s2151[] = "/scene/sunflowerS/bas/sunflower_s_wet.bas";

static const char s2152[] = "/scene/sunflowerS/bas/sunflower_s_wet_B.bas";

static f32 s1431[] = { 1.0f, 1.0f, 1.0f };

static f32 s1411[] = { 1.0f, 1.0f, 1.0f };

static u32 s1210[] = { 0, 2, 1, 3 };

static const char* MtxCalcTypeName[] = {
	s1755,
	s1756,
	s1757,
	s1758,
};

static const char* monteMCommon_bastable[] = {
	s1938,
	s1939,
	s1940,
	s1941,
	s1942,
	s1943,
	s1944,
	s1945,
	0,
	0,
	s1946,
	0,
	0,
	s1947,
	0,
	0,
	0,
	0,
	s1948,
	s1949,
	s1950,
	s1951,
	s1952,
};

static const char* monteMCommon_bas_c_table[] = {
	s1953,
	s1954,
	s1955,
	s1956,
	s1957,
	s1958,
	s1959,
	s1960,
	0,
	0,
	s1961,
	0,
	0,
	s1962,
	0,
	0,
	0,
	0,
	s1963,
	s1964,
	s1965,
	s1966,
	s1967,
};

static const char* monteME_bastable[] = {
	s1970,
	0,
	0,
	s1971,
	0,
	0,
	s1972,
};

static const char* monteME_bas_c_table[] = {
	s1970,
	0,
	0,
	s1971,
	0,
	0,
	s1972,
};

static const char* monteMF_bastable[] = {
	s1975,
	s1976,
	s1977,
};

static const char* monteMF_bas_c_table[] = {
	s1975,
	s1976,
	s1977,
};

static const char* monteMH_bastable[] = {
	s1984,
	0,
	s1985,
	s1986,
};

static const char* monteMH_bas_c_table[] = {
	s1984,
	0,
	s1985,
	s1986,
};

static const char* monteWCommon_bastable[] = {
	s1989,
	s1990,
	s1991,
	s1992,
	s1993,
	s1994,
	0,
	s1995,
	0,
	0,
	s1996,
	0,
	0,
	s1997,
	0,
	0,
	s1998,
	0,
	0,
	s1999,
	s2000,
	s2001,
	s2002,
	s2003,
};

static const char* monteWCommon_bas_c_table[] = {
	s2004,
	s2005,
	s2006,
	s2007,
	s2008,
	s2009,
	0,
	s2010,
	0,
	0,
	s2011,
	0,
	0,
	s2012,
	0,
	0,
	s2013,
	0,
	0,
	s2014,
	s2015,
	s2016,
	s2017,
	s2018,
};

static const char* monteWC_bastable[] = {
	0,
	s2021,
	0,
	s2022,
	s2023,
};

static const char* monteWC_bas_c_table[] = {
	0,
	s2021,
	0,
	s2022,
	s2023,
};

static const char* mareM_bastable[] = {
	s2026,
	s2027,
	s2028,
	s2029,
	s2030,
	s2031,
	s2032,
	0,
	0,
	s2033,
	s2034,
	0,
	0,
	0,
	0,
	0,
	s2035,
	s2036,
	s2037,
	s2038,
	s2039,
};

static const char* mareM_bas_c_table[] = {
	s2040,
	s2041,
	s2042,
	s2043,
	s2044,
	s2045,
	s2046,
	0,
	0,
	s2047,
	s2048,
	0,
	0,
	0,
	0,
	0,
	s2035,
	s2049,
	s2050,
	s2051,
	s2052,
};

static const char* mareMA_bastable[] = {
	0,
	0,
	0,
	0,
	0,
};

static const char* mareMA_bas_c_table[] = {
	0,
	0,
	0,
	0,
	0,
};

static const char* mareMB_bastable[] = {
	0,
	0,
	0,
	0,
	0,
	s2057,
};

static const char* mareMB_bas_c_table[] = {
	0,
	0,
	0,
	0,
	0,
	s2057,
};

static const char* mareMC_bastable[] = {
	s2059,
	s2060,
	0,
	s2061,
	s2062,
};

static const char* mareMC_bas_c_table[] = {
	s2059,
	s2060,
	0,
	s2061,
	s2062,
};

static const char* mareW_bastable[] = {
	s2069,
	s2070,
	s2071,
	s2072,
	s2073,
	s2074,
	0,
	0,
	s2075,
	s2076,
	0,
	0,
	0,
	0,
	0,
	s2077,
	s2078,
	s2079,
	s2080,
	s2081,
};

static const char* mareW_bas_c_table[] = {
	s2082,
	s2083,
	s2084,
	s2085,
	s2086,
	s2087,
	0,
	0,
	s2088,
	s2089,
	0,
	0,
	0,
	0,
	0,
	s2077,
	s2090,
	s2091,
	s2092,
	s2093,
};

static const char* mareWA_bastable[] = {
	0,
	0,
	0,
	0,
};

static const char* mareWA_bas_c_table[] = {
	0,
	0,
	0,
	0,
};

static const char* mareWB_bastable[] = {
	s2098,
	s2099,
	s2100,
	0,
	s2101,
	s2102,
};

static const char* mareWB_bas_c_table[] = {
	s2098,
	s2099,
	s2100,
	0,
	s2101,
	s2102,
};

static const char* kinopio_bastable[] = {
	s2105,
	s2106,
	s2107,
	s2108,
	s2109,
	0,
	0,
	0,
	s2110,
	0,
	s2111,
	0,
	0,
	s2112,
	0,
	0,
	0,
	0,
	s2113,
	s2114,
	0,
	s2115,
	s2116,
	s2117,
	0,
};

static const char* kinopio_bas_c_table[] = {
	s2105,
	s2106,
	s2107,
	s2108,
	s2109,
	0,
	0,
	0,
	s2110,
	0,
	s2111,
	0,
	0,
	s2112,
	0,
	0,
	0,
	0,
	s2113,
	s2114,
	0,
	s2115,
	s2116,
	s2117,
	0,
};

static const char* kinojii_bastable[] = {
	s2120,
	s2121,
	0,
	0,
	s2122,
	0,
	s2123,
	0,
	0,
	s2124,
	0,
	0,
	0,
	s2125,
	0,
	s2126,
	s2127,
	0,
};

static const char* kinojii_bas_c_table[] = {
	s2120,
	s2121,
	0,
	0,
	s2122,
	0,
	s2123,
	0,
	0,
	s2124,
	0,
	0,
	0,
	s2125,
	0,
	s2126,
	s2127,
	0,
};

static const char* peach_bastable[] = {
	s2130,
	s2131,
	s2132,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	s2133,
	s2134,
	s2135,
	0,
	s2136,
};

static const char* peach_bas_c_table[] = {
	s2130,
	s2131,
	s2132,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	s2133,
	s2134,
	s2135,
	0,
	s2136,
};

static const char* raccoonDog_bastable[] = {
	s2139,
	s2140,
	0,
	s2141,
};

static const char* raccoonDog_bas_c_table[] = {
	s2139,
	s2140,
	0,
	s2141,
};

static const char* sunflowerL_bastable[] = {
	0,
	s2144,
	0,
	s2145,
	s2146,
};

static const char* sunflowerL_bas_c_table[] = {
	0,
	s2144,
	0,
	s2145,
	s2146,
};

static const char* sunflowerS_bastable[] = {
	0,
	s2150,
	0,
	s2151,
	s2152,
};

static const char* sunflowerS_bas_c_table[] = {
	0,
	s2150,
	0,
	s2151,
	s2152,
};

static const char* monteMG_bastable[] = {
	s1980,
	s1981,
};

static const char* mareMD_bastable[] = {
	s2065,
	s2066,
};

const char** TBaseNPC::getBasNameTable() const
{
	const char** table = 0;
	bool child = isChild();

	if (isNormalMonteM()) {
		if (child)
			table = monteMCommon_bas_c_table;
		else
			table = monteMCommon_bastable;
	} else if (isNormalMonteW()) {
		if (child)
			table = monteWCommon_bas_c_table;
		else
			table = monteWCommon_bastable;
	} else {
		switch (mActorType) {
		case 0x04000006:
			table = monteME_bastable;
			break;
		case 0x04000007:
			table = monteMF_bastable;
			break;
		case 0x04000008:
			table = monteMG_bastable;
			break;
		case 0x04000009:
			table = monteMH_bastable;
			break;
		case 0x0400000D:
			table = monteWC_bastable;
			break;
		case 0x0400000E:
			if (child)
				table = mareM_bas_c_table;
			else
				table = mareM_bastable;
			break;
		case 0x0400000F:
			table = mareMA_bastable;
			break;
		case 0x04000010:
			table = mareMB_bastable;
			break;
		case 0x04000011:
			table = mareMC_bastable;
			break;
		case 0x04000012:
			table = mareMD_bastable;
			break;
		case 0x04000013:
			if (child)
				table = mareW_bas_c_table;
			else
				table = mareW_bastable;
			break;
		case 0x04000014:
			table = mareWA_bastable;
			break;
		case 0x04000015:
			table = mareWB_bastable;
			break;
		case 0x04000016:
			table = kinopio_bastable;
			break;
		case 0x04000017:
			table = kinojii_bastable;
			break;
		case 0x04000018:
			table = peach_bastable;
			break;
		case 0x04000019:
			table = raccoonDog_bastable;
			break;
		case 0x0400001A:
			table = sunflowerL_bastable;
			break;
		case 0x0400001B:
			table = sunflowerS_bastable;
			break;
		case 0x0400001C:
		case 0x0400001D:
			break;
		}
	}

	return table;
}
