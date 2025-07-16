// flap_index.h
#ifndef FLAP_INDEX_H
#define FLAP_INDEX_H

const int NUM_FLAPS = 57;
const char flapCharacters[NUM_FLAPS] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '.', ':', '-', '\'', '/', ',', '@', '\"', '#',
  '!', '&', '?', '~', '_', '+', '=', '(', ')', '$', '%', ' '

};

// NOTE: These step values are based on 57 total flaps and 4096 steps per full rotation.
// If you change NUM_FLAPS, regenerate this table to maintain alignment accuracy.
const int flapSteps[NUM_FLAPS] = {
     0,  72, 144, 216, 288, 360, 432, 504, 576, 648,
   720, 792, 864, 936,1008,1080,1152,1224,1296,1368,
  1440,1512,1584,1656,1728,1800,1872,1944,2016,2088,
  2160,2232,2304,2376,2448,2520,2592,2664,2736,2808,
  2880,2952,3024,3096,3168,3240,3312,3384,3456,3528,
  3600,3672,3744,3816,3888,3960,4032
};


#endif
