/**
 * Copyright: 2018, Ableton AG, Berlin. All rights reserved.
 */

#ifndef UTIL_H_
#define UTIL_H_

// useful defines

// suppress "unused variable" compiler warning
#define UNUSED(x) (void)x;

// useful special unsigned value distinguishable from zero
#define NONE 0xFFFFFFFF

// use as K(Something) for true and !K(Something) for false
#define K(a) 1

#endif /* UTIL_H_ */
