#ifndef APPS_GUARD
#define APPS_GUARD

// Only one of these can be uncommented at a time

// #define GIF_PLAYER
// #define MORPHING_CLOCK
// #define PONG_CLOCK
//  #define STOCK_TICKER
//  #define WEATHER_STATION

#define ATTRACT
// #define BUBBLES
// #define CODE_RAIN
// #define DVD_LOGO
// #define ELECTRIC_MANDALA
// #define FLOCK
// #define FLOW_FIELD
// #define HUE_VALUE_SPECTRUM
// #define INCREMENTAL_DRIFT
// #define JULIA_SET
// #define LIFE
// #define MAZE
// #define MUNCH
// #define PENDULUM_WAVE
// #define PERIODIC_TABLE
// #define PLASMA
// #define PONG_WARS
// #define SIMPLEX_NOISE
// #define SNAKE_GAME
// #define SNAKES
//  #define SWIRL
//  #define TV_TEST_PATTERN

// These are the apps that have app-specific config
#if defined(STOCK_TICKER) || defined(PONG_WARS) || defined(MORPHING_CLOCK) || defined(GIF_PLAYER) || defined(WEATHER_STATION)
#define APP_SPECIFIC_CONFIG
#endif

#endif