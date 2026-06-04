extern const float __two_to_log2e_m1_tI[];
extern const float __one_over_F[];

static const float __log2_F[] = {
	-0.375f, -0.36377275f, -0.352632195f, -0.341576993f, -0.330605894f,
	-0.319717556f, -0.308910817f, -0.298184395f, -0.287537158f, -0.276967913f,
	-0.266475528f, -0.256058931f, -0.245716989f, -0.235448644f, -0.225252882f,
	-0.21512866f, -0.205074996f, -0.195090905f, -0.185175434f, -0.175327659f,
	-0.165546641f, -0.155831486f, -0.146181315f, -0.136595264f,
	-0.127072483f, -0.117612153f, -0.108213462f, -0.0988755971f, -0.089597784f,
	-0.0803792477f, -0.0712192506f, -0.0621170439f, -0.0530719049f,
	-0.0440831222f, -0.0351499952f, -0.0262718461f, -0.017447995f,
	-0.00867778528f, 3.94313465e-05f, 0.00870429259f, 0.0173174236f,
	0.0258794371f, 0.0343909375f, 0.0428525135f, 0.0512647554f, 0.0596282259f,
	0.0679434985f, 0.0762111098f, 0.0844316185f, 0.0926055536f, 0.100733429f,
	0.108815774f, 0.116853096f, 0.124845885f, 0.132794634f, 0.140699834f,
	0.148561954f, 0.156381458f, 0.164158806f, 0.171894461f, 0.179588854f,
	0.187242419f, 0.194855601f, 0.202428833f, 0.209962502f, 0.217457041f,
	0.224912837f, 0.232330307f, 0.239709839f, 0.24705182f, 0.254356623f,
	0.261624634f, 0.268856198f, 0.2760517f, 0.28321147f, 0.290335923f,
	0.29742533f, 0.304480106f, 0.31150052f, 0.318486959f, 0.325439721f,
	0.332359135f, 0.339245528f, 0.346099198f, 0.352920443f, 0.35970962f,
	0.366466999f, 0.373192847f, 0.379887491f, 0.386551231f, 0.393184334f,
	0.399787068f, 0.406359702f, 0.412902564f, 0.419415861f, 0.425899893f,
	0.432354927f, 0.438781202f, 0.445178956f, 0.451548487f, 0.457890004f,
	0.464203775f, 0.470490038f, 0.476749033f, 0.482980996f, 0.489186138f,
	0.495364726f, 0.501516938f, 0.507643044f, 0.513743222f, 0.51981777f,
	0.525866807f, 0.531890571f, 0.537889361f, 0.543863237f, 0.549812496f,
	0.555737317f, 0.561637938f, 0.567514479f, 0.573367238f, 0.579196334f,
	0.585001945f, 0.590784311f, 0.59654355f, 0.602279902f, 0.607993603f,
	0.613684714f, 0.619353414f, 0.625f,
};

static const float __two_to_x[] = {
	0.693147182f, 0.240226507f, 0.0555041581f, 0.00961813424f,
	0.00133318256f, 0.00015401977f, 1.54832742e-05f, 1.33928177e-06f,
	1.02999984e-07f,
};

static const float __exp_to_x[] = {
	0.999999881f, 0.49999997f, 0.166667983f, 0.0416668877f, 0.00832859613f,
	0.001388276f, 0.000204699929f, 2.54991846e-05f,
};

static inline float make_float(unsigned long bits)
{
	union {
		unsigned long u;
		float f;
	} v;
	v.u = bits;
	return v.f;
}

static inline unsigned long float_bits(float value)
{
	union {
		float f;
		unsigned long u;
	} v;
	v.f = value;
	return v.u;
}

static inline float fabs_local(float x)
{
	unsigned long bits;

	bits = float_bits(x);
	bits &= 0x7fffffff;
	return make_float(bits);
}

static inline int is_nan_bits(unsigned long bits)
{
	return (bits & 0x7f800000) == 0x7f800000 && (bits & 0x007fffff) != 0;
}

static inline int is_inf_bits(unsigned long bits)
{
	return (bits & 0x7fffffff) == 0x7f800000;
}

static inline float log2f_approx(float x)
{
	unsigned long bits;
	unsigned long hiBits;
	unsigned long fullBits;
	int exponent;
	int index;
	float result;
	float delta;
	float delta2;
	float correction;

	bits     = float_bits(x);
	exponent = (int)(bits >> 23) - 128;
	index    = (int)((bits >> 16) & 0x7f);
	result   = 1.375f + (float)exponent + __log2_F[index];

	if ((bits & 0xffff) != 0) {
		hiBits   = (bits & 0x007f0000) | 0x3f800000;
		fullBits = (bits & 0x007fffff) | 0x3f800000;
		if ((bits & 0x8000) != 0) {
			++index;
			hiBits += 0x10000;
		}

		delta  = (make_float(fullBits) - make_float(hiBits)) * __one_over_F[index];
		delta2 = delta * delta;
		correction
		    = delta * make_float(0x3EF637A6) + make_float(0xBF38AA80);
		correction = delta2 * correction;
		correction = 0.03253879f * delta + correction;
		correction = 0.41015625f * delta + correction;
		correction = delta + correction;
		result += correction;
	}

	return result;
}

static inline float two_to_x(float x)
{
	int n;
	float frac;
	float scale;
	float poly;

	n    = (int)x;
	frac = x - (float)n;

	if (n > 128)
		return make_float(0x7f800000);

	if (n < -127)
		return 0.0f;

	scale = make_float((unsigned long)(n + 127) << 23);
	poly  = __two_to_x[8];
	poly  = frac * poly + __two_to_x[7];
	poly  = frac * poly + __two_to_x[6];
	poly  = frac * poly + __two_to_x[5];
	poly  = frac * poly + __two_to_x[4];
	poly  = frac * poly + __two_to_x[3];
	poly  = frac * poly + __two_to_x[2];
	poly  = frac * poly + __two_to_x[1];
	poly  = frac * poly + __two_to_x[0];
	poly  = 0.75f + (0.25f + frac * poly);

	return scale * poly;
}

float expf(float x)
{
	int n;
	float frac;
	int tableIndex;
	float scale;
	float poly;

	if (x > 88.72284f)
		return make_float(0x7f800000);

	if (x < -87.33655f)
		return 0.0f;

	n          = (int)x;
	frac       = x - (float)n;
	tableIndex = n + 88;

	scale = make_float((unsigned long)(n + 127) << 23);
	poly  = __exp_to_x[7];
	poly  = frac * poly + __exp_to_x[6];
	poly  = frac * poly + __exp_to_x[5];
	poly  = frac * poly + __exp_to_x[4];
	poly  = frac * poly + __exp_to_x[3];
	poly  = frac * poly + __exp_to_x[2];
	poly  = frac * poly + __exp_to_x[1];
	poly  = frac * poly + __exp_to_x[0];
	poly  = 0.9921875f + (0.007812501f + frac * poly);

	return __two_to_log2e_m1_tI[tableIndex] * (scale * poly);
}

float powf(float base, float exponent)
{
	unsigned long baseBits;
	unsigned long exponentBits;
	int exponentInt;
	float absBase;
	float result;

	baseBits     = float_bits(base);
	exponentBits = float_bits(exponent);

	if (base > 0.0f)
		return two_to_x(exponent * log2f_approx(base));

	if (base < 0.0f) {
		exponentInt = (int)exponent;
		if ((float)exponentInt != exponent)
			return make_float(0x7fffffff);

		result = two_to_x(exponent * log2f_approx(-base));
		if ((exponentInt & 1) != 0)
			result = -result;
		return result;
	}

	if (base == 0.0f) {
		exponentInt = (int)exponent;
		if (exponent > 0.0f) {
			if ((baseBits & 0x80000000) != 0 && (float)exponentInt == exponent
			    && (exponentInt & 1) != 0)
				return make_float(0x80000000);
			return 0.0f;
		}
		if ((baseBits & 0x80000000) != 0 && (float)exponentInt == exponent
		    && (exponentInt & 1) != 0)
			return make_float(0xff800000);
		return make_float(0x7f800000);
	}

	if (is_nan_bits(baseBits) || is_nan_bits(exponentBits))
		return make_float(0x7fffffff);

	if (exponent == 0.0f || base == 1.0f)
		return 1.0f;

	if (is_inf_bits(exponentBits)) {
		absBase = fabs_local(base);
		if (absBase == 1.0f)
			return 1.0f;
		if ((absBase > 1.0f && exponent > 0.0f)
		    || (absBase < 1.0f && exponent < 0.0f))
			return make_float(0x7f800000);
		return 0.0f;
	}

	return 0.0f;
}
