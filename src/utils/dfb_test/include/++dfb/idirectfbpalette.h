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

#ifndef IDIRECTFBPALETTE_H
#define IDIRECTFBPALETTE_H

#ifndef DFBPP_H
#error Please include ++dfb.h only.
#endif

class IDirectFBPalette : public IPPAny<IDirectFBPalette, IDirectFBPalette_C> {
friend
     class IDirectFB;
friend
     class IDirectFBSurface;

public:
     IDirectFBPalette(IDirectFBPalette_C* myptr=NULL):IPPAny<IDirectFBPalette, IDirectFBPalette_C>(myptr){}

     DFBPaletteCapabilities GetCapabilities          ();
     unsigned int           GetSize                  ();

     void                   SetEntries               (DFBColor     *entries,
                                                      unsigned int  num_entries,
                                                      unsigned int  offset);

     void                   GetEntries               (DFBColor     *entries,
                                                      unsigned int  num_entries,
                                                      unsigned int  offset);

     unsigned int           FindBestMatch            (u8          r,
                                                      u8          g,
                                                      u8          b,
                                                      u8          a);

     IDirectFBPalette       CreateCopy               ();


     inline IDirectFBPalette& operator = (const IDirectFBPalette& other){
          return IPPAny<IDirectFBPalette, IDirectFBPalette_C>::operator =(other);
     }
     inline IDirectFBPalette& operator = (IDirectFBPalette_C* other){
          return IPPAny<IDirectFBPalette, IDirectFBPalette_C>::operator =(other);
     }
};

#endif
