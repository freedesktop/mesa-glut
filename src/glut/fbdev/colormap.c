/*
 * Mesa 3-D graphics library
 * Version:  6.5
 * Copyright (C) 1995-2006  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Library for glut using mesa fbdev driver
 *
 * Written by Sean D'Epagnier (c) 2006
 */

#include <stdlib.h>

#include <linux/fb.h>

#include <GL/gl.h>
#include <GL/glfbdev.h>
#include <GL/glut.h>

#include "internal.h"

#define TOCMAP(x)(unsigned short)((x<0?0:x>1?1:x)*(GLfloat) ((1<<16) - 1))
#define TORMAP(x)(unsigned short)((x<0?0:x>1?1:x)*(GLfloat)(REVERSECMAPSIZE-1))
#define FROMCMAP(x) (GLfloat)x / (GLfloat)((1<<16) - 1)

static struct fb_cmap ColorMap;

unsigned short RedColorMap[256], GreenColorMap[256], BlueColorMap[256];

unsigned char ReverseColorMap[REVERSECMAPSIZE]
                             [REVERSECMAPSIZE]
                             [REVERSECMAPSIZE];

static void FindReverseMap(int r, int g, int b)
{
   static int count;
   int i, shift = 16 - REVERSECMAPSIZELOG;
   unsigned int minv = -1, mini = 0;
   for(i=0; i<256; i++) {
      int val = 0;
      val += abs(r-(ColorMap.red[i]>>shift));
      val += abs(g-(ColorMap.green[i]>>shift));
      val += abs(b-(ColorMap.blue[i]>>shift));
      if(val < minv) {
	 minv = val;
	 mini = i;
      }
   }
   ReverseColorMap[r][g][b] = mini;
}

static void FillItemReverseColorMap(int r, int g, int b)
{
   FindReverseMap(r, g, b);
   if(r > 0)
      FindReverseMap(r-1, g, b);
   if(r < REVERSECMAPSIZE - 1)
      FindReverseMap(r+1, g, b);
   if(g > 0)
      FindReverseMap(r, g-1, b);
   if(g < REVERSECMAPSIZE - 1)
      FindReverseMap(r, g+1, b);
   if(b > 0)
      FindReverseMap(r, g, b-1);
   if(b < REVERSECMAPSIZE - 1)
      FindReverseMap(r, g, b+1);
}

static void FillReverseColorMap(void)
{
   int r, g, b;
   for(r = 0; r < REVERSECMAPSIZE; r++)
      for(g = 0; g < REVERSECMAPSIZE; g++)
	 for(b = 0; b < REVERSECMAPSIZE; b++)
	    FindReverseMap(r, g, b);
}

void RestoreColorMap(void)
{
   if (ioctl(FrameBufferFD, FBIOPUTCMAP, (void *) &ColorMap) < 0)
      sprintf(exiterror, "ioctl(FBIOPUTCMAP) failed!\n");
}

void LoadColorMap(void)
{
   /* we're assuming 256 entries here */
   int i;

   ColorMap.start = 0;
   ColorMap.len = 256;
   ColorMap.red   = RedColorMap;
   ColorMap.green = GreenColorMap;
   ColorMap.blue  = BlueColorMap;
   ColorMap.transp = NULL;

   if (ioctl(FrameBufferFD, FBIOGETCMAP, (void *) &ColorMap) < 0)
      sprintf(exiterror, "ioctl(FBIOGETCMAP) failed!\n");

   switch(VarInfo.bits_per_pixel) {
   case 15:
      for(i=0; i<32; i++)
	 RedColorMap[i] = GreenColorMap[i] = BlueColorMap[i] = i*65535/31;
      break;
   case 16:
      for(i=0; i<32; i++) 
	 RedColorMap[i] = BlueColorMap[i] = i*65535/31;
      for(i=0; i<64; i++) 
	 GreenColorMap[i] = i*65535/63;
      break;
   case 24:
   case 32:
      for(i=0; i<256; i++)
	 RedColorMap[i] = GreenColorMap[i] = BlueColorMap[i] = i*257;
      break;
   }

   RestoreColorMap();

   if(DisplayMode & GLUT_INDEX)
      FillReverseColorMap();
}

void glutSetColor(int cell, GLfloat red, GLfloat green, GLfloat blue)
{
   if(cell < 0 || cell >= 256)
      return;

   RedColorMap[cell] = TOCMAP(red);
   GreenColorMap[cell] = TOCMAP(green); 
   BlueColorMap[cell] = TOCMAP(blue);
    
   RestoreColorMap();

   FillItemReverseColorMap(TORMAP(red), TORMAP(green), TORMAP(blue));
}

GLfloat glutGetColor(int cell, int component)
{
   if(!(DisplayMode & GLUT_INDEX))
      return -1.0;

   if(cell < 0 || cell > 256)
      return -1.0;

   switch(component) {
   case GLUT_RED:
      return FROMCMAP(RedColorMap[cell]);
   case GLUT_GREEN:
      return FROMCMAP(GreenColorMap[cell]);
   case GLUT_BLUE:
      return FROMCMAP(BlueColorMap[cell]);
   }
   return -1.0;
}

void glutCopyColormap(int win)
{
}
