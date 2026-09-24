#pragma once
// Geometry for additional tile rows. Single-row tiles retain their existing path.
// All dimensions are supplied by the caller, measured from the active look/fonts.
// No entity data, drawing objects or image buffers are owned by this module.
#include <algorithm>
#include <array>

namespace tall_tile {
struct Rect {
  int x=0,y=0,w=0,h=0;
  int right() const {return x+w;}
  int bottom() const {return y+h;}
  bool empty() const {return w<=0||h<=0;}
};
struct Metrics {
  int width=0,height=0;
  int name_h=0,state_h=0,icon=0,gap=0,touch=0,reach=0;
  // Rows already selected by the user, not suggested or automatically enabled.
  std::array<int,2> row_heights{};
  int row_count=0;
};
struct Layout {
  bool fits=false,state=false;
  Rect header,body;
  std::array<Rect,2> rows{};
  int count=0;
};
inline Layout layout(const Metrics &m) {
  Layout out;
  if(m.width<=0||m.height<=0||m.name_h<=0||m.touch<=0||m.gap<0||m.row_count<0||m.row_count>2)return out;
  const int count=m.row_count;
  int controls=0;
  for(int i=0;i<count;++i)controls+=std::max(m.touch,m.row_heights[i]);
  if(count)controls+=m.gap*count;
  // Optional state/ornament may give way; selected touch controls never shrink.
  const int room=m.height-controls;
  if(room<m.name_h)return out;
  out.state=room>=m.name_h+m.state_h;
  const int lines=m.name_h+(out.state?m.state_h:0);
  const int heading=std::min(room,std::max(lines,std::min(m.icon,m.height/3)));
  out.header={0,0,m.width,heading};
  const int body_top=std::min(room,heading+m.gap);
  out.body={0,body_top,m.width,std::max(0,room-body_top)};
  int y=m.height-controls;
  const int width=m.reach>0?std::min(m.width,m.reach):m.width;
  for(int i=0;i<count;++i){
    y+=m.gap;
    const int h=std::max(m.touch,m.row_heights[i]);
    out.rows[i]={(m.width-width)/2,y,width,h};y+=h;
  }
  out.count=count;out.fits=true;return out;
}
// A group stays centred within hand reach. An impossible group is explicitly
// empty, allowing the editor/renderer to keep the detail view as the fallback.
inline std::array<Rect,3> keys(Rect row,int count,int desired,int touch,int gap) {
  std::array<Rect,3> result{};
  if(count<1||count>3||touch<=0||gap<0||row.h<touch||row.w<count*touch+(count-1)*gap)return result;
  const int side=std::max(touch,std::min({desired,row.h,(row.w-(count-1)*gap)/count}));
  const int width=count*side+(count-1)*gap;
  for(int i=0;i<count;++i)result[i]={row.x+(row.w-width)/2+i*(side+gap),row.y+(row.h-side)/2,side,side};
  return result;
}
// A toggle needs visible travel, not a circle. Keep the touch height while
// fitting the width, or explicitly fall back to the tile's detail action.
inline Rect toggle(int width,int touch) {
  if(touch<=0||width<touch+touch/2)return {};
  return {0,0,std::min(width,2*touch),touch};
}
struct ActionLayout { bool fits=false; Rect icon,title,state; };
// Built-in action tiles have no extra controls. Centre their measured stack,
// giving up the optional subtitle before reducing the icon below its glyph.
inline ActionLayout action(int width,int height,int icon,int glyph,int name,int state,int gap) {
  if(width<=0||height<=0||icon<=0||glyph<=0||name<=0||gap<0)return {};
  if(height<glyph+gap+name+state)state=0;
  const int side=std::min({icon,width,height-gap-name-state});
  if(side<glyph)return {};
  const int y=(height-side-gap-name-state)/2;
  return {true,{(width-side)/2,y,side,side},{0,y+side+gap,width,name},
          {0,y+side+gap+name,width,state}};
}
// Font selection also uses measured width. A large display with a dense grid can
// have less room than a small display with two columns.
inline bool fits_text(Rect area,int width,int line_height) {
  return width>0&&line_height>0&&width<=area.w&&line_height<=area.h;
}
}  // namespace tall_tile
