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
struct Fonts {const lv_font_t *name,*note,*headline,*value,*digits,*icon;};
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
inline void key(lv_obj_t *p,Rect r,const char *s,const Fonts &f,bool active=false) {
 assert(r.w>=ui::touch_min() && r.h>=ui::touch_min());
 auto *o=box(p,r,active?theme::ACCENT_TINT:theme::TRACK,ui::px(12));
 glyph(o,s,{0,0,r.w,r.h},f,active?theme::ACCENT:theme::INK);
}
// All layout decisions derive from available room and measured font heights.
// Fixed physical touch sizes win over optional artwork, secondary words and ornament.
struct Metrics {int pad,gap,w,h,key,head,controls_y; bool artist,art;};
inline Metrics metrics(int width,int height,const Fonts &f,bool controls) {
 Metrics m{};m.pad=ui::px(ui::large()?12:8);m.gap=ui::px(ui::large()?8:4);
 m.w=width-2*m.pad;m.h=height-2*m.pad;m.key=std::max(ui::touch_min(),ui::px(44));
 m.head=std::max(lv_font_get_line_height(f.name),ui::px(ui::large()?30:18));
 m.controls_y=controls?m.h-m.key:m.h;
 m.artist=m.controls_y-m.head-2*m.gap>=lv_font_get_line_height(f.headline)+lv_font_get_line_height(f.note);
 m.art=m.w>=ui::px(280) && m.controls_y>=ui::px(64);
 return m;
}
inline void album(lv_obj_t *p,Rect r,const Fonts &f) {
 // Deliberately illustrative cover placeholder: no image decoder/cache is claimed here.
 auto *a=box(p,r,theme::BUTTON,ui::px(12));
 auto *disc=box(a,{r.w/5,r.h/5,r.w*3/5,r.h*3/5},theme::ACCENT,LV_RADIUS_CIRCLE);
 const int size=r.w/5;box(disc,{(r.w*3/5-size)/2,(r.h*3/5-size)/2,size,size},theme::ACCENT_TINT,LV_RADIUS_CIRCLE);
}
inline void controls(lv_obj_t *p,const Metrics&m,const Fonts&f,const std::string&kind,bool bold) {
 int y=m.controls_y;
 if(kind=="playback"||kind=="vacuum"||kind=="modes") {
  if(m.w<3*ui::touch_min()+2*m.gap) {text(p,"Open controls",0,y,m.w,f.note);return;}
  int w=(m.w-2*m.gap)/3;
  const char *a=kind=="modes"?"\U000F0425":kind=="vacuum"?"\U000F040A":"\U000F04AE";
  const char *b=kind=="modes"?"\U000F0717":kind=="vacuum"?"\U000F04DB":"\U000F03E4";
  const char *c=kind=="modes"?"\U000F0238":kind=="vacuum"?"\U000F05F8":"\U000F04AD";
  key(p,{0,y,w,m.key},a,f);key(p,{w+m.gap,y,w,m.key},b,f,bold && kind!="modes");key(p,{2*(w+m.gap),y,m.w-2*(w+m.gap),m.key},c,f,kind=="modes");
 } else if(kind=="setpoint") {
  auto *pill=box(p,{0,y,m.w,m.key},theme::TRACK,ui::px(12));
  glyph(pill,"\U000F0374",{0,0,m.key,m.key},f);glyph(pill,"\U000F0415",{m.w-m.key,0,m.key,m.key},f);
  text(pill,"21.5°",m.key,(m.key-lv_font_get_line_height(f.name))/2,m.w-2*m.key,f.name,theme::INK,LV_TEXT_ALIGN_CENTER);
 } else if(kind=="brightness"||kind=="volume") {
  int track_w=m.w;
  if(kind=="volume"){track_w-=m.key+m.gap;key(p,{track_w+m.gap,y,m.key,m.key},"\U000F057E",f);}
  auto *track=box(p,{0,y,track_w,m.key},kind=="brightness"?theme::AMBER_TRACK:theme::ACCENT_TINT,ui::px(12));
  auto *fill=box(track,{0,0,track_w*65/100,m.key},kind=="brightness"?theme::SUN_PATH:theme::ACCENT,ui::px(12));
  box(fill,{track_w*65/100-ui::px(10),m.key/4,ui::px(4),m.key/2},theme::ON_ACCENT,ui::px(2));
 }
}
inline void card(lv_obj_t *root,Rect r,const Fonts &f,const std::string&kind,const std::string&chosen,int variant,bool tall=true) {
 auto *outer=box(root,r,theme::CARD,ui::px(ui::large()?20:14));
 lv_obj_set_style_border_width(outer,1,0);lv_obj_set_style_border_color(outer,theme::color(theme::LINE),0);
 const bool enabled=!chosen.empty();auto m=metrics(r.w,r.h,f,enabled);
 auto *p=box(outer,{m.pad,m.pad,m.w,m.h},theme::CARD);
 const char *icon=kind=="media"?"\U000F075A":kind=="climate"?"\U000F050F":kind=="light"?"\U000F0335":"\U000F070D";
 const char *name=kind=="media"?"Living room":kind=="climate"?"Climate":kind=="light"?"Lights":"Vacuum";
 int cover=(kind=="media" && variant==1 && m.art)?std::min(m.controls_y-m.gap,ui::px(100)):0;
 if(cover)album(p,{0,0,cover,cover},f);
 int circle=m.head;
 auto *badge=box(p,{cover?cover+m.gap:0,0,circle,circle},kind=="light"?theme::AMBER_TRACK:theme::TRACK,LV_RADIUS_CIRCLE);
 glyph(badge,icon,{0,0,circle,circle},f,kind=="light"?theme::SUN_PATH:theme::SLATE);
 text(p,name,(cover?cover+m.gap:0)+circle+m.gap,(circle-lv_font_get_line_height(f.name))/2,m.w-(cover?cover+m.gap:0)-circle-m.gap,f.name);
 int top=m.head+m.gap,room=m.controls_y-top-(enabled?m.gap:0);
 if(kind=="media") {
  int tx=0,tw=m.w;
  if(cover) {tx=cover+m.gap;tw=m.w-tx;}
  int block=lv_font_get_line_height(f.headline)+(m.artist?lv_font_get_line_height(f.note)+m.gap/2:0);
  int y=top+std::max(0,(room-block)/2);
  text(p,"Weightless",tx,y,tw,f.headline);
  if(m.artist)text(p,"Marconi Union",tx,y+lv_font_get_line_height(f.headline)+m.gap/2,tw,f.note,theme::MUTED);
 } else if(kind=="climate") {
  const lv_font_t *number=room>=lv_font_get_line_height(f.value)?f.value:f.headline;
  bool hero=variant==1 && room>=lv_font_get_line_height(f.digits)+lv_font_get_line_height(f.note)+m.gap;
  if(hero)number=f.digits;
  int h=lv_font_get_line_height(number),nh=lv_font_get_line_height(f.note);
  bool note=room>=h+nh+m.gap/2;
  int y=top+std::max(0,(room-h-(note?nh+m.gap/2:0))/2);
  text(p,hero?"21.5°":"20.8°",0,y,m.w,number,theme::INK,LV_TEXT_ALIGN_CENTER);
  if(note)text(p,hero?"Room 20.8° · heating":"Room · heating",0,y+h+m.gap/2,m.w,f.note,theme::MUTED,LV_TEXT_ALIGN_CENTER);
  // B distributes the SAME selected setpoint controls around the target, no mode row added.
  if(hero && chosen=="setpoint") {
   int w=(m.w-m.gap)/2;key(p,{0,m.controls_y,w,m.key},"\U000F0374",f);key(p,{w+m.gap,m.controls_y,m.w-w-m.gap,m.key},"\U000F0415",f);
   return;
  }
 } else {
  const char *value=kind=="light"?"65%":"Docked";
  const auto *font=room>=lv_font_get_line_height(f.value)?f.value:f.headline;
  int h=lv_font_get_line_height(font),y=top+std::max(0,(room-h)/2);
  text(p,value,0,y,m.w,font,theme::INK,LV_TEXT_ALIGN_CENTER);
 }
 if(enabled)controls(p,m,f,chosen,variant==1);
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
