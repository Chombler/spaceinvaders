#ifndef __GAME_H__
#define __GAME_H__
#include <stdlib.h>
#include <stdbool.h>
#include "lcd.h"

#define PLAYER_WIDTH 20
#define PLAYER_HEIGHT 20
#define PLAYER_SPEED 2
#define PLAYER_X 120
#define PLAYER_Y 260

#define NUM_ENEMIES 45
#define ENEMIES_IN_ROW 9
#define ENEMY_WIDTH 20
#define ENEMY_HEIGHT 20
#define ENEMY_SPEED 4
#define ENEMY_X LEFT_BOUND
#define ENEMY_Y 160
#define CRAB_POSITION 0
#define GHOST_POSITION 2*ENEMIES_IN_ROW
#define SQUID_POSITION 4*ENEMIES_IN_ROW

#define BULLET_WIDTH 2
#define BULLET_HEIGHT 10
#define BULLET_SPEED 2
#define CRAB_BULLET 0
#define GHOST_BULLET 1
#define SQUID_BULLET 2

#define NUM_BARRICADES 25
#define BARRICADES_IN_ROW 5
#define BARRICADE_WIDTH 5
#define BARRICADE_HEIGHT 5
#define BARRICADE_BLOCK_WIDTH BARRICADE_WIDTH*BARRICADES_IN_ROW
#define BARRICADE_BLOCK_HEIGHT BARRICADE_HEIGHT*NUM_BARRICADES/BARRICADES_IN_ROW
#define FAR_LEFT_BARRICADE_X 40
#define LEFT_BARRICADE_X 85
#define RIGHT_BARRICADE_X 130
#define FAR_RIGHT_BARRICADE_X 175
#define BARRICADE_Y 215


#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define LEFT_BOUND 10
#define RIGHT_BOUND 230
#define UPPER_BOUND 20
#define LOWER_BOUND 300

#define COLLISION 1
#define NO_COLLISION 0
#define LEFT -1
#define RIGHT 1

#define MAIN 0
#define START 1
#define IN_GAME 2
#define WIN 3
#define LOSE 4
#define WAITING 5

#define PLAYER 0
#define ENEMY  1
#define BULLET 2
#define BARRICADE 3

typedef struct _object {
  const Picture* sprite_0;
  const Picture* sprite_1;
  int curr_sprite;
  int type;
  int width;
  int height;
  int speed;
  int x;
  int y;
} object;

typedef struct _Node{
  object* obj;
  int position;
  int score;
  struct _Node *next;
}Node;

struct {
  object* player;
  object* player_bullet;
  Node* enemy_root;
  Node* curr_enemy;
  object* enemy_bullets[3];
  Node* far_left_barricade;
  Node* left_barricade;
  Node* right_barricade;
  Node* far_right_barricade;
  int valid_shooters[ENEMIES_IN_ROW];
  int enemies_reached_edge;
  int num_enemies_alive;
  int enemy_delay;
  int enemy_dir;
  int player_dir;
  int score;
  int lives;
  int level;
  int respawn;
  int respawn_timer;
} state;

extern const Picture background; // A 240x320 background image
extern const Picture destroyed_barricade;
extern const Picture barricade;

extern const Picture ship_1; // A 20x20 Galaga ship
extern const Picture crab_1; // A 20x20 enemy crab
extern const Picture ghost_1; // A 20x20 enemy crab
extern const Picture squid_1; // A 20x20 enemy squid
extern const Picture crab_2; // A 20x20 enemy crab
extern const Picture ghost_2; // A 20x20 enemy crab
extern const Picture squid_2; // A 20x20 enemy squid
extern const Picture ship_bullet; // A 20x20 enemy squid
extern const Picture crab_bullet; // A 20x20 enemy crab
extern const Picture ghost_bullet; // A 20x20 enemy crab
extern const Picture squid_bullet; // A 20x20 enemy squid

int stage;



object* create_object(const Picture* sprite_0, const Picture* sprite_1, int type, int speed, int x, int y, int width, int height);
int is_within_object(int x, int y, object* obj);
int collision(object* a, object* b);
int is_oob_left(object* obj);
int is_oob_right(object* obj);
int is_oob_left_right(object* obj);
int is_oob_up(object* obj);
int is_oob_down(object* obj);
int is_oob_up_down(object* obj);
bool empty(object* array[], int size);
bool full(object* array[], int size);

#endif
