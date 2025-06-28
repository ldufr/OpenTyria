#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "raylib.h"

#define STB_DS_IMPLEMENTATION
#include "../code/stb_ds.h"

#include "../code/errors.h"
#include "../code/slice.h"
#include "../code/array.h"
#include "../code/endian.h"

#include "../code/array.c"

#include "../code/GmDefs.h"
#include "../code/GmMath.h"
#include "../code/GmPathHeap.h"
#include "../code/GmPaths.h"

#include "../code/GmPaths.c"
#include "../code/GmPathHeap.c"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))
#endif

#include "reader.c"
#include "compress.c"
#include "cmdline.c"

#include "riff.c"
#include "dat.c"
#include "anff.c"

#include "PathDataImport.c"
#include "MapParams.c"
#include "PathFinding.c"

#ifndef BUILD_TESTS
#include "main.c"
#else
#include "tests.c"
#endif
