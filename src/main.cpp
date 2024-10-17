#include "Arduino.h"
#include "../lib/luxigrid.h"

extern void setup();
extern void loop();

#ifdef GIF_PLAYER
#include "../apps/gif-player.hpp"
#endif

#ifdef MORPHING_CLOCK
#include "../apps/morphing-clock.hpp"
#endif

#ifdef PONG_CLOCK
#include "../apps/pong-clock.hpp"
#endif

#ifdef STOCK_TICKER
#include "../apps/stock-ticker.hpp"
#endif

#ifdef WEATHER_STATION
#include "../apps/weather-station.hpp"
#endif

//
// Animations
//
#ifdef ATTRACT
#include "../apps/animations/attract.hpp"
#endif

#ifdef BUBBLES
#include "../apps/animations/bubbles.hpp"
#endif

#ifdef CODE_RAIN
#include "../apps/animations/code-rain.hpp"
#endif

#ifdef DVD_LOGO
#include "../apps/animations/dvd-logo.hpp"
#endif

#ifdef ELECTRIC_MANDALA
#include "../apps/animations/electric-mandala.hpp"
#endif

#ifdef FLOCK
#include "../apps/animations/flock.hpp"
#endif

#ifdef FLOW_FIELD
#include "../apps/animations/flow-field.hpp"
#endif

#ifdef HUE_VALUE_SPECTRUM
#include "../apps/animations/hue-value-spectrum.hpp"
#endif

#ifdef INCREMENTAL_DRIFT
#include "../apps/animations/incremental-drift.hpp"
#endif

#ifdef JULIA_SET
#include "../apps/animations/julia-set.hpp"
#endif

#ifdef LIFE
#include "../apps/animations/life.hpp"
#endif

#ifdef MAZE
#include "../apps/animations/maze.hpp"
#endif

#ifdef MUNCH
#include "../apps/animations/munch.hpp"
#endif

#ifdef PENDULUM_WAVE
#include "../apps/animations/pendulum-wave.hpp"
#endif

#ifdef PERIODIC_TABLE
#include "../apps/animations/periodic-table.hpp"
#endif

#ifdef PLASMA
#include "../apps/animations/plasma.hpp"
#endif

#ifdef PONG_WARS
#include "../apps/animations/pong-wars.hpp"
#endif

#ifdef SIMPLEX_NOISE
#include "../apps/animations/simplex-noise.hpp"
#endif

#ifdef SNAKE_GAME
#include "../apps/animations/snake-game.hpp"
#endif

#ifdef SNAKES
#include "../apps/animations/snakes.hpp"
#endif

#ifdef SWIRL
#include "../apps/animations/swirl.hpp"
#endif

#ifdef TV_TEST_PATTERN
#include "../apps/animations/tv-test-pattern.hpp"
#endif