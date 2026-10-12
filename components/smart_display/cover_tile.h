#pragma once
// Optional cover controls use measured space and physical touch sizes. Returning
// no layout leaves the primary controls intact and tilt in the detail view.
#include "tall_tile.h"
namespace cover_tile {
using tall_tile::Rect;
struct Group { Rect area, control, caption; int keys=0; bool slider=false,horizontal=false; };
struct Layout { bool fits=false; std::array<Group,2> groups{}; };
inline Layout layout(Rect body,int touch,int gap,int caption,int slider_min,
                     bool position,int primary_keys,bool tilt,int tilt_keys) {
  Layout out;
  if(touch<=0||gap<0||caption<=0||(!tilt&&!tilt_keys))return out;
  const bool primary=position||primary_keys;
  const int columns=primary?2:1, column_gap=2*gap;
  const int control_h=body.h-caption-gap;
  if(control_h<touch)return out;
  if((position||tilt)&&control_h<slider_min)return out;
  std::array<int,2> widths{};std::array<bool,2> horizontal{};
  const int preferred=std::min((body.w-column_gap*(columns-1))/columns,2*touch);
  for(int i=0;i<2;++i){
    if(i==0&&!primary)continue;
    const int n=i?tilt_keys:primary_keys;
    horizontal[i]=n&&control_h<n*touch+(n-1)*gap;
    widths[i]=horizontal[i]?n*touch+(n-1)*gap:preferred;
    if(widths[i]<touch)return out;
  }
  const int total=widths[0]+widths[1]+(primary?column_gap:0);
  if(total>body.w)return out;
  int x=body.x+(body.w-total)/2;
  for(int i=0;i<2;++i){
    if(i==0&&!primary)continue;
    auto &g=out.groups[i];g.slider=i?tilt:position;g.keys=g.slider?0:(i?tilt_keys:primary_keys);
    g.horizontal=horizontal[i];
    const int h=g.slider?control_h:g.horizontal?touch:g.keys*touch+(g.keys-1)*gap;
    g.area={x,body.y,widths[i],body.h};
    g.control={x,body.y+(control_h-h)/2,widths[i],h};
    g.caption={x,body.bottom()-caption,widths[i],caption};
    x+=widths[i]+column_gap;
  }
  out.fits=true;return out;
}
}
