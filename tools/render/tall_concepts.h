#pragma once
// Host-only design study. No device firmware includes this file.
// Inputs are the actual tile rectangle, look/density, existing fonts and selected controls.
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "../../components/smart_display/ui_scale.h"
#include "../../components/smart_display/theme.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
namespace tall_concepts {
inline std::string media_title="Weightless",media_artist="Marconi Union";
#if LV_USE_IMAGE
inline const lv_image_dsc_t *media_art=nullptr;
#endif
struct Fonts {const lv_font_t *name,*note,*headline,*value,*digits,*icon,*tile_icon;};
struct Board {const char *name,*look; int width,height,dpi,cols,rows,top,footer,margin,gx,gy; Fonts f;};
struct Rect {int x,y,w,h;};
inline lv_obj_t *box(lv_obj_t *parent,Rect r,theme::Role color,int radius=0) {
  assert(r.w>0 && r.h>0);
  auto *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(o,r.x,r.y);lv_obj_set_size(o,r.w,r.h);
  lv_obj_set_style_bg_color(o,theme::color(color),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
  lv_obj_set_style_radius(o,radius,0);return o;
}
inline lv_obj_t *text(lv_obj_t *p,const std::string &s,int x,int y,int width,const lv_font_t *f,theme::Role color=theme::INK,lv_text_align_t align=LV_TEXT_ALIGN_LEFT) {
  auto *o=lv_label_create(p);lv_obj_remove_style_all(o);lv_label_set_text(o,s.c_str());
  lv_obj_set_style_text_font(o,f,0);lv_obj_set_style_text_color(o,theme::color(color),0);
  lv_label_set_long_mode(o,LV_LABEL_LONG_DOT);lv_obj_set_style_text_align(o,align,0);
  lv_obj_set_pos(o,x,y);lv_obj_set_size(o,std::max(1,width),lv_font_get_line_height(f));return o;
}
inline void glyph(lv_obj_t *p,const char *s,Rect r,const Fonts &f,theme::Role color=theme::INK) {
 text(p,s,r.x,r.y+(r.h-lv_font_get_line_height(f.icon))/2,r.w,f.icon,color,LV_TEXT_ALIGN_CENTER);
}
// All layout decisions derive from available room and measured font heights.
// Fixed physical touch sizes win over optional artwork, secondary words and ornament.
struct Metrics {int pad,gap,w,h,key,head,controls_y; };
inline Metrics metrics(int width,int height,const Fonts &f,bool controls) {
 Metrics m{};m.pad=ui::px(ui::large()?12:8);m.gap=ui::px(ui::large()?8:4);
 m.w=width-2*m.pad;m.h=height-2*m.pad;m.key=std::max(ui::touch_min(),ui::px(40));
 m.head=std::max(lv_font_get_line_height(f.name),ui::px(ui::large()?54:28));
 m.controls_y=controls?m.h-m.key:m.h;
 return m;
}
// State colour is data, using the same Home Assistant palette as device cards.
inline void paint(lv_obj_t *o,uint32_t color) {lv_obj_set_style_bg_color(o,lv_color_hex(color),0);}
inline int text_width(const char *s,const lv_font_t *font) {
 lv_point_t size;lv_text_get_size(&size,s,font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);return size.x;
}
inline void state_glyph(lv_obj_t *p,const char *s,Rect r,const Fonts &f,uint32_t color) {
 auto *o=text(p,s,r.x,r.y+(r.h-lv_font_get_line_height(f.icon))/2,r.w,f.icon,theme::INK,LV_TEXT_ALIGN_CENTER);
 lv_obj_set_style_text_color(o,lv_color_hex(theme::icon(color)),0);
}
inline void album(lv_obj_t *p,Rect r,const Fonts &) {
 // An abstract cover fixture drawn by LVGL, not a fetched or decoded album image.
 #if LV_USE_IMAGE
 if(media_art){
  auto *frame=box(p,r,theme::CARD,ui::px(8));lv_obj_set_style_clip_corner(frame,true,0);
  auto *art=lv_image_create(frame);lv_obj_remove_style_all(art);lv_image_set_src(art,media_art);lv_obj_set_size(art,r.w,r.h);lv_image_set_inner_align(art,LV_IMAGE_ALIGN_COVER);
  lv_obj_set_style_radius(art,ui::px(8),0);return;
 }
#endif
 auto *a=box(p,r,theme::BUTTON,ui::px(14));
 paint(a,theme::surface(theme::swatch("blue")));
 const int inset=r.w/8;
 auto *field=box(a,{inset,inset,r.w-2*inset,r.h-2*inset},theme::ACCENT_TINT,ui::px(8));
 paint(field,theme::surface(theme::swatch("mint")));
 int size=r.w/2;
 auto *disc=box(field,{(r.w-2*inset-size)/2,(r.h-2*inset-size)/2,size,size},theme::ACCENT,LV_RADIUS_CIRCLE);
 paint(disc,theme::foreground(theme::ha::TEAL));
 box(disc,{size/3,size/3,size/3,size/3},theme::ACCENT_TINT,LV_RADIUS_CIRCLE);
}
inline void round_key(lv_obj_t *p,Rect r,const char *s,const Fonts &f,uint32_t color,bool primary=false,bool naked=false,bool photo=false) {
 assert(r.w>=ui::touch_min() && r.h>=ui::touch_min());
 auto *o=box(p,r,theme::TRACK,LV_RADIUS_CIRCLE);
 if(naked)lv_obj_set_style_bg_opa(o,LV_OPA_TRANSP,0);
 else if(primary)paint(o,theme::state(color));
 if(photo){if(primary)paint(o,theme::hex(theme::CAMERA_INK));glyph(o,s,{0,0,r.w,r.h},f,primary?theme::CAMERA_PAGE:theme::CAMERA_INK);}
 else if(primary)glyph(o,s,{0,0,r.w,r.h},f,theme::ON_ACCENT);else glyph(o,s,{0,0,r.w,r.h},f);
}
inline void controls(lv_obj_t *p,const Metrics&m,const Fonts&f,const std::string&kind,bool tonal,bool photo=false) {
 const int y=m.controls_y;
 if(kind=="playback"||kind=="vacuum"||kind=="modes") {
  if(m.w<3*ui::touch_min()+2*m.gap) {text(p,"Open controls",0,y,m.w,f.note);return;}
  const int k=std::min(m.key,(m.w-2*m.gap)/3);
  const int width=3*k+2*m.gap,x=(m.w-width)/2;
  const char *a=kind=="modes"?"\U000F0425":kind=="vacuum"?"\U000F040A":"\U000F04AE";
  const char *b=kind=="modes"?"\U000F0717":kind=="vacuum"?"\U000F04DB":"\U000F03E4";
  const char *c=kind=="modes"?"\U000F0238":kind=="vacuum"?"\U000F05F8":"\U000F04AD";
  const uint32_t color=kind=="modes"?theme::ha::DEEP_ORANGE:theme::ha::LIGHT_BLUE;
  round_key(p,{x,y,k,m.key},a,f,color,false,photo,photo);
  round_key(p,{x+k+m.gap,y,k,m.key},b,f,color,kind=="playback",false,photo);
  round_key(p,{x+2*(k+m.gap),y,k,m.key},c,f,color,kind=="modes",photo,photo);
 } else if(kind=="setpoint") {
  auto *pill=box(p,{0,y,m.w,m.key},theme::TRACK,ui::px(18));
  if(tonal)paint(pill,theme::tint(theme::ha::DEEP_ORANGE,24));
  glyph(pill,"\U000F0374",{0,0,m.key,m.key},f);glyph(pill,"\U000F0415",{m.w-m.key,0,m.key,m.key},f);
  text(pill,"21.5°",m.key,(m.key-lv_font_get_line_height(f.name))/2,m.w-2*m.key,f.name,theme::INK,LV_TEXT_ALIGN_CENTER);
 } else if(kind=="brightness"||kind=="volume") {
  int track_w=m.w;
  if(kind=="volume"){track_w-=m.key+m.gap;round_key(p,{track_w+m.gap,y,m.key,m.key},"\U000F057E",f,theme::ha::LIGHT_BLUE);}
  const uint32_t color=kind=="brightness"?theme::ha::ORANGE:theme::ha::LIGHT_BLUE;
  auto *track=box(p,{0,y,track_w,m.key},theme::TRACK,ui::px(16));paint(track,theme::tint(color,45));
  auto *fill=box(track,{0,0,track_w*65/100,m.key},theme::ACCENT,ui::px(16));paint(fill,theme::state(color));
  box(fill,{track_w*65/100-ui::px(10),m.key/4,ui::px(4),m.key/2},theme::ON_ACCENT,ui::px(2));
 }
}
inline void card(lv_obj_t *root,Rect r,const Fonts &f,const std::string&kind,const std::string&chosen,int variant,bool tall=true) {
 const bool enabled=!chosen.empty();auto m=metrics(r.w,r.h,f,enabled);
 bool photo=false;
#if LV_USE_IMAGE
 photo=variant && kind=="media" && media_art;
#endif
 const uint32_t color=kind=="climate"?theme::ha::DEEP_ORANGE:kind=="light"?theme::ha::ORANGE:theme::ha::LIGHT_BLUE;
 const uint32_t surface=theme::hex(theme::CARD);
 auto *outer=box(root,r,theme::CARD,ui::px(ui::large()?22:18));paint(outer,surface);
 if(!photo){lv_obj_set_style_border_width(outer,1,0);lv_obj_set_style_border_color(outer,theme::color(theme::LINE),0);}
 #if LV_USE_IMAGE
 if(photo){
  // Host-only clipping; device acceptance must use its existing pre-rounded image path.
  lv_obj_set_style_clip_corner(outer,true,0);
  auto *art=lv_image_create(outer);lv_obj_remove_style_all(art);lv_image_set_src(art,media_art);
  lv_obj_set_size(art,r.w,r.h);lv_image_set_inner_align(art,LV_IMAGE_ALIGN_COVER);
  lv_obj_set_style_radius(art,ui::px(ui::large()?22:18),0);
  auto *shade=box(outer,{0,0,r.w,r.h},theme::CAMERA_PAGE,ui::px(ui::large()?22:18));
  lv_obj_set_style_bg_opa(shade,170,0);
 }
#endif
 auto *p=box(outer,{m.pad,m.pad,m.w,m.h},theme::CARD);lv_obj_set_style_bg_opa(p,LV_OPA_TRANSP,0);
 const char *icon=kind=="media"?"\U000F075A":kind=="climate"?"\U000F050F":kind=="light"?"\U000F0335":"\U000F070D";
 const char *name=kind=="media"?"Living room":kind=="climate"?"Climate":kind=="light"?"Lights":"Vacuum";
 const int head=m.head;Fonts badge_fonts=f;badge_fonts.icon=f.tile_icon;
 auto *badge=box(p,{0,0,head,head},theme::TRACK,LV_RADIUS_CIRCLE);paint(badge,theme::tint(color,40));
 if(photo){paint(badge,theme::hex(theme::CAMERA_INK));lv_obj_set_style_bg_opa(badge,35,0);glyph(badge,icon,{0,0,head,head},badge_fonts,theme::CAMERA_INK);}
 else state_glyph(badge,icon,{0,0,head,head},badge_fonts,color);
 const int title_h=lv_font_get_line_height(f.name),note_h=lv_font_get_line_height(f.note);
 const bool header_note=head>=title_h+note_h;
 const int header_y=(head-title_h-(header_note?note_h:0))/2;
 text(p,name,head+m.gap,header_y,m.w-head-m.gap,f.name,photo?theme::CAMERA_INK:theme::INK);
 if(header_note)text(p,kind=="media"?"Playing":kind=="climate"?"Heating":kind=="light"?"On":"Docked",
   head+m.gap,header_y+title_h,m.w-head-m.gap,f.note,photo?theme::CAMERA_INK:theme::MUTED);
 const int top=head+m.gap,room=m.controls_y-top-(enabled?m.gap:0);
 if(kind=="media") {
  const lv_font_t *title_font=f.headline;
  if(lv_font_get_line_height(title_font)>room)title_font=f.name;
  if(lv_font_get_line_height(title_font)>room)title_font=f.note;
  const int title_h=lv_font_get_line_height(title_font);
  const bool artist=room>=title_h+lv_font_get_line_height(f.note)+m.gap/2;
  const int block=title_h+(artist?lv_font_get_line_height(f.note)+m.gap/2:0);
  int cover=!photo && m.w>=ui::px(300) && room>=ui::px(64)?std::min(room,ui::px(100)):0;
  int tw=m.w-(cover?cover+2*m.gap:0);
  if(cover)album(p,{m.w-cover,top+(room-cover)/2,cover,cover},f);
  int y=top+std::max(0,(room-block)/2);
  text(p,media_title,0,y,tw,title_font,photo?theme::CAMERA_INK:theme::INK);
  if(artist)text(p,media_artist,0,y+title_h+m.gap/2,tw,f.note,photo?theme::CAMERA_INK:theme::MUTED);
 } else if(kind=="climate") {
  if(!variant && chosen=="setpoint") {
   // Put the selected controls around the target. Measure the remaining label
   // width before choosing a font; never steal room from physical touch targets.
   const int bottom=lv_font_get_line_height(f.note),available=m.h-top-bottom-m.gap;
   const int center_w=m.w-2*m.key-2*m.gap;
   const lv_font_t *number=f.name;
   for(auto *candidate:{f.digits,f.value,f.headline,f.name})
    if(text_width("21.5°",candidate)<=center_w && lv_font_get_line_height(candidate)<=available){number=candidate;break;}
   const int nh=lv_font_get_line_height(number),row=std::max(m.key,nh),y=top+std::max(0,(available-row)/2);
   round_key(p,{0,y+(row-m.key)/2,m.key,m.key},"\U000F0374",f,color,false,false);
   round_key(p,{m.w-m.key,y+(row-m.key)/2,m.key,m.key},"\U000F0415",f,color,false,false);
   text(p,"21.5°",m.key+m.gap,y+(row-nh)/2,center_w,number,theme::INK,LV_TEXT_ALIGN_CENTER);
   text(p,"Now 20.8°",0,m.h-bottom,m.w,f.note,theme::MUTED,LV_TEXT_ALIGN_CENTER);
   return;
  }
  const lv_font_t *number=room>=lv_font_get_line_height(f.value)+lv_font_get_line_height(f.note)+m.gap?f.value:f.headline;
  if(lv_font_get_line_height(number)>room)number=f.note;
  int h=lv_font_get_line_height(number),nh=lv_font_get_line_height(f.note);
  bool note=room>=h+nh+m.gap/2;
  int y=top+std::max(0,(room-h-(note?nh+m.gap/2:0))/2);
  text(p,"20.8°",0,y,m.w,number,theme::INK,LV_TEXT_ALIGN_CENTER);
  if(note)text(p,"Room temperature",0,y+h+m.gap/2,m.w,f.note,theme::MUTED,LV_TEXT_ALIGN_CENTER);
 } else {
  const char *value=kind=="light"?"65%":"Docked";
  const auto *font=room>=lv_font_get_line_height(f.value)?f.value:room>=lv_font_get_line_height(f.headline)?f.headline:f.note;
  int h=lv_font_get_line_height(font),y=top+std::max(0,(room-h)/2);
  text(p,value,0,y,m.w,font,theme::INK,LV_TEXT_ALIGN_CENTER);
 }
 if(enabled)controls(p,m,f,chosen,variant==1,photo);
}
inline int check_bounds(lv_obj_t *parent) {
 int count=0;lv_area_t p;lv_obj_get_coords(parent,&p);
 for(unsigned i=0;i<lv_obj_get_child_count(parent);++i){auto *o=lv_obj_get_child(parent,i);lv_area_t r;lv_obj_get_coords(o,&r);
  if(r.x1<p.x1 || r.y1<p.y1 || r.x2>p.x2 || r.y2>p.y2){fprintf(stderr,"Overflow parent %d,%d..%d,%d child %d,%d..%d,%d\n",p.x1,p.y1,p.x2,p.y2,r.x1,r.y1,r.x2,r.y2);abort();}
  count+=1+check_bounds(o);
 }return count;
}
inline void save(lv_obj_t *root,const std::string&path) {
 lv_obj_update_layout(root);int checked=check_bounds(root);printf("BOUNDS PASS %d %s\n",checked,path.c_str());auto *buf=lv_snapshot_take(root,LV_COLOR_FORMAT_RGB888);assert(buf);
 FILE *fp=fopen(path.c_str(),"wb");assert(fp);fprintf(fp,"P6\n%d %d\n255\n",int(buf->header.w),int(buf->header.h));
 for(int y=0;y<int(buf->header.h);++y)for(int x=0;x<int(buf->header.w);++x){const auto *p=buf->data+y*buf->header.stride+3*x;uint8_t rgb[]={p[2],p[1],p[0]};fwrite(rgb,1,3,fp);}fclose(fp);lv_draw_buf_destroy(buf);
}
inline void render(const Board&b,const std::string&out,int variant,const std::string&scene,bool dark=false) {
 ui::configure(b.dpi,b.look);theme::set_dark(dark);
 auto *root=box(lv_screen_active(),{0,0,b.width,b.height},theme::PAGE);
 text(root,"Living room",b.margin,(b.top-lv_font_get_line_height(b.f.headline))/2,b.width-2*b.margin,b.f.headline);
 int cw=(b.width-2*b.margin-(b.cols-1)*b.gx)/b.cols;
 int ch=(b.height-b.top-b.footer-(b.rows-1)*b.gy)/b.rows;
 bool narrow=scene=="tall";int span=narrow?1:std::min(2,b.cols);
 Rect main{b.margin,b.top,span*cw+(span-1)*b.gx,2*ch+b.gy};
 std::string kind=scene.substr(0,scene.find('-'));
 std::string chosen=scene.find("none")!=std::string::npos?"":scene=="media-volume"?"volume":scene=="climate-modes"?"modes":kind=="media"?"playback":kind=="climate"?"setpoint":kind=="light"?"brightness":kind=="vacuum"?"vacuum":"";
 if(narrow){
  card(root,main,b.f,"media","playback",variant);
  if(b.cols>=2){main.x+=cw+b.gx;card(root,main,b.f,"climate","setpoint",variant);}
 }else card(root,main,b.f,kind,chosen,variant);
 // Ordinary neighbours keep their original grid footprint, never grow to fill unused cells.
 for(int row=0;row<b.rows;++row)for(int col=0;col<b.cols;++col) {
  if(row<2 && col<(narrow?2:span))continue;
  auto *o=box(root,{b.margin+col*(cw+b.gx),b.top+row*(ch+b.gy),cw,ch},theme::CARD,ui::px(ui::large()?20:14));
  text(o,(row+col)%2?"Kitchen":"Outside",ui::px(10),std::max(0,(ch-2*lv_font_get_line_height(b.f.note))/2),cw-ui::px(20),b.f.name);
  text(o,(row+col)%2?"Off":"18.2 °C",ui::px(10),ch/2,cw-ui::px(20),b.f.note,theme::MUTED);
 }
 int dot=ui::px(6),gap=ui::px(10),y=b.height-b.footer/2;
 for(int i=0;i<3;++i)box(root,{b.width/2+(i-1)*(dot+gap)-dot/2,y-dot/2,dot,dot},i==0?theme::ACCENT:theme::TICK,LV_RADIUS_CIRCLE);
 save(root,out+"/"+b.name+"-"+(variant?"B":"A")+"-"+scene+(dark?"-dark":"")+".ppm");lv_obj_delete(root);
}
}
