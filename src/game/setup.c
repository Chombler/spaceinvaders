#include <stdlib.h>
#include "game.h"

object* create_object(const Picture* sprite_0, const Picture* sprite_1, int type, int speed, int x, int y, int width, int height){
  object* obj = malloc(sizeof(object));
  *obj = (object){
    .sprite_0 = sprite_0,
    .sprite_1 = sprite_1,
    .curr_sprite = 0,
    .type = type,
    .width = width,
    .height = height,
    .speed = speed,
    .x = x,
    .y = y
  };
  return obj;
}

object* create_player(int x, int y){
  return create_object(&ship_1, NULL, PLAYER, PLAYER_SPEED, x, y, PLAYER_WIDTH, PLAYER_HEIGHT);
}

object* create_enemy(int x, int y, const Picture* sprite_0, const Picture* sprite_1){
  return create_object(sprite_0, sprite_1, ENEMY, ENEMY_SPEED, x, y, ENEMY_WIDTH, ENEMY_HEIGHT);
}

object* create_bullet(int x, int y, int speed, const Picture* sprite){
  return create_object(sprite, NULL, BULLET, speed, x, y, BULLET_WIDTH, BULLET_HEIGHT);
}

object* create_barricade(int x, int y){
  return create_object(&barricade, &destroyed_barricade, BARRICADE, 0, x, y, BARRICADE_WIDTH, BARRICADE_HEIGHT);
}

int is_within_object(int x, int y, object* obj){
  return(x >= obj->x && x <= obj->x + obj->width &&
         y >= obj->y && y <= obj->y + obj->height);
}

int is_within(object* a, object* b){
  if(is_within_object(a->x            , a->y            , b) ||
     is_within_object(a->x + a->width , a->y            , b) ||
     is_within_object(a->x            , a->y + a->height, b) ||
     is_within_object(a->x + a->width , a->y + a->height, b)){
       return COLLISION;
     }
  return NO_COLLISION;
}

int collision(object* a, object* b){
  if(is_within(a, b) || is_within(b, a)){
       return COLLISION;
     }
  return NO_COLLISION;
}

int is_oob_left(object* obj){
  return(obj->x < LEFT_BOUND);
}

int is_oob_right(object* obj){
  return(obj->x + obj->width > RIGHT_BOUND);
}

int is_oob_left_right(object* obj){
  return(is_oob_left(obj) || is_oob_right(obj));
}

int is_oob_up(object* obj){
  return(obj->y < UPPER_BOUND);
}

int is_oob_down(object* obj){
  return(obj->y + obj->height > LOWER_BOUND);
}

int is_oob_up_down(object* obj){
  return(is_oob_up(obj) || is_oob_down(obj));
}

void create_enemies(){
  for(int i = 0; i < NUM_ENEMIES; i++){
    object* obj;
    int x = ENEMY_X + ENEMY_WIDTH * (1 + i % ENEMIES_IN_ROW);
    int y = ENEMY_Y - ENEMY_HEIGHT * (i / ENEMIES_IN_ROW) - 5*state.level;

    if(i <  ENEMIES_IN_ROW * 2) obj = create_enemy(x, y, &crab_1, &crab_2);
    else if(i <  ENEMIES_IN_ROW * 4) obj = create_enemy(x, y, &ghost_1, &ghost_2);
    else obj = create_enemy(x, y, &squid_1, &squid_2);

    draw(obj);
    Node* node = malloc(sizeof(*node));
    node->obj = obj;
    node->score = 10 * (1 + (i+1)/(2*ENEMIES_IN_ROW));
    node->position = i;
    if(i == 0){
      state.enemy_root = node;
      state.curr_enemy = state.enemy_root;
    }
    else{
      state.curr_enemy->next = node;
      state.curr_enemy = state.curr_enemy->next;
      state.curr_enemy->next = NULL;
    }
  }
}

Node* create_barricade_group(int x, int y){
  Node* node;
  Node* root = malloc(sizeof(Node));
  root->obj = create_barricade(x, y);
  draw(root->obj);
  node = root;

  for(int i = 0; i < NUM_BARRICADES; i++){
    object* obj;
    int obj_x = x + BARRICADE_WIDTH * (i % BARRICADES_IN_ROW);
    int obj_y = y + BARRICADE_HEIGHT * (i / BARRICADES_IN_ROW);

    obj = create_barricade(obj_x, obj_y);

    draw(obj);
    Node* new_node = malloc(sizeof(Node));
    new_node->obj = obj;
    node->next = new_node;
    node = new_node;
    node->next = NULL;
  }
  return root;
}

int contains(int array[], int length, int num){
  for(int i = 0; i <length; i++){
    if(array[i] == num) return 1;
  }
  return 0;
}

void reset_objects(){
  if(state.player) free(state.player);
  if(state.player_bullet) free(state.player_bullet);
  state.player_bullet = NULL;
  while(state.enemy_root != NULL){
    Node* hold = state.enemy_root -> next;
    free(state.enemy_root->obj);
    free(state.enemy_root);
    state.enemy_root = hold;
  }
  for(int i = 0; i < 2; i++){
    if(state.enemy_bullets[i]) free(state.enemy_bullets[i]);
    state.enemy_bullets[i] = NULL;
  }
  while(state.far_left_barricade != NULL){
    Node* hold = state.far_left_barricade -> next;
    free(state.far_left_barricade->obj);
    free(state.far_left_barricade);
    state.far_left_barricade = hold;
  }
  while(state.left_barricade != NULL){
    Node* hold = state.left_barricade -> next;
    free(state.left_barricade->obj);
    free(state.left_barricade);
    state.left_barricade = hold;
  }
  while(state.right_barricade != NULL){
    Node* hold = state.right_barricade -> next;
    free(state.right_barricade->obj);
    free(state.right_barricade);
    state.right_barricade = hold;
  }
  while(state.far_right_barricade != NULL){
    Node* hold = state.far_right_barricade -> next;
    free(state.far_right_barricade->obj);
    free(state.far_right_barricade);
    state.far_right_barricade = hold;
  }

}

void generate_actors(){
  state.player = create_player(PLAYER_X, PLAYER_Y);
  draw_player(state.player);

  create_enemies();

  state.far_left_barricade  = create_barricade_group(FAR_LEFT_BARRICADE_X,  BARRICADE_Y);
  state.left_barricade      = create_barricade_group(LEFT_BARRICADE_X,      BARRICADE_Y);
  state.right_barricade     = create_barricade_group(RIGHT_BARRICADE_X,     BARRICADE_Y);
  state.far_right_barricade = create_barricade_group(FAR_RIGHT_BARRICADE_X, BARRICADE_Y);
}

void create_game(){
  reset_objects();
  state.lives = 3;
  state.score = 0;
  state.level = 0;

  LCD_DrawPicture(0,0,&background);
  LCD_DrawString(0,0, WHITE, WHITE, "Score: ", 16, 1); // clear background
  draw_score(state.score);
  draw_lives(state.lives);
  generate_actors();

  state.curr_enemy = state.enemy_root;
  state.enemies_reached_edge = false;
  state.enemy_dir = RIGHT;
  state.num_enemies_alive = NUM_ENEMIES;
  state.player_dir = 0;
  state.level = 1;
  state.enemy_delay = 0;
  for(int i = 0; i < ENEMIES_IN_ROW; i++){
    state.valid_shooters[i] = i;
  }
}

void next_level(){
  reset_objects();
  wait(200000000); // Wait 1 seconds

  LCD_DrawPicture(0,0,&background);
  LCD_DrawString(0,0, WHITE, WHITE, "Score: ", 16, 1); // clear background
  draw_score(state.score);
  draw_lives(state.lives);
  generate_actors();

  state.curr_enemy = state.enemy_root;
  state.enemies_reached_edge = false;
  state.enemy_dir = RIGHT;
  state.num_enemies_alive = NUM_ENEMIES;
  state.player_dir = 0;
  state.level += 1;
  state.enemy_delay = 0;
  state.respawn = 0;
  for(int i = 0; i < ENEMIES_IN_ROW; i++){
    state.valid_shooters[i] = i;
  }
}
