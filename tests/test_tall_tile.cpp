#include "../components/smart_display/tall_tile.h"
#include <cassert>
#include <cstdio>
using namespace tall_tile;
static bool inside(Rect r,int w,int h){return r.x>=0&&r.y>=0&&r.right()<=w&&r.bottom()<=h;}
int main(){
  // Physical density, available area and selected group count vary independently.
  // This catches negative body space, overlap and touch shrinkage, rather than
  // pinning coordinates to one board or repeating the layout implementation.
  unsigned checked=0;
  for(int touch:{28,39,47,60,84})for(int width=touch*2;width<900;width+=17)
  for(int height=20;height<650;height+=19)for(int groups=0;groups<=2;++groups){
    Metrics m{width,height,16,14,54,8,touch,400,{touch,touch},groups};
    auto l=layout(m);
    if(!l.fits){assert(height<16+groups*(touch+8));continue;}
    assert(inside(l.header,width,height));assert(l.header.h>=16);
    assert(inside(l.body,width,height));assert(l.body.y>=l.header.bottom());
    int end=l.body.bottom();
    for(int i=0;i<groups;++i){
      auto row=l.rows[i];assert(inside(row,width,height));assert(row.h>=touch);assert(row.y>=end);end=row.bottom();
      auto buttons=keys(row,3,touch+12,touch,8);
      if(buttons[0].empty()){assert(row.w<3*touch+16);continue;}
      for(int j=0;j<3;++j){assert(inside(buttons[j],width,height));assert(buttons[j].w>=touch&&buttons[j].h>=touch);if(j)assert(buttons[j-1].right()<=buttons[j].x);}
    }
    if(groups)assert(end==height);
    ++checked;
  }
  // A CYD-sized cell accepts one row, but cannot promise two physical rows.
  Metrics compact{131,92,13,13,28,4,39,280,{39,39},1};
  assert(layout(compact).fits);compact.row_count=2;assert(!layout(compact).fits);
  Metrics spacious{424,204,21,19,54,8,47,400,{47,47},2};
  assert(layout(spacious).fits);
  assert(fits_text({0,0,80,40},79,39));assert(!fits_text({0,0,80,40},81,39));
  std::printf("tall tile geometry: %u valid arrangements checked\n",checked);
}
