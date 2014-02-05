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

#ifndef IDIRECTFBDISPLAYLAYER_H
#define IDIRECTFBDISPLAYLAYER_H

#ifndef DFBPP_H
#error Please include ++dfb.h only.
#endif

class IDirectFBDisplayLayer : public IPPAny<IDirectFBDisplayLayer, IDirectFBDisplayLayer_C> {
friend
     class IDirectFB;

public:
     IDirectFBDisplayLayer(IDirectFBDisplayLayer_C* myptr=NULL):IPPAny<IDirectFBDisplayLayer, IDirectFBDisplayLayer_C>(myptr){}

     DFBDisplayLayerID      GetID                 ();

     DFBDisplayLayerDescription
                            GetDescription        ();
                            
     void                   GetSourceDescriptions (DFBDisplayLayerSourceDescription *desc);

     IDirectFBSurface       GetSurface            ();

     IDirectFBScreen        GetScreen             ();

     void                   SetCooperativeLevel   (DFBDisplayLayerCooperativeLevel level);
     void                   SetOpacity            (u8                            opacity);
     void                   SetSourceRectangle    (int                             x,
                                                   int                             y,
                                                   int                             width,
                                                   int                             height);
     void                   SetScreenLocation     (float                           x,
                                                   float                           y,
                                                   float                           width,
                                                   float                           height);
     void                   SetScreenPosition     (int                             x,
                                                   int                             y);
     void                   SetScreenRectangle    (int                             x,
                                                   int                             y,
                                                   int                             width,
                                                   int                             height);
     void                   SetClipRegions        (const DFBRegion                *regions,
                                                   int                             num_regions,
                                                   DFBBoolean                      positive);
     void                   SetSrcColorKey        (u8                            r,
                                                   u8                            g,
                                                   u8                            b);
     void                   SetDstColorKey        (u8                            r,
                                                   u8                            g,
                                                   u8                            b);
     int                    GetLevel              ();
     void                   SetLevel              (int                             level);
     int                    GetCurrentOutputField ();
     void                   SetFieldParity        (int                             field);
     void                   WaitForSync           ();

     void                   GetConfiguration      (DFBDisplayLayerConfig          *config);
     void                   TestConfiguration     (DFBDisplayLayerConfig          &config,
                                                   DFBDisplayLayerConfigFlags     *failed = NULL);
     void                   SetConfiguration      (DFBDisplayLayerConfig          &config);

     void                   SetBackgroundMode     (DFBDisplayLayerBackgroundMode   mode);
     void                   SetBackgroundImage    (IDirectFBSurface               *surface);
     void                   SetBackgroundColor    (u8                            r,
                                                   u8                            g,
                                                   u8                            b,
                                                   u8                            a = 0xFF);

     void                   GetColorAdjustment    (DFBColorAdjustment             *adj);
     void                   SetColorAdjustment    (DFBColorAdjustment             &adj);

     IDirectFBWindow        CreateWindow          (DFBWindowDescription           &desc);
     IDirectFBWindow        GetWindow             (DFBWindowID                     window_id);

     void                   EnableCursor          (bool                            enable);
     void                   GetCursorPosition     (int                            *x,
                                                   int                            *y);
     void                   WarpCursor            (int                             x,
                                                   int                             y);
     void                   SetCursorAcceleration (int                             numerator,
                                                   int                             denominator,
                                                   int                             threshold);
     void                   SetCursorShape        (IDirectFBSurface               *shape,
                                                   int                             hot_x,
                                                   int                             hot_y);
     void                   SetCursorOpacity      (u8                            opacity);

     void                   SwitchContext         (DFBBoolean                      exclusive);

     inline IDirectFBDisplayLayer& operator = (const IDirectFBDisplayLayer& other){
          return IPPAny<IDirectFBDisplayLayer, IDirectFBDisplayLayer_C>::operator =(other);
     }
     inline IDirectFBDisplayLayer& operator = (IDirectFBDisplayLayer_C* other){
          return IPPAny<IDirectFBDisplayLayer, IDirectFBDisplayLayer_C>::operator =(other);
     }
};

#endif
