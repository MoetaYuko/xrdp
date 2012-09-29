/**
 * FreeRDP: A Remote Desktop Protocol Server
 * freerdp wrapper
 *
 * Copyright 2011-2012 Jay Sorg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xrdp-freerdp.h"
#include "xrdp-color.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { g_write _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { g_writeln _args ; } } while (0)

struct mod_context
{
  rdpContext _p;
  struct mod* modi;
};
typedef struct mod_context modContext;

/*****************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_start(struct mod* mod, int w, int h, int bpp)
{
  rdpSettings* settings;

  LLOGLN(10, ("lxrdp_start: w %d h %d bpp %d", w, h, bpp));
  settings = mod->inst->settings;
  settings->width = w;
  settings->height = h;
  settings->color_depth = bpp;
  mod->bpp = bpp;

  settings->encryption = 1;
  settings->tls_security = 1;
  settings->nla_security = 0;
  settings->rdp_security = 1;

  return 0;
}

/******************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_connect(struct mod* mod)
{
  boolean ok;

  LLOGLN(10, ("lxrdp_connect:"));

  ok = freerdp_connect(mod->inst);
  LLOGLN(0, ("lxrdp_connect: freerdp_connect returned %d", ok));

  if (!ok)
  {
    return 1;
  }
  return 0;
}

/******************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_event(struct mod* mod, int msg, long param1, long param2,
            long param3, long param4)
{
  int x;
  int y;
  int flags;
  int size;
  int total_size;
  int chanid;
  int lchid;
  char* data;

  LLOGLN(10, ("lxrdp_event: msg %d", msg));
  switch (msg)
  {
    case 15: /* key down */
      mod->inst->input->KeyboardEvent(mod->inst->input, param4, param3);
      break;
    case 16: /* key up */
      mod->inst->input->KeyboardEvent(mod->inst->input, param4, param3);
      break;
    case 100: /* mouse move */
      LLOGLN(10, ("mouse move %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_MOVE;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 101: /* left button up */
      LLOGLN(10, ("left button up %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON1;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 102: /* left button down */
      LLOGLN(10, ("left button down %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON1 | PTR_FLAGS_DOWN;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 103: /* right button up */
      LLOGLN(10, ("right button up %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON2;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 104: /* right button down */
      LLOGLN(10, ("right button down %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON2 | PTR_FLAGS_DOWN;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 105: /* middle button up */
      LLOGLN(10, ("middle button up %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON3;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 106: /* middle button down */
      LLOGLN(10, ("middle button down %d %d", param1, param2));
      x = param1;
      y = param2;
      flags = PTR_FLAGS_BUTTON3 | PTR_FLAGS_DOWN;
      mod->inst->input->MouseEvent(mod->inst->input, flags, x, y);
      break;
    case 107: /* wheel up */
      flags = PTR_FLAGS_WHEEL | 0x0078;
      mod->inst->input->MouseEvent(mod->inst->input, flags, 0, 0);
    case 108:
      break;
    case 109: /* wheel down */
      flags = PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x0088;
      mod->inst->input->MouseEvent(mod->inst->input, flags, 0, 0);
    case 110:
      break;
    case 0x5555:
      chanid = LOWORD(param1);
      flags = HIWORD(param1);
      size = (int)param2;
      data = (char*)param3;
      total_size = (int)param4;
      LLOGLN(10, ("lxrdp_event: client to server flags %d", flags));
      if ((chanid < 0) || (chanid >= mod->inst->settings->num_channels))
      {
        LLOGLN(0, ("lxrdp_event: error chanid %d", chanid));
        break;
      }
      lchid = mod->inst->settings->channels[chanid].channel_id;
      switch (flags & 3)
      {
        case 3:
          mod->inst->SendChannelData(mod->inst, lchid, data, total_size);
          break;
        case 2:
          /* end */
          g_memcpy(mod->chan_buf + mod->chan_buf_valid, data, size);
          mod->chan_buf_valid += size;
          mod->inst->SendChannelData(mod->inst, lchid, mod->chan_buf, total_size);
          g_free(mod->chan_buf);
          mod->chan_buf = 0;
          mod->chan_buf_bytes = 0;
          mod->chan_buf_valid = 0;
          break;
        case 1:
          /* start */
          g_free(mod->chan_buf);
          mod->chan_buf = (char*)g_malloc(total_size, 0);
          mod->chan_buf_bytes = total_size;
          mod->chan_buf_valid = 0;
          g_memcpy(mod->chan_buf + mod->chan_buf_valid, data, size);
          mod->chan_buf_valid += size;
          break;
        default:
          /* middle */
          g_memcpy(mod->chan_buf + mod->chan_buf_valid, data, size);
          mod->chan_buf_valid += size;
          break;
      }
      break;
  }
  return 0;
}

/******************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_signal(struct mod* mod)
{
  LLOGLN(10, ("lxrdp_signal:"));
  return 0;
}

/******************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_end(struct mod* mod)
{
  int i;
  int j;

  for (j = 0; j < 4; j++)
  {
    for (i = 0; i < 4096; i++)
    {
      g_free(mod->bitmap_cache[j][i].data);
    }
  }
  for (i = 0; i < 64; i++)
  {
    if (mod->brush_cache[i].data != mod->brush_cache[i].b8x8)
    {
      g_free(mod->brush_cache[i].data);
    }
  }
  LLOGLN(10, ("lxrdp_end:"));
  return 0;
}

/******************************************************************************/
/* return error */
static int DEFAULT_CC
lxrdp_set_param(struct mod* mod, char* name, char* value)
{
  rdpSettings* settings;

  LLOGLN(10, ("lxrdp_set_param: name [%s] value [%s]", name, value));
  settings = mod->inst->settings;

  LLOGLN(10, ("%p %d", settings->hostname, settings->encryption));

  if (g_strcmp(name, "hostname") == 0)
  {
  }
  else if (g_strcmp(name, "ip") == 0)
  {
    settings->hostname = g_strdup(value);
  }
  else if (g_strcmp(name, "port") == 0)
  {
    settings->port = g_atoi(value);
  }
  else if (g_strcmp(name, "keylayout") == 0)
  {
  }
  else if (g_strcmp(name, "name") == 0)
  {
  }
  else if (g_strcmp(name, "lib") == 0)
  {
  }
  else
  {
    LLOGLN(0, ("lxrdp_set_param: unknown name [%s] value [%s]", name, value));
  }

  return 0;
}

/******************************************************************************/
static int DEFAULT_CC
lxrdp_session_change(struct mod* mod, int a, int b)
{
  LLOGLN(10, ("lxrdp_session_change:"));
  return 0;
}

/******************************************************************************/
static int DEFAULT_CC
lxrdp_get_wait_objs(struct mod* mod, tbus* read_objs, int* rcount,
                  tbus* write_objs, int* wcount, int* timeout)
{
  void** rfds;
  void** wfds;
  boolean ok;

  LLOGLN(10, ("lxrdp_get_wait_objs:"));
  rfds = (void**)read_objs;
  wfds = (void**)write_objs;
  ok = freerdp_get_fds(mod->inst, rfds, rcount, wfds, wcount);
  if (!ok)
  {
    LLOGLN(0, ("lxrdp_get_wait_objs: freerdp_get_fds failed"));
    return 1;
  }
  return 0;
}

/******************************************************************************/
static int DEFAULT_CC
lxrdp_check_wait_objs(struct mod* mod)
{
  boolean ok;

  LLOGLN(10, ("lxrdp_check_wait_objs:"));
  ok = freerdp_check_fds(mod->inst);
  if (!ok)
  {
    LLOGLN(0, ("lxrdp_check_wait_objs: freerdp_check_fds failed"));
    return 1;
  }
  return 0;
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_begin_paint(rdpContext* context)
{
  struct mod* mod;

  LLOGLN(10, ("lfreerdp_begin_paint:"));
  mod = ((struct mod_context*)context)->modi;
  mod->server_begin_update(mod);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_end_paint(rdpContext* context)
{
  struct mod* mod;

  LLOGLN(10, ("lfreerdp_end_paint:"));
  mod = ((struct mod_context*)context)->modi;
  mod->server_end_update(mod);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_set_bounds(rdpContext* context, rdpBounds* bounds)
{
  struct mod* mod;
  int x;
  int y;
  int cx;
  int cy;

  LLOGLN(10, ("lfreerdp_set_bounds: %p", bounds));
  mod = ((struct mod_context*)context)->modi;
  if (bounds != 0)
  {
    x = bounds->left;
    y = bounds->top;
    cx = (bounds->right - bounds->left) + 1;
    cy = (bounds->bottom - bounds->top) + 1;
    mod->server_set_clip(mod, x, y, cx, cy);
  }
  else
  {
    mod->server_reset_clip(mod);
  }
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_bitmap_update(rdpContext* context, BITMAP_UPDATE* bitmap)
{
  struct mod* mod;
  int index;
  int cx;
  int cy;
  int server_bpp;
  int server_Bpp;
  int client_bpp;
  int j;
  int line_bytes;
  BITMAP_DATA* bd;
  tui8* dst_data;
  tui8* dst_data1;
  tui8* src;
  tui8* dst;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_bitmap_update: %d %d", bitmap->number, bitmap->count));

  server_bpp = mod->inst->settings->color_depth;
  server_Bpp = (server_bpp + 7) / 8;
  client_bpp = mod->bpp;

  for (index = 0; index < bitmap->number; index++)
  {
    bd = bitmap->rectangles + index;
    cx = (bd->destRight - bd->destLeft) + 1;
    cy = (bd->destBottom - bd->destTop) + 1;
    line_bytes = server_Bpp * bd->width;
    dst_data = (tui8*)g_malloc(bd->height * line_bytes + 16, 0);
    if (bd->compressed)
    {
      bitmap_decompress(bd->bitmapDataStream, dst_data, bd->width,
                        bd->height, bd->bitmapLength, server_bpp, server_bpp);
    }
    else
    { /* bitmap is upside down */
      src = bd->bitmapDataStream;
      dst = dst_data + bd->height * line_bytes;
      for (j = 0; j < bd->height; j++)
      {
        dst -= line_bytes;
        g_memcpy(dst, src, line_bytes);
        src += line_bytes;
      }
    }
    dst_data1 = convert_bitmap(server_bpp, client_bpp, dst_data,
                               bd->width, bd->height, mod->colormap);
    mod->server_paint_rect(mod, bd->destLeft, bd->destTop, cx, cy,
                           dst_data1, bd->width, bd->height, 0, 0);
    if (dst_data1 != dst_data)
    {
      g_free(dst_data1);
    }
    g_free(dst_data);
  }
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_dst_blt(rdpContext* context, DSTBLT_ORDER* dstblt)
{
  struct mod* mod;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_dst_blt:"));
  mod->server_set_opcode(mod, dstblt->bRop);
  mod->server_fill_rect(mod, dstblt->nLeftRect, dstblt->nTopRect,
                        dstblt->nWidth, dstblt->nHeight);
  mod->server_set_opcode(mod, 0xcc);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pat_blt(rdpContext* context, PATBLT_ORDER* patblt)
{
  struct mod* mod;
  int idx;
  int fgcolor;
  int bgcolor;
  int server_bpp;
  int client_bpp;
  struct brush_item* bi;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_pat_blt:"));

  server_bpp = mod->inst->settings->color_depth;
  client_bpp = mod->bpp;
  LLOGLN(0, ("lfreerdp_pat_blt: bpp %d %d", server_bpp, client_bpp));

  fgcolor = convert_color(server_bpp, client_bpp,
                          patblt->foreColor, mod->colormap);
  bgcolor = convert_color(server_bpp, client_bpp,
                          patblt->backColor, mod->colormap);

  mod->server_set_mixmode(mod, 1);
  mod->server_set_opcode(mod, patblt->bRop);
  mod->server_set_fgcolor(mod, fgcolor);
  mod->server_set_bgcolor(mod, bgcolor);

  if (patblt->brush.style & 0x80)
  {
    idx = patblt->brush.hatch;
    if ((idx < 0) || (idx >= 64))
    {
      LLOGLN(0, ("lfreerdp_pat_blt: error"));
      return;
    }
    bi = mod->brush_cache + idx;
    mod->server_set_brush(mod, patblt->brush.x, patblt->brush.y,
                          3, bi->b8x8);
  }
  else
  {
    mod->server_set_brush(mod, patblt->brush.x, patblt->brush.y,
                          patblt->brush.style, patblt->brush.p8x8);
  }
  mod->server_fill_rect(mod, patblt->nLeftRect, patblt->nTopRect,
                        patblt->nWidth, patblt->nHeight);
  mod->server_set_opcode(mod, 0xcc);
  mod->server_set_mixmode(mod, 0);

}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_scr_blt(rdpContext* context, SCRBLT_ORDER* scrblt)
{
  struct mod* mod;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_scr_blt:"));
  mod->server_set_opcode(mod, scrblt->bRop);
  mod->server_screen_blt(mod, scrblt->nLeftRect, scrblt->nTopRect,
                         scrblt->nWidth, scrblt->nHeight,
                         scrblt->nXSrc, scrblt->nYSrc);
  mod->server_set_opcode(mod, 0xcc);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_opaque_rect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect)
{
  struct mod* mod;
  int server_bpp;
  int client_bpp;
  int fgcolor;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_opaque_rect:"));
  server_bpp = mod->inst->settings->color_depth;
  client_bpp = mod->bpp;
  fgcolor = convert_color(server_bpp, client_bpp,
                          opaque_rect->color, mod->colormap);
  mod->server_set_fgcolor(mod, fgcolor);
  mod->server_fill_rect(mod, opaque_rect->nLeftRect, opaque_rect->nTopRect,
                        opaque_rect->nWidth, opaque_rect->nHeight);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_mem_blt(rdpContext* context, MEMBLT_ORDER* memblt)
{
  int id;
  int idx;
  struct mod* mod;
  struct bitmap_item* bi;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_mem_blt: cacheId %d cacheIndex %d",
         memblt->cacheId, memblt->cacheIndex));

  id = memblt->cacheId;
  idx = memblt->cacheIndex;

  if (idx == 32767) /* BITMAPCACHE_WAITING_LIST_INDEX */
  {
    idx = 4096 - 1;
  }

  if ((id < 0) || (id >= 4))
  {
    LLOGLN(0, ("lfreerdp_mem_blt: bad id [%d]", id));
    return;
  }
  if ((idx < 0) || (idx >= 4096))
  {
    LLOGLN(0, ("lfreerdp_mem_blt: bad idx [%d]", idx));
    return;
  }

  bi = &(mod->bitmap_cache[id][idx]);

  mod->server_set_opcode(mod, memblt->bRop);
  mod->server_paint_rect(mod, memblt->nLeftRect, memblt->nTopRect,
                         memblt->nWidth, memblt->nHeight,
                         bi->data, bi->width, bi->height,
                         memblt->nXSrc, memblt->nYSrc);
  mod->server_set_opcode(mod, 0xcc);

}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_glyph_index(rdpContext* context, GLYPH_INDEX_ORDER* glyph_index)
{
  struct mod* mod;
  int server_bpp;
  int client_bpp;
  int fgcolor;
  int bgcolor;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_glyph_index:"));
  server_bpp = mod->inst->settings->color_depth;
  client_bpp = mod->bpp;
  fgcolor = convert_color(server_bpp, client_bpp,
                          glyph_index->foreColor, mod->colormap);
  bgcolor = convert_color(server_bpp, client_bpp,
                          glyph_index->backColor, mod->colormap);
  mod->server_set_bgcolor(mod, fgcolor);
  mod->server_set_fgcolor(mod, bgcolor);
  mod->server_draw_text(mod, glyph_index->cacheId, glyph_index->flAccel,
      glyph_index->fOpRedundant,
      glyph_index->bkLeft, glyph_index->bkTop,
      glyph_index->bkRight, glyph_index->bkBottom,
      glyph_index->opLeft, glyph_index->opTop,
      glyph_index->opRight, glyph_index->opBottom,
      glyph_index->x, glyph_index->y,
      glyph_index->data, glyph_index->cbData);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_line_to(rdpContext* context, LINE_TO_ORDER* line_to)
{
  struct mod* mod;
  int server_bpp;
  int client_bpp;
  int fgcolor;
  int bgcolor;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_line_to:"));
  mod->server_set_opcode(mod, line_to->bRop2);
  server_bpp = mod->inst->settings->color_depth;
  client_bpp = mod->bpp;
  fgcolor = convert_color(server_bpp, client_bpp,
                          line_to->penColor, mod->colormap);
  bgcolor = convert_color(server_bpp, client_bpp,
                          line_to->backColor, mod->colormap);
  mod->server_set_fgcolor(mod, fgcolor);
  mod->server_set_bgcolor(mod, bgcolor);
  mod->server_set_pen(mod, line_to->penStyle, line_to->penWidth);
  mod->server_draw_line(mod, line_to->nXStart, line_to->nYStart,
                        line_to->nXEnd, line_to->nYEnd);
  mod->server_set_opcode(mod, 0xcc);
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_cache_bitmap(rdpContext* context, CACHE_BITMAP_ORDER* cache_bitmap_order)
{
  LLOGLN(10, ("lfreerdp_cache_bitmap:"));
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_cache_bitmapV2(rdpContext* context,
                        CACHE_BITMAP_V2_ORDER* cache_bitmap_v2_order)
{
  char* dst_data;
  char* dst_data1;
  int bytes;
  int width;
  int height;
  int id;
  int idx;
  int flags;
  int server_bpp;
  int server_Bpp;
  int client_bpp;
  struct mod* mod;

  LLOGLN(10, ("lfreerdp_cache_bitmapV2: %d %d 0x%8.8x compressed %d",
            cache_bitmap_v2_order->cacheId,
            cache_bitmap_v2_order->cacheIndex,
            cache_bitmap_v2_order->flags,
            cache_bitmap_v2_order->compressed));

  mod = ((struct mod_context*)context)->modi;
  id = cache_bitmap_v2_order->cacheId;
  idx = cache_bitmap_v2_order->cacheIndex;
  flags = cache_bitmap_v2_order->flags;
  if (flags & 0x10) /* CBR2_DO_NOT_CACHE */
  {
    idx = 4096 - 1;
  }
  if ((id < 0) || (id >= 4))
  {
    LLOGLN(0, ("lfreerdp_cache_bitmapV2: bad id [%d]", id));
    return;
  }
  if ((idx < 0) || (idx >= 4096))
  {
    LLOGLN(0, ("lfreerdp_cache_bitmapV2: bad idx [%d]", idx));
    return;
  }

  server_bpp = mod->inst->settings->color_depth;
  server_Bpp = (server_bpp + 7) / 8;
  client_bpp = mod->bpp;

  width = cache_bitmap_v2_order->bitmapWidth;
  height = cache_bitmap_v2_order->bitmapHeight;
  bytes = width * height * server_Bpp + 16;
  dst_data = (char*)g_malloc(bytes, 0);
  if (cache_bitmap_v2_order->compressed)
  {
    bitmap_decompress(cache_bitmap_v2_order->bitmapDataStream,
                      dst_data, width, height,
                      cache_bitmap_v2_order->bitmapLength,
                      server_bpp, server_bpp);
  }
  else
  {
    g_memcpy(dst_data, cache_bitmap_v2_order->bitmapDataStream,
             width * height * server_Bpp);
  }
  dst_data1 = convert_bitmap(server_bpp, client_bpp, dst_data,
                             width, height, mod->colormap);
  g_free(mod->bitmap_cache[id][idx].data);
  mod->bitmap_cache[id][idx].width = width;
  mod->bitmap_cache[id][idx].height = height;
  mod->bitmap_cache[id][idx].data = dst_data1;
  if (dst_data != dst_data1)
  {
    g_free(dst_data);
  }
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_cache_glyph(rdpContext* context, CACHE_GLYPH_ORDER* cache_glyph_order)
{
  int index;
  GLYPH_DATA* gd;
  struct mod* mod;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(10, ("lfreerdp_cache_glyph: %d", cache_glyph_order->cGlyphs));
  for (index = 0; index < cache_glyph_order->cGlyphs; index++)
  {
    gd = cache_glyph_order->glyphData[index];
    LLOGLN(10, ("  %d %d %d %d %d", gd->cacheIndex, gd->x, gd->y,
           gd->cx, gd->cy));
    mod->server_add_char(mod, cache_glyph_order->cacheId, gd->cacheIndex,
                         gd->x, gd->y, gd->cx, gd->cy, gd->aj);
    xfree(gd->aj);
    gd->aj = 0;
    xfree(gd);
    cache_glyph_order->glyphData[index] = 0;
  }
  xfree(cache_glyph_order->unicodeCharacters);
  cache_glyph_order->unicodeCharacters = 0;
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_cache_brush(rdpContext* context, CACHE_BRUSH_ORDER* cache_brush_order)
{
  int idx;
  int bytes;
  int bpp;
  int cx;
  int cy;
  struct mod* mod;

  mod = ((struct mod_context*)context)->modi;
  bpp = cache_brush_order->bpp;
  cx = cache_brush_order->cx;
  cy = cache_brush_order->cy;
  idx = cache_brush_order->index;
  bytes = cache_brush_order->length;
  LLOGLN(10, ("lfreerdp_cache_brush: bpp %d cx %d cy %d idx %d bytes %d",
         bpp, cx, cy, idx, bytes));
  if ((idx < 0) || (idx >= 64))
  {
    LLOGLN(0, ("lfreerdp_cache_brush: error idx %d", idx));
    return;
  }
  if ((bpp != 1) || (cx != 8) || (cy != 8))
  {
    LLOGLN(0, ("lfreerdp_cache_brush: error unsupported brush "
           "bpp %d cx %d cy %d", bpp, cx, cy));
    return;
  }
  mod->brush_cache[idx].bpp = bpp;
  mod->brush_cache[idx].width = cx;
  mod->brush_cache[idx].height = cy;
  mod->brush_cache[idx].data = mod->brush_cache[idx].b8x8;
  if (bytes > 8)
  {
    bytes = 8;
  }
  g_memset(mod->brush_cache[idx].data, 0, 8);
  if (bytes > 0)
  {
    if (bytes > 8)
    {
      LLOGLN(0, ("lfreerdp_cache_brush: bytes to big %d", bytes));
      bytes = 8;
    }
    g_memcpy(mod->brush_cache[idx].data, cache_brush_order->data, bytes);
  }
  LLOGLN(10, ("lfreerdp_cache_brush: out bpp %d cx %d cy %d idx %d bytes %d",
         bpp, cx, cy, idx, bytes));

  xfree(cache_brush_order->data);
  cache_brush_order->data = 0;

}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pointer_position(rdpContext* context,
                          POINTER_POSITION_UPDATE* pointer_position)
{
  LLOGLN(0, ("lfreerdp_pointer_position:"));
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pointer_system(rdpContext* context,
                        POINTER_SYSTEM_UPDATE* pointer_system)
{
  LLOGLN(0, ("lfreerdp_pointer_system:"));
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pointer_color(rdpContext* context,
                       POINTER_COLOR_UPDATE* pointer_color)
{
  LLOGLN(0, ("lfreerdp_pointer_color:"));
}

/******************************************************************************/
static int APP_CC
lfreerdp_get_pixel(void* bits, int width, int height, int bpp,
                   int delta, int x, int y)
{
  int start;
  int shift;
  int pixel;
  tui8* src8;

  if (bpp == 1)
  {
    src8 = (tui8*)bits;
    start = (y * delta) + x / 8;
    shift = x % 8;
    pixel = (src8[start] & (0x80 >> shift)) != 0;
    return pixel ? 0xffffff : 0;
  }
  else
  {
    LLOGLN(0, ("lfreerdp_get_pixel: unknown bpp %d", bpp));
  }
  return 0;
}

/******************************************************************************/
static int APP_CC
lfreerdp_set_pixel(int pixel, void* bits, int width, int height, int bpp,
                   int delta, int x, int y)
{
  tui8* dst8;
  int start;
  int shift;

  if (bpp == 1)
  {
    dst8 = (tui8*)bits;
    start = (y * delta) + x / 8;
    shift = x % 8;
    if (pixel)
    {
      dst8[start] = dst8[start] | (0x80 >> shift);
    }
    else
    {
      dst8[start] = dst8[start] & ~(0x80 >> shift);
    }
  }
  else if (bpp == 24)
  {
    dst8 = (tui8*)bits;
    dst8 += y * delta + x * 3;
    dst8[0] = (pixel >> 0) & 0xff;
    dst8[1] = (pixel >> 8) & 0xff;
    dst8[2] = (pixel >> 16) & 0xff;
  }
  else
  {
    LLOGLN(0, ("lfreerdp_set_pixel: unknown bpp %d", bpp));
  }
  return 0;
}

/******************************************************************************/
static int APP_CC
lfreerdp_convert_color_image(void* dst, int dst_width, int dst_height,
                             int dst_bpp, int dst_delta,
                             void* src, int src_width, int src_height,
                             int src_bpp, int src_delta)
{
  int i;
  int j;
  int pixel;

  for (j = 0; j < dst_height; j++)
  {
    for (i = 0; i < dst_width; i++)
    {
      pixel = lfreerdp_get_pixel(src, src_width, src_height, src_bpp,
                                 src_delta, i, j);
      lfreerdp_set_pixel(pixel, dst, dst_width, dst_height, dst_bpp,
                         dst_delta, i, j);
    }
  }
  return 0;
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pointer_new(rdpContext* context,
                     POINTER_NEW_UPDATE* pointer_new)
{
  struct mod* mod;
  int index;
  tui8* dst;
  tui8* src;

  mod = ((struct mod_context*)context)->modi;
  LLOGLN(0, ("lfreerdp_pointer_new:"));
  LLOGLN(0, ("  bpp %d", pointer_new->xorBpp));
  LLOGLN(0, ("  width %d height %d", pointer_new->colorPtrAttr.width,
      pointer_new->colorPtrAttr.height));

  LLOGLN(0, ("  lengthXorMask %d lengthAndMask %d",
      pointer_new->colorPtrAttr.lengthXorMask,
      pointer_new->colorPtrAttr.lengthAndMask));

  index = pointer_new->colorPtrAttr.cacheIndex;
  if (pointer_new->xorBpp == 1 &&
      pointer_new->colorPtrAttr.width == 32 &&
      pointer_new->colorPtrAttr.height == 32 &&
      index < 32)
  {
    mod->pointer_cache[index].hotx = pointer_new->colorPtrAttr.xPos;
    mod->pointer_cache[index].hoty = pointer_new->colorPtrAttr.yPos;

    dst = mod->pointer_cache[index].data;
    dst += 32 * 32 * 3 - 32 * 3;
    src = pointer_new->colorPtrAttr.xorMaskData;
    lfreerdp_convert_color_image(dst, 32, 32, 24, 32 * -3,
                                 src, 32, 32, 1, 32 / 8);

    dst = mod->pointer_cache[index].mask;
    dst += 32 * 32 / 8 - 32 / 8;
    src = pointer_new->colorPtrAttr.andMaskData;
    lfreerdp_convert_color_image(dst, 32, 32, 1, 32 / -8,
                                 src, 32, 32, 1, 32 / 8);

    //memcpy(mod->pointer_cache[index].mask,
    //    pointer_new->colorPtrAttr.andMaskData, 32 * 32 / 8);

    mod->server_set_cursor(mod, mod->pointer_cache[index].hotx,
        mod->pointer_cache[index].hoty, mod->pointer_cache[index].data,
        mod->pointer_cache[index].mask);
  }
  else
  {
    LLOGLN(0, ("lfreerdp_pointer_new: error"));
  }

  xfree(pointer_new->colorPtrAttr.xorMaskData);
  pointer_new->colorPtrAttr.xorMaskData = 0;
  xfree(pointer_new->colorPtrAttr.andMaskData);
  pointer_new->colorPtrAttr.andMaskData = 0;

}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_pointer_cached(rdpContext* context,
                        POINTER_CACHED_UPDATE* pointer_cached)
{
  struct mod* mod;
  int index;

  LLOGLN(0, ("lfreerdp_pointer_cached:"));
  mod = ((struct mod_context*)context)->modi;
  index = pointer_cached->cacheIndex;
  mod->server_set_cursor(mod, mod->pointer_cache[index].hotx,
      mod->pointer_cache[index].hoty, mod->pointer_cache[index].data,
      mod->pointer_cache[index].mask);
}

/******************************************************************************/
static boolean DEFAULT_CC
lfreerdp_pre_connect(freerdp* instance)
{
  struct mod* mod;
  int index;
  int error;
  int num_chans;
  int ch_flags;
  char ch_name[256];
  char* dst_ch_name;

  LLOGLN(0, ("lfreerdp_pre_connect:"));
  mod = ((struct mod_context*)(instance->context))->modi;
  num_chans = 0;
  index = 0;
  error = mod->server_query_channel(mod, index, ch_name, &ch_flags);
  while (error == 0)
  {
    num_chans++;
    LLOGLN(10, ("lfreerdp_pre_connect: got channel [%s], flags [0x%8.8x]",
           ch_name, ch_flags));
    dst_ch_name = instance->settings->channels[index].name;
    g_memset(dst_ch_name, 0, 8);
    g_snprintf(dst_ch_name, 8, "%s", ch_name);
    instance->settings->channels[index].options = ch_flags;
    index++;
    error = mod->server_query_channel(mod, index, ch_name, &ch_flags);
  }
  instance->settings->num_channels = num_chans;

  instance->settings->offscreen_bitmap_cache = false;

  instance->settings->glyph_cache = true;
  instance->settings->glyphSupportLevel = GLYPH_SUPPORT_FULL;
  instance->settings->order_support[NEG_GLYPH_INDEX_INDEX] = true;
  instance->settings->order_support[NEG_FAST_GLYPH_INDEX] = false;
  instance->settings->order_support[NEG_FAST_INDEX_INDEX] = false;
  instance->settings->order_support[NEG_SCRBLT_INDEX] = true;
  instance->settings->order_support[NEG_SAVEBITMAP_INDEX] = false;

  instance->settings->bitmap_cache = true;
  instance->settings->order_support[NEG_MEMBLT_INDEX] = true;
  instance->settings->order_support[NEG_MEMBLT_V2_INDEX] = true;
  instance->settings->order_support[NEG_MEM3BLT_INDEX] = false;
  instance->settings->order_support[NEG_MEM3BLT_V2_INDEX] = false;
  instance->settings->bitmapCacheV2NumCells = 3; // 5;
  instance->settings->bitmapCacheV2CellInfo[0].numEntries = 0x78; // 600;
  instance->settings->bitmapCacheV2CellInfo[0].persistent = false;
  instance->settings->bitmapCacheV2CellInfo[1].numEntries = 0x78; // 600;
  instance->settings->bitmapCacheV2CellInfo[1].persistent = false;
  instance->settings->bitmapCacheV2CellInfo[2].numEntries = 0x150; // 2048;
  instance->settings->bitmapCacheV2CellInfo[2].persistent = false;
  instance->settings->bitmapCacheV2CellInfo[3].numEntries = 0; // 4096;
  instance->settings->bitmapCacheV2CellInfo[3].persistent = false;
  instance->settings->bitmapCacheV2CellInfo[4].numEntries = 0; // 2048;
  instance->settings->bitmapCacheV2CellInfo[4].persistent = false;

  instance->settings->order_support[NEG_MULTIDSTBLT_INDEX] = false;
  instance->settings->order_support[NEG_MULTIPATBLT_INDEX] = false;
  instance->settings->order_support[NEG_MULTISCRBLT_INDEX] = false;
  instance->settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = false;
  instance->settings->order_support[NEG_POLYLINE_INDEX] = false;

  // here
  //instance->settings->rdp_version = 4;

  instance->update->BeginPaint = lfreerdp_begin_paint;
  instance->update->EndPaint = lfreerdp_end_paint;
  instance->update->SetBounds = lfreerdp_set_bounds;
  instance->update->BitmapUpdate = lfreerdp_bitmap_update;

  instance->update->primary->DstBlt = lfreerdp_dst_blt;
  instance->update->primary->PatBlt = lfreerdp_pat_blt;
  instance->update->primary->ScrBlt = lfreerdp_scr_blt;
  instance->update->primary->OpaqueRect = lfreerdp_opaque_rect;
  instance->update->primary->MemBlt = lfreerdp_mem_blt;
  instance->update->primary->GlyphIndex = lfreerdp_glyph_index;
  instance->update->primary->LineTo = lfreerdp_line_to;

  instance->update->secondary->CacheBitmap = lfreerdp_cache_bitmap;
  instance->update->secondary->CacheBitmapV2 = lfreerdp_cache_bitmapV2;
  instance->update->secondary->CacheGlyph = lfreerdp_cache_glyph;
  instance->update->secondary->CacheBrush = lfreerdp_cache_brush;

  instance->update->pointer->PointerPosition = lfreerdp_pointer_position;
  instance->update->pointer->PointerSystem = lfreerdp_pointer_system;
  instance->update->pointer->PointerColor = lfreerdp_pointer_color;
  instance->update->pointer->PointerNew = lfreerdp_pointer_new;
  instance->update->pointer->PointerCached = lfreerdp_pointer_cached;

  return true;
}

/******************************************************************************/
static boolean  DEFAULT_CC
lfreerdp_post_connect(freerdp* instance)
{
  LLOGLN(0, ("lfreerdp_post_connect:"));
  return true;
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_context_new(freerdp* instance, rdpContext* context)
{
  LLOGLN(0, ("lfreerdp_context_new: %p", context));
}

/******************************************************************************/
static void DEFAULT_CC
lfreerdp_context_free(freerdp* instance, rdpContext* context)
{
  LLOGLN(0, ("lfreerdp_context_free:"));
}

/******************************************************************************/
static int DEFAULT_CC
lfreerdp_receive_channel_data(freerdp* instance, int channelId, uint8* data,
                              int size, int flags, int total_size)
{
  struct mod* mod;
  int lchid;
  int index;
  int error;

  mod = ((struct mod_context*)(instance->context))->modi;
  lchid = -1;
  for (index = 0; index < instance->settings->num_channels; index++)
  {
    if (instance->settings->channels[index].channel_id == channelId)
    {
      lchid = index;
      break;
    }
  }
  if (lchid >= 0)
  {
    LLOGLN(10, ("lfreerdp_receive_channel_data: server to client"));
    error = mod->server_send_to_channel(mod, lchid, data, size,
                                        total_size, flags);
    if (error != 0)
    {
      LLOGLN(0, ("lfreerdp_receive_channel_data: error %d", error));
    }
  }
  else
  {
    LLOGLN(0, ("lfreerdp_receive_channel_data: bad lchid"));
  }
  return 0;
}

/******************************************************************************/
struct mod* EXPORT_CC
mod_init(void)
{
  struct mod* mod;
  modContext* lcon;

  LLOGLN(0, ("mod_init:"));
  mod = (struct mod*)g_malloc(sizeof(struct mod), 1);
  freerdp_get_version(&(mod->vmaj), &(mod->vmin), &(mod->vrev));
  LLOGLN(0, ("  FreeRDP version major %d minor %d revision %d",
         mod->vmaj, mod->vmin, mod->vrev));
  mod->size = sizeof(struct mod);
  mod->version = CURRENT_MOD_VER;
  mod->handle = (tbus)mod;
  mod->mod_connect = lxrdp_connect;
  mod->mod_start = lxrdp_start;
  mod->mod_event = lxrdp_event;
  mod->mod_signal = lxrdp_signal;
  mod->mod_end = lxrdp_end;
  mod->mod_set_param = lxrdp_set_param;
  mod->mod_session_change = lxrdp_session_change;
  mod->mod_get_wait_objs = lxrdp_get_wait_objs;
  mod->mod_check_wait_objs = lxrdp_check_wait_objs;

  mod->inst = freerdp_new();
  mod->inst->PreConnect = lfreerdp_pre_connect;
  mod->inst->PostConnect = lfreerdp_post_connect;
  mod->inst->context_size = sizeof(modContext);
  mod->inst->ContextNew = lfreerdp_context_new;
  mod->inst->ContextFree = lfreerdp_context_free;
  mod->inst->ReceiveChannelData = lfreerdp_receive_channel_data;

  freerdp_context_new(mod->inst);

  lcon = (modContext*)(mod->inst->context);
  lcon->modi = mod;
  LLOGLN(10, ("mod_init: mod %p", mod));

  return mod;
}

/******************************************************************************/
int EXPORT_CC
mod_exit(struct mod* mod)
{
  LLOGLN(0, ("mod_exit:"));
  if (mod == 0)
  {
    return 0;
  }

  if ((mod->vmaj == 1) && (mod->vmin == 0) && (mod->vrev == 1))
  {
    /* this version has a bug with double free in freerdp_free */
  }
  else
  {
    freerdp_context_free(mod->inst);
  }
  freerdp_free(mod->inst);
  g_free(mod);
  return 0;
}
