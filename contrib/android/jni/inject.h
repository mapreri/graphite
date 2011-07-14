/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, Inc., 59 Temple Place, 
    Suite 330, Boston, MA 02111-1307, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

    Alternatively, you may use this library under the terms of the Mozilla
    Public License (http://mozilla.org/MPL) or under the GNU General Public
    License, as published by the Free Sofware Foundation; either version
    2 of the license or (at your option) any later version.
*/

#include "SkTypeface.h"
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include "graphite2/Font.h"

struct fnmap {
    const char *starget;
    const char *ssrc;
    void *ptarget;
    void *psrc;
};

typedef struct fnmap func_map;

typedef struct rec_ft_table {
    unsigned long tag;
    void *buffer;
    struct rec_ft_table *next;
} rec_ft_table;

typedef struct myfontmap {
    struct myfontmap *next;
    const char *name;
    SkTypeface *tf;
    FT_Face ftface;
    rec_ft_table *tables;
    gr_face *grface;
} myfontmap;

#ifdef __cplusplus
extern "C"
{
#endif

void inject_fns(const char *srcname, const char *targetname, func_map *map, int num_map);
void unload_injected(const char *srcname, const char *tgtname, func_map *map, int num_map);

#ifdef __cplusplus
}
#endif

