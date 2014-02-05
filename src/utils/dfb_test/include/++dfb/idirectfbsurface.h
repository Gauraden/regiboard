/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   All rights reserved.

   Written by Denis Oliver Kropp <dok@convergence.de>,
              Andreas Hundt <andi@convergence.de> and
              Sven Neumann <sven@convergence.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef IDIRECTFBSURFACE_H
#define IDIRECTFBSURFACE_H

#ifndef DFBPP_H
#error Please include ++dfb.h only.
#endif

class IDirectFBSurface : public IPPAny<IDirectFBSurface, IDirectFBSurface_C> {
friend
     class IDirectFB;
friend
     class IDirectFBDisplayLayer;
friend
     class IDirectFBImageProvider;
friend
     class IDirectFBVideoProvider;
friend
     class IDirectFBWindow;

public:
     IDirectFBSurface(IDirectFBSurface_C* myptr=NULL):IPPAny<IDirectFBSurface, IDirectFBSurface_C>(myptr){}

     DFBSurfaceCapabilities GetCapabilities     ();
     void                   GetPosition         (int                      *x,
                                                 int                      *y);
     void                   GetSize             (int                      *width,
                                                 int                      *height);
     void                   GetVisibleRectangle (DFBRectangle             *rect);
     DFBSurfacePixelFormat  GetPixelFormat      ();
     DFBAccelerationMask    GetAccelerationMask (IDirectFBSurface         *source = NULL);

     IDirectFBPalette       GetPalette          ();
     void                   SetPalette          (IDirectFBPalette         *palette);
     void                   SetAlphaRamp        (u8                      a0,
                                                 u8                      a1,
                                                 u8                      a2,
                                                 u8                      a3);

     void                   Lock                (DFBSurfaceLockFlags       flags,
                                                 void                    **ptr,
                                                 int                      *pitch);
     void                   Unlock              ();
     void                   Flip                (DFBRegion                *region = NULL,
                                                 DFBSurfaceFlipFlags       flags = static_cast<DFBSurfaceFlipFlags>(0));
     void                   SetField            (int                       field);
     void                   Clear               (u8                      r = 0x00,
                                                 u8                      g = 0x00,
                                                 u8                      b = 0x00,
                                                 u8                      a = 0x00);
     void                   Clear               (DFBColor               &color);

     void                   SetClip             (const DFBRegion          *clip = 0);
     void                   SetClip             (const DFBRectangle       *clip);
     void                   SetColor            (u8                      r,
                                                 u8                      g,
                                                 u8                      b,
                                                 u8                      a = 0xFF);
     void                   SetColor            (DFBColor               &color);
     void                   SetColorIndex       (unsigned int              index);
     void                   SetSrcBlendFunction (DFBSurfaceBlendFunction   function);
     void                   SetDstBlendFunction (DFBSurfaceBlendFunction   function);
     void                   SetPorterDuff       (DFBSurfacePorterDuffRule  rule);
     void                   SetSrcColorKey      (u8                      r,
                                                 u8                      g,
                                                 u8                      b);
     void                   SetSrcColorKeyIndex (unsigned int              index);
     void                   SetDstColorKey      (u8                      r,
                                                 u8                      g,
                                                 u8                      b);
     void                   SetDstColorKeyIndex (unsigned int              index);

     void                   SetBlittingFlags    (DFBSurfaceBlittingFlags   flags);
     void                   Blit                (IDirectFBSurface         *source,
                                                 const DFBRectangle       *source_rect = NULL,
                                                 int                       x = 0,
                                                 int                       y = 0);
     void                   TileBlit            (IDirectFBSurface         *source,
                                                 const DFBRectangle       *source_rect = NULL,
                                                 int                       x = 0,
                                                 int                       y = 0);
     void                   BatchBlit           (IDirectFBSurface         *source,
                                                 const DFBRectangle       *source_rects,
                                                 const DFBPoint           *dest_points,
                                                 int                       num);
     void                   StretchBlit         (IDirectFBSurface         *source,
                                                 const DFBRectangle       *source_rect = NULL,
                                                 const DFBRectangle       *destination_rect = NULL);

     void                   TextureTriangles    (IDirectFBSurface         *source,
                                                 const DFBVertex          *vertices,
                                                 const int                *indices,
                                                 int                       num,
                                                 DFBTriangleFormation      formation);

     void                   SetDrawingFlags     (DFBSurfaceDrawingFlags    flags);
     void                   FillRectangle       (int                       x,
                                                 int                       y,
                                                 int                       width,
                                                 int                       height);
     void                   FillRectangle       (DFBRectangle             &rect);
     void                   FillRectangle       (DFBRegion                &rect);
     void                   FillRectangles      (const DFBRectangle       *rects,
                                                 unsigned int              num_rects);
     void                   DrawRectangle       (int                       x,
                                                 int                       y,
                                                 int                       width,
                                                 int                       height);
     void                   DrawLine            (int                       x1,
                                                 int                       y1,
                                                 int                       x2,
                                                 int                       y2); 
     void                   DrawLines           (const DFBRegion          *lines,
                                                 unsigned int              num_lines);
     void                   FillTriangle        (int                       x1,
                                                 int                       y1,
                                                 int                       x2,
                                                 int                       y2,
                                                 int                       x3,
                                                 int                       y3);
     void                   FillSpans           (int                       y,
                                                 const DFBSpan            *spans,
                                                 unsigned int              num);

     void                   SetFont             (const IDirectFBFont &font) const;
     IDirectFBFont          GetFont             () const;
     void                   DrawString          (const char               *text,
                                                 int                       bytes,
                                                 int                       x,
                                                 int                       y,
                                                 DFBSurfaceTextFlags       flags);
     void                   DrawGlyph           (unsigned int              index,
                                                 int                       x,
                                                 int                       y,
                                                 DFBSurfaceTextFlags       flags);
     void                   SetEncoding         (DFBTextEncodingID         encoding);

     IDirectFBSurface       GetSubSurface       (DFBRectangle             *rect);

     void                   Dump                (const char               *directory,
                                                 const char               *prefix);

     void                   DisableAcceleration (DFBAccelerationMask       mask);

     IDirectFBGL           *GetGL               ();

     /* Additional methods added for enhanced usability */

     int                    GetWidth            ();
     int                    GetHeight           ();

     void                   SetColor            (const DFBColor           &color);
     void                   SetColor            (const DFBColor           *color);

     void                   FillRectangle       (const DFBRectangle       &rect);
     void                   DrawRectangle       (const DFBRectangle       &rect);
     void                   DrawLine            (const DFBRegion          &line);

     IDirectFBSurface       GetSubSurface       (int                       x,
                                                 int                       y,
                                                 int                       width,
                                                 int                       height);

     void                   GetClip             (DFBRegion                *clip);

     int                    GetFramebufferOffset();

     void                   ReleaseSource       ();
     void                   SetIndexTranslation (const int                *indices,
                                                 int                       num_indices);

     void                   Read                (void                     *ptr,
                                                 int                       pitch,
                                                 const DFBRectangle       *rect = NULL);

     void                   Write               (const void               *ptr,
                                                 int                       pitch,
                                                 const DFBRectangle       *rect = NULL);

     inline IDirectFBSurface& operator = (const IDirectFBSurface& other){
          return IPPAny<IDirectFBSurface, IDirectFBSurface_C>::operator =(other);
     }
     inline IDirectFBSurface& operator = (IDirectFBSurface_C* other){
          return IPPAny<IDirectFBSurface, IDirectFBSurface_C>::operator =(other);
     }

     void                   SetRenderOptions    (DFBSurfaceRenderOptions   options);
     void                   SetMatrix           (const s32                *matrix3x3);
};

#endif
