/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include <accelerometer.h>
#include "stm32f0xx.h"
#include <stdint.h>
#include "game.h"
#include "midi.h"
#include "midiplay.h"
#include "lcd.h"
#include "music.h"

//#include "step.c"
extern uint8_t midifile[];

object* create_bullet();
void draw(object* obj);
void erase(object* obj);
void draw_main();
void draw_win();
void draw_lose();
void draw_score(int score);
void start_tim3();
void stop_tim3();
void setup_io();
void setup_timers();

void fire_enemy_bullet(object* enemy, const Picture* sprite);
int is_within(object* a, object* b);
void check_enemy_and_barricade(object** enemy, Node** barricade_root);

object far_left_barricade = (object){
  .width = BARRICADE_BLOCK_WIDTH,
  .height = BARRICADE_BLOCK_HEIGHT,
  .x = FAR_LEFT_BARRICADE_X,
  .y = BARRICADE_Y
};

object left_barricade = (object){
  .width = BARRICADE_BLOCK_WIDTH,
  .height = BARRICADE_BLOCK_HEIGHT,
  .x = LEFT_BARRICADE_X,
  .y = BARRICADE_Y
};

object right_barricade = (object){
  .width = BARRICADE_BLOCK_WIDTH,
  .height = BARRICADE_BLOCK_HEIGHT,
  .x = RIGHT_BARRICADE_X,
  .y = BARRICADE_Y
};

object far_right_barricade = (object){
  .width = BARRICADE_BLOCK_WIDTH,
  .height = BARRICADE_BLOCK_HEIGHT,
  .x = FAR_RIGHT_BARRICADE_X,
  .y = BARRICADE_Y
};

void move_enemies(){
  if(state.enemy_delay > 0){
    state.enemy_delay -= 1;
    return;
  }
  if(state.enemy_root == NULL) return;
  if(state.curr_enemy == NULL) state.curr_enemy = state.enemy_root;
  if(state.curr_enemy == state.enemy_root){
    if(state.enemies_reached_edge) state.enemies_reached_edge = false;
    for(Node* node = state.enemy_root; !state.enemies_reached_edge && node != NULL; node = node->next){
      if((state.enemy_dir == LEFT && is_oob_left(node->obj) ) ||
              (state.enemy_dir == RIGHT && is_oob_right(node->obj)) ){
        state.enemies_reached_edge = true;
        state.enemy_dir *= -1;
      }
    }
  }
  if(!state.enemy_bullets[CRAB_BULLET] && state.curr_enemy->position <= GHOST_POSITION &&
      contains(state.valid_shooters, ENEMIES_IN_ROW, state.curr_enemy->position)){
    fire_enemy_bullet(state.curr_enemy->obj, &crab_bullet);
  }
  else if(!state.enemy_bullets[GHOST_BULLET] && state.curr_enemy->position > GHOST_POSITION &&
      state.curr_enemy->position <= SQUID_POSITION && contains(state.valid_shooters, ENEMIES_IN_ROW, state.curr_enemy->position)){
    fire_enemy_bullet(state.curr_enemy->obj, &ghost_bullet);
  }
  else if(!state.enemy_bullets[SQUID_BULLET] && state.curr_enemy->position > SQUID_POSITION &&
      contains(state.valid_shooters, ENEMIES_IN_ROW, state.curr_enemy->position)){
    fire_enemy_bullet(state.curr_enemy->obj, &squid_bullet);
  }
  if(state.enemies_reached_edge){
    erase(state.curr_enemy->obj);
    state.curr_enemy->obj -> y += ENEMY_HEIGHT/2;
  }
  else{
    erase(state.curr_enemy->obj);
    state.curr_enemy->obj -> x += state.curr_enemy->obj -> speed * state.enemy_dir;
  }

  if(state.far_left_barricade && is_within((state.curr_enemy->obj), &far_left_barricade) ){
    check_enemy_and_barricade(&(state.curr_enemy->obj), &state.far_left_barricade);
  }
  else if(state.left_barricade && is_within((state.curr_enemy->obj), &left_barricade) ){
    check_enemy_and_barricade(&(state.curr_enemy->obj), &state.left_barricade);
  }
  else if(state.right_barricade && is_within((state.curr_enemy->obj), &right_barricade) ){
    check_enemy_and_barricade(&(state.curr_enemy->obj), &state.right_barricade);
  }
  else if(state.far_right_barricade && is_within((state.curr_enemy->obj), &far_right_barricade) ){
    check_enemy_and_barricade(&(state.curr_enemy->obj), &state.far_right_barricade);
  }

  draw(state.curr_enemy->obj);
  state.curr_enemy = state.curr_enemy->next ? state.curr_enemy->next : state.enemy_root;
}

void move_player(){
  if(!state.player) return;
  if(state.player_dir == 0) return;
  state.player->x += state.player->speed * state.player_dir;
  if(is_oob_left(state.player)){
    state.player->x = LEFT_BOUND;
  }
  else if(is_oob_right(state.player)){
    state.player->x = RIGHT_BOUND - state.player->width;
  }
  draw_player(state.player);
}

void move_bullets(){
  //Move Player Bullets
  if(state.player_bullet){
    erase(state.player_bullet);
    state.player_bullet->y += state.player_bullet->speed;
    draw(state.player_bullet);
    if(is_oob_up(state.player_bullet)){
      erase(state.player_bullet);
      free(state.player_bullet);
      state.player_bullet = NULL;
    }
  }

  //Move Enemy Bullets
  for(int i = 0; i < 2; i++){
    if(state.enemy_bullets[i]){
      erase(state.enemy_bullets[i]);
      state.enemy_bullets[i]->y += state.enemy_bullets[i]->speed;
      draw(state.enemy_bullets[i]);
      if(is_oob_down(state.enemy_bullets[i])){
        erase(state.enemy_bullets[i]);
        free(state.enemy_bullets[i]);
        state.enemy_bullets[i] = NULL;
      }
    }
  }
}

int check_player_bullet_and_enemies(){
  int pos = -1;
  if(state.player_bullet && collision(state.player_bullet, state.enemy_root->obj)){
    erase(state.player_bullet);
    free(state.player_bullet);
    state.player_bullet = NULL;

    pos = state.enemy_root->position;
    Node* hold = state.enemy_root->next;
    erase(state.enemy_root->obj);
    state.score += state.enemy_root->score;
    if(state.curr_enemy == state.enemy_root){
      state.curr_enemy = state.enemy_root->next;
    }
    free(state.enemy_root->obj);
    free(state.enemy_root);
    state.enemy_root = hold;
    state.num_enemies_alive -= 1;
    draw_score(state.score);
    state.enemy_delay = 15;
  }
  for(Node* node = state.enemy_root; state.player_bullet && node->next != NULL; node = node->next){
    if(collision(state.player_bullet, node->next->obj)){
      erase(state.player_bullet);
      free(state.player_bullet);
      state.player_bullet = NULL;

      pos = node->next->position;
      Node* new_next = node->next->next;
      Node* to_remove = node->next;
      erase(node->next->obj);
      state.score += node->score;
      if(to_remove == state.curr_enemy){
        state.curr_enemy = new_next;
      }
      node->next = new_next;
      free(to_remove->obj);
      free(to_remove);
      state.num_enemies_alive -= 1;
      draw_score(state.score);
      state.enemy_delay = 15;
    }
  }
  return pos;
}

void check_bullet_and_barricade(object** bullet, Node** barricade_root){
  if(*bullet && collision(*bullet, (*barricade_root)->obj)){
    erase(*bullet);
    free(*bullet);
    *bullet = NULL;

    Node* hold = (*barricade_root)->next;
    erase((*barricade_root)->obj);
    free((*barricade_root)->obj);
    free(*barricade_root);
    *barricade_root = hold;
  }
  for(Node* node = *barricade_root; *bullet && node->next != NULL; node = node->next){
    if(collision(*bullet, node->next->obj)){
      erase(*bullet);
      free(*bullet);
      *bullet = NULL;

      Node* new_next = node->next->next;
      Node* to_remove = node->next;
      erase(node->next->obj);
      node->next = new_next;
      free(to_remove->obj);
      free(to_remove);
    }
  }
}

void check_enemy_and_barricade(object** enemy, Node** barricade_root){
  if(*enemy && collision(*enemy, (*barricade_root)->obj)){
    Node* hold = (*barricade_root)->next;
    erase((*barricade_root)->obj);
    free((*barricade_root)->obj);
    free(*barricade_root);
    *barricade_root = hold;
  }
  for(Node* node = *barricade_root; *enemy && node->next != NULL; node = node->next){
    if(collision(*enemy, node->next->obj)){
      Node* new_next = node->next->next;
      Node* to_remove = node->next;
      erase(node->next->obj);
      node->next = new_next;
      free(to_remove->obj);
      free(to_remove);
    }
  }
}

void check_collisions(void){
  //Check Player Bullet
  int pos = check_player_bullet_and_enemies();
  if(pos != -1){
    for(int i = 0; i < ENEMIES_IN_ROW; i++){
      if(state.valid_shooters[i] == pos){
        state.valid_shooters[i] += ENEMIES_IN_ROW;
      }
    }
  }

  if(state.player_bullet && state.far_left_barricade && is_within(state.player_bullet, &far_left_barricade) ){
    check_bullet_and_barricade(&state.player_bullet, &state.far_left_barricade);
  }
  else if(state.player_bullet && state.left_barricade && is_within(state.player_bullet, &left_barricade) ){
    check_bullet_and_barricade(&state.player_bullet, &state.left_barricade);
  }
  else if(state.player_bullet && state.right_barricade && is_within(state.player_bullet, &right_barricade) ){
    check_bullet_and_barricade(&state.player_bullet, &state.right_barricade);
  }
  else if(state.player_bullet && state.far_right_barricade && is_within(state.player_bullet, &far_right_barricade) ){
    check_bullet_and_barricade(&state.player_bullet, &state.far_right_barricade);
  }

  //Check Enemy Bullets
  for(int i = 0; i < 2; i++){
    if(state.enemy_bullets[i] && collision(state.enemy_bullets[i], state.player)){
      erase(state.player);
      erase(state.enemy_bullets[i]);
      free(state.player);
      free(state.enemy_bullets[i]);
      state.player = NULL;
      state.enemy_bullets[i] = NULL;
      state.lives -= 1;
    }
    if(state.enemy_bullets[i] && state.far_left_barricade && is_within(state.enemy_bullets[i], &far_left_barricade) ){
      check_bullet_and_barricade(&state.enemy_bullets[i], &state.far_left_barricade);
    }
    else if(state.enemy_bullets[i] && state.left_barricade && is_within(state.enemy_bullets[i], &left_barricade) ){
      check_bullet_and_barricade(&state.enemy_bullets[i], &state.left_barricade);
    }
    else if(state.enemy_bullets[i] && state.right_barricade && is_within(state.enemy_bullets[i], &right_barricade) ){
      check_bullet_and_barricade(&state.enemy_bullets[i], &state.right_barricade);
    }
    else if(state.enemy_bullets[i] && state.far_right_barricade && is_within(state.enemy_bullets[i], &far_right_barricade) ){
      check_bullet_and_barricade(&state.enemy_bullets[i], &state.far_right_barricade);
    }
  }
}

void fire_player_bullet(void){
  if(state.player_bullet) return;
  state.player_bullet = create_bullet(state.player->x + (PLAYER_WIDTH - BULLET_WIDTH)/2,
      state.player->y - BULLET_HEIGHT, BULLET_SPEED * -2, &ship_bullet);
}

void fire_enemy_bullet(object* enemy, const Picture* sprite){
  for(int i = 0; i < 2; i++){
    if(!state.enemy_bullets[i]){
      state.enemy_bullets[i] = create_bullet(enemy->x + (ENEMY_WIDTH - BULLET_WIDTH)/2,
          enemy->y + ENEMY_HEIGHT, BULLET_SPEED, sprite);
      return;
    }
  }
}

void wait(long n){
  for(long i = n; i >= 0; i-=83);
}

void check_game_over(void){
  if(state.player == NULL && state.respawn == 0){
    if(state.lives <= 0){
      stop_tim3();
      stage = LOSE;
      draw_lose();
      draw_lives(0);
    }
    else{
      state.enemy_delay = 180;
      state.respawn = 1;
      state.respawn_timer = 120;
    }
  }
  else if(state.num_enemies_alive <= 0){
    stop_tim3();
    stage = WIN;
  }
  else{
    if(state.enemy_root){
      if(state.enemy_root->obj->y >= PLAYER_Y){
        stop_tim3();
        stage = LOSE;
        draw_lose();
      }
      else{
        return;
      }
    }
  }
}


void game_loop(void){
  state.player_dir = read_tilt();
  move_enemies();
  move_player();
  move_bullets();
  check_collisions();
  check_game_over();
  if(state.respawn){
    if(state.respawn_timer > 0){
      state.respawn_timer -= 1;
    }
    else{
      state.player = create_player(PLAYER_X, PLAYER_Y);
      draw_player(state.player);
      draw_lives(state.lives);
      state.respawn = 0;
    }
  }
}


int main(void)
{

  stage = MAIN;
  MIDI_Player *mp = midi_init(midifile);
  init_wavetable_hybrid2();
  setup_timers();
  setup_io();
  LCD_DrawPicture(0, 0, &background);
  draw_main();
  for(;;){
    if (mp->nexttick >= MAXTICKS)
        mp = midi_init(midifile);
  }
    //move_ship();
}


