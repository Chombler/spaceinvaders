#include "stm32f0xx.h"
#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "midi.h"
#include "midiplay.h"
#include "game.h"

static void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

// Copy a subset of a large source picture into a smaller destination.
// sx,sy are the offset into the source picture.
void pic_subset(Picture *dst, const Picture *src, int sx, int sy)
{
    int dw = dst->width;
    int dh = dst->height;
    for(int y=0; y<dh; y++) {
        if (y+sy < 0)
            continue;
        if (y+sy >= src->height)
            break;
        for(int x=0; x<dw; x++) {
            if (x+sx < 0)
                continue;
            if (x+sx >= src->width)
                break;
            dst->pix2[dw * y + x] = src->pix2[src->width * (y+sy) + x + sx];
        }
    }
}

// Overlay a picture onto a destination picture.
// xoffset,yoffset are the offset into the destination picture that the
// source picture is placed.
// Any pixel in the source that is the 'transparent' color will not be
// copied.  This defines a color in the source that can be used as a
// transparent color.
void pic_overlay(Picture *dst, int xoffset, int yoffset, const Picture *src, int transparent)
{
    for(int y=0; y<src->height; y++) {
        int dy = y+yoffset;
        if (dy < 0)
            continue;
        if (dy >= dst->height)
            break;
        for(int x=0; x<src->width; x++) {
            int dx = x+xoffset;
            if (dx < 0)
                continue;
            if (dx >= dst->width)
                break;
            unsigned short int p = src->pix2[y*src->width + x];
            if (p != transparent)
                dst->pix2[dy*dst->width + dx] = p;
        }
    }
}

extern const Picture background; // A 240x320 background image
extern const Picture ship; // A 20x20 Galaga ship

// This C macro will create an array of Picture elements.
// Really, you'll just use it as a pointer to a single Picture
// element with an internal pix2[] array large enough to hold
// an image of the specified size.
// BE CAREFUL HOW LARGE OF A PICTURE YOU TRY TO CREATE:
// A 100x100 picture uses 20000 bytes.  You have 32768 bytes of SRAM.
#define TempPicturePtr(name,width,height) Picture name[(width)*(height)/6+2] = { {width,height,2} }

void erase(object* obj)
{
  if(obj->type == ENEMY){
    TempPicturePtr(tmp, ENEMY_WIDTH, ENEMY_HEIGHT); // Create a temporary image.
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    obj->curr_sprite = 1 - obj->curr_sprite;
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
  else if(obj->type == BULLET){
    TempPicturePtr(tmp, BULLET_WIDTH, BULLET_HEIGHT);
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
  else if(obj->type == BARRICADE){
    TempPicturePtr(tmp, BARRICADE_WIDTH, BARRICADE_HEIGHT); // Create a temporary image.
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
  else{
    TempPicturePtr(tmp, PLAYER_WIDTH, PLAYER_HEIGHT); // Create a temporary image.
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
}

void draw(object* obj)
{
  if(obj->type == ENEMY){
    TempPicturePtr(tmp, ENEMY_WIDTH, ENEMY_HEIGHT); // Create a temporary image.
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    if(obj->curr_sprite == 0){
      pic_overlay(tmp, 0, 0, obj->sprite_0, 0xffff); // Overlay the sprite
    }
    else{
      pic_overlay(tmp, 0, 0, obj->sprite_1, 0xffff); // Overlay the sprite
    }
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
  else if(obj->type == BARRICADE){
    TempPicturePtr(tmp, BARRICADE_WIDTH, BARRICADE_HEIGHT); // Create a temporary image.
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    if(obj->curr_sprite == 0){
      pic_overlay(tmp, 0, 0, obj->sprite_0, 0xffff); // Overlay the sprite
      obj->curr_sprite = 1;
    }
    else{
      pic_overlay(tmp, 0, 0, obj->sprite_1, 0xffff); // Overlay the sprite
      obj->curr_sprite = 0;
    }
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
  else{
    TempPicturePtr(tmp, BULLET_WIDTH, BULLET_HEIGHT);
    pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
    pic_overlay(tmp, 0, 0, obj->sprite_0, 0xffff); // Overlay the sprite
    LCD_DrawPicture(obj->x,obj->y, tmp); // Draw
  }
}

void draw_main(){
  LCD_DrawString(15,40, WHITE, BLACK, "S P A C E   I N V A D E R S", 16, 0); // clear background
  LCD_DrawString(40,141, WHITE, BLACK, "Press SHOOT to start!", 16, 1); // clear background
}


void draw_lose(){
  LCD_DrawString(48,30, WHITE, BLACK, "G A M E    O V E R", 16, 0); // clear background
  LCD_DrawString(20,50, WHITE, BLACK, "Press SHOOT to try again!", 16, 1); // clear background
}

void draw_player(object* obj){
  TempPicturePtr(tmp, 2*PLAYER_WIDTH, PLAYER_HEIGHT);
  pic_subset(tmp, &background, obj->x, obj->y); // Copy the background
  pic_overlay(tmp, PLAYER_WIDTH/2, 0, obj->sprite_0, 0xffff); // Overlay the ship
  LCD_DrawPicture(obj->x - PLAYER_WIDTH/2,obj->y, tmp); // Draw
}

void draw_score(int score){
  TempPicturePtr(tmp, 60, 16);
  pic_subset(tmp, &background, 56, 0); // Copy the background
  LCD_DrawPicture(56,0, tmp); // Draw

  char* str = (char*)malloc(5* sizeof(char));
  sprintf(str, "%04d", score);
  LCD_DrawString(56,0, WHITE, BLACK, str, 16, 1); // clear background
  free(str);
}

void draw_lives(int lives){
  TempPicturePtr(tmp, 60, 20);
  pic_subset(tmp, &background, 0, 300); // Copy the background
  LCD_DrawPicture(0,300, tmp); // Draw

  char* str = (char*)malloc(2* sizeof(char));
  sprintf(str, "%d", lives);
  LCD_DrawString(0, 300, WHITE, BLACK, str, 16, 1);
  free(str);
  for(int i = 1; i < lives; i++){
    TempPicturePtr(tmp, PLAYER_WIDTH, PLAYER_HEIGHT);
    pic_subset(tmp, &background, 20*i - 10, 300); // Copy the background
    pic_overlay(tmp, 0, 0, &ship_1, 0xffff); // Overlay the ship
    LCD_DrawPicture(20*i - 10, 300, tmp); // Draw
  }
}

void basic_drawing(void)
{
    LCD_Clear(0);
    LCD_DrawRectangle(10, 10, 30, 50, GREEN);
    LCD_DrawFillRectangle(50, 10, 70, 50, BLUE);
    LCD_DrawLine(10, 10, 70, 50, RED);
    LCD_Circle(50, 90, 40, 1, CYAN);
    LCD_DrawTriangle(90,10, 120,10, 90,30, YELLOW);
    LCD_DrawFillTriangle(90,90, 120,120, 90,120, GRAY);
    LCD_DrawFillRectangle(10, 140, 120, 159, WHITE);
    LCD_DrawString(20,141, BLACK, WHITE, "Test string!", 16, 0); // clear background
}


