#ifndef TRIG_TABLES_H
#define TRIG_TABLES_H

#include <stdint.h>

// From 0..pi/2
static const uint16_t tan_table[256] = {
    0,
    201,
    402,
    603,
    804,
    1005,
    1207,
    1408,
    1609,
    1810,
    2011,
    2213,
    2414,
    2615,
    2817,
    3018,
    3220,
    3421,
    3623,
    3825,
    4026,
    4228,
    4430,
    4632,
    4834,
    5036,
    5239,
    5441,
    5644,
    5846,
    6049,
    6252,
    6455,
    6658,
    6861,
    7064,
    7268,
    7471,
    7675,
    7879,
    8083,
    8287,
    8492,
    8696,
    8901,
    9106,
    9311,
    9516,
    9721,
    9927,
    10133,
    10339,
    10545,
    10751,
    10958,
    11165,
    11372,
    11579,
    11786,
    11994,
    12202,
    12410,
    12618,
    12827,
    13036,
    13245,
    13454,
    13664,
    13874,
    14084,
    14295,
    14506,
    14717,
    14928,
    15140,
    15352,
    15564,
    15776,
    15989,
    16202,
    16416,
    16630,
    16844,
    17058,
    17273,
    17489,
    17704,
    17920,
    18136,
    18353,
    18570,
    18787,
    19005,
    19223,
    19442,
    19661,
    19880,
    20100,
    20320,
    20541,
    20762,
    20983,
    21205,
    21427,
    21650,
    21873,
    22097,
    22321,
    22546,
    22771,
    22997,
    23223,
    23449,
    23676,
    23904,
    24132,
    24360,
    24590,
    24819,
    25049,
    25280,
    25511,
    25743,
    25975,
    26208,
    26442,
    26676,
    26911,
    27146,
    27382,
    27618,
    27855,
    28093,
    28331,
    28570,
    28810,
    29050,
    29291,
    29533,
    29775,
    30018,
    30261,
    30506,
    30751,
    30996,
    31243,
    31490,
    31738,
    31986,
    32236,
    32486,
    32736,
    32988,
    33240,
    33494,
    33748,
    34002,
    34258,
    34514,
    34772,
    35030,
    35289,
    35548,
    35809,
    36071,
    36333,
    36596,
    36861,
    37126,
    37392,
    37659,
    37927,
    38196,
    38465,
    38736,
    39008,
    39281,
    39555,
    39829,
    40105,
    40382,
    40660,
    40939,
    41219,
    41500,
    41782,
    42066,
    42350,
    42636,
    42923,
    43210,
    43500,
    43790,
    44081,
    44374,
    44668,
    44963,
    45259,
    45557,
    45856,
    46156,
    46457,
    46760,
    47064,
    47369,
    47676,
    47984,
    48294,
    48605,
    48917,
    49231,
    49546,
    49863,
    50181,
    50501,
    50822,
    51145,
    51469,
    51795,
    52122,
    52451,
    52782,
    53114,
    53448,
    53784,
    54121,
    54460,
    54801,
    55144,
    55488,
    55834,
    56182,
    56532,
    56883,
    57237,
    57592,
    57950,
    58309,
    58670,
    59033,
    59398,
    59766,
    60135,
    60506,
    60880,
    61255,
    61633,
    62013,
    62395,
    62780,
    63167,
    63556,
    63947,
    64341,
    64737,
    65135
};


// From 0..pi/2
static const uint16_t sin_table[256] = {
    0,
    402,
    804,
    1206,
    1608,
    2010,
    2412,
    2814,
    3216,
    3617,
    4019,
    4420,
    4821,
    5222,
    5623,
    6023,
    6424,
    6824,
    7224,
    7623,
    8022,
    8421,
    8820,
    9218,
    9616,
    10014,
    10411,
    10808,
    11204,
    11600,
    11996,
    12391,
    12785,
    13180,
    13573,
    13966,
    14359,
    14751,
    15143,
    15534,
    15924,
    16314,
    16703,
    17091,
    17479,
    17867,
    18253,
    18639,
    19024,
    19409,
    19792,
    20175,
    20557,
    20939,
    21320,
    21699,
    22078,
    22457,
    22834,
    23210,
    23586,
    23961,
    24335,
    24708,
    25080,
    25451,
    25821,
    26190,
    26558,
    26925,
    27291,
    27656,
    28020,
    28383,
    28745,
    29106,
    29466,
    29824,
    30182,
    30538,
    30893,
    31248,
    31600,
    31952,
    32303,
    32652,
    33000,
    33347,
    33692,
    34037,
    34380,
    34721,
    35062,
    35401,
    35738,
    36075,
    36410,
    36744,
    37076,
    37407,
    37736,
    38064,
    38391,
    38716,
    39040,
    39362,
    39683,
    40002,
    40320,
    40636,
    40951,
    41264,
    41576,
    41886,
    42194,
    42501,
    42806,
    43110,
    43412,
    43713,
    44011,
    44308,
    44604,
    44898,
    45190,
    45480,
    45769,
    46056,
    46341,
    46624,
    46906,
    47186,
    47464,
    47741,
    48015,
    48288,
    48559,
    48828,
    49095,
    49361,
    49624,
    49886,
    50146,
    50404,
    50660,
    50914,
    51166,
    51417,
    51665,
    51911,
    52156,
    52398,
    52639,
    52878,
    53114,
    53349,
    53581,
    53812,
    54040,
    54267,
    54491,
    54714,
    54934,
    55152,
    55368,
    55582,
    55794,
    56004,
    56212,
    56418,
    56621,
    56823,
    57022,
    57219,
    57414,
    57607,
    57798,
    57986,
    58172,
    58356,
    58538,
    58718,
    58896,
    59071,
    59244,
    59415,
    59583,
    59750,
    59914,
    60075,
    60235,
    60392,
    60547,
    60700,
    60851,
    60999,
    61145,
    61288,
    61429,
    61568,
    61705,
    61839,
    61971,
    62101,
    62228,
    62353,
    62476,
    62596,
    62714,
    62830,
    62943,
    63054,
    63162,
    63268,
    63372,
    63473,
    63572,
    63668,
    63763,
    63854,
    63944,
    64031,
    64115,
    64197,
    64277,
    64354,
    64429,
    64501,
    64571,
    64639,
    64704,
    64766,
    64827,
    64884,
    64940,
    64993,
    65043,
    65091,
    65137,
    65180,
    65220,
    65259,
    65294,
    65328,
    65358,
    65387,
    65413,
    65436,
    65457,
    65476,
    65492,
    65505,
    65516,
    65525,
    65531,
    65535
};

#endif  // TRIG_TABLES_H
