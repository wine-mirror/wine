// cos MPEG decoding tables
// output of:
// src/libmpg123/calctables cos

#if defined(RUNTIME_TABLES)

#ifdef REAL_IS_FLOAT

// aligned to 16 bytes for vector instructions, e.g. AltiVec

static ALIGNED(16) real cos64[16];
static ALIGNED(16) real cos32[8];
static ALIGNED(16) real cos16[4];
static ALIGNED(16) real cos8[2];
static ALIGNED(16) real cos4[1];

#endif

#else

#ifdef REAL_IS_FLOAT

// aligned to 16 bytes for vector instructions, e.g. AltiVec

static const ALIGNED(16) real cos64[16] = 
{
	 5.00602998e-01,  5.05470960e-01,  5.15447310e-01,  5.31042591e-01
,	 5.53103896e-01,  5.82934968e-01,  6.22504123e-01,  6.74808341e-01
,	 7.44536271e-01,  8.39349645e-01,  9.72568238e-01,  1.16943993e+00
,	 1.48416462e+00,  2.05778101e+00,  3.40760842e+00,  1.01900081e+01
};
static const ALIGNED(16) real cos32[8] = 
{
	 5.02419286e-01,  5.22498615e-01,  5.66944035e-01,  6.46821783e-01
,	 7.88154623e-01,  1.06067769e+00,  1.72244710e+00,  5.10114862e+00
};
static const ALIGNED(16) real cos16[4] = 
{
	 5.09795579e-01,  6.01344887e-01,  8.99976223e-01,  2.56291545e+00
};
static const ALIGNED(16) real cos8[2] = 
{
	 5.41196100e-01,  1.30656296e+00
};
static const ALIGNED(16) real cos4[1] = 
{
	 7.07106781e-01
};

#endif

#ifdef REAL_IS_FIXED

static const real cos64[16] = 
{
	    8398725,     8480395,     8647771,     8909416
,	    9279544,     9780026,    10443886,    11321405
,	   12491246,    14081950,    16316987,    19619946
,	   24900150,    34523836,    57170182,   170959967
};
static const real cos32[8] = 
{
	    8429197,     8766072,     9511743,    10851869
,	   13223040,    17795219,    28897867,    85583072
};
static const real cos16[4] = 
{
	    8552951,    10088893,    15099095,    42998586
};
static const real cos8[2] = 
{
	    9079764,    21920489
};
static const real cos4[1] = 
{
	   11863283
};

#endif

#endif
