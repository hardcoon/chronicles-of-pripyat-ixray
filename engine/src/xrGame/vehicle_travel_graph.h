#pragma once
// COP_ENGINE_PATCH: COP-ENG-240 - bounded, incremental automotive road search.
// No engine pointers: a cancelled trip owns and frees all search memory.
#include <vector>
#include <queue>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <limits>
#include <array>

namespace vehicle_travel
{
struct Vec3 {float x,y,z;};
inline bool triangle_hits_cylinder(const std::array<Vec3,3>& triangle,Vec3 center,float radius,float half_height)
{
    // Clip to the cylinder's vertical band, then test its circular footprint.
    // A diagonal wall outside the circle must not collide with a square's corner.
    std::array<Vec3,8> polygon{},output{};int count=3;
    for(int i=0;i<3;++i)polygon[i]={triangle[i].x-center.x,triangle[i].y-center.y,triangle[i].z-center.z};
    for(int plane=0;plane<2&&count;++plane)
    {
        const float limit=plane?half_height:-half_height;int written=0;
        Vec3 previous=polygon[count-1];bool previous_inside=plane?previous.y<=limit:previous.y>=limit;
        for(int i=0;i<count;++i)
        {
            Vec3 current=polygon[i];bool inside=plane?current.y<=limit:current.y>=limit;
            if(inside!=previous_inside)
            {
                float t=(limit-previous.y)/(current.y-previous.y);
                output[written++]={previous.x+(current.x-previous.x)*t,limit,previous.z+(current.z-previous.z)*t};
            }
            if(inside)output[written++]=current;
            previous=current;previous_inside=inside;
        }
        polygon=output;count=written;
    }
    if(!count)return false;
    const float radius_sqr=radius*radius;float area=0;bool positive=false,negative=false;
    for(int i=0;i<count;++i)
    {
        const Vec3 a=polygon[i],b=polygon[(i+1)%count];
        float x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
        float t=length>1e-12f?std::max(0.f,std::min(1.f,-(a.x*x+a.z*z)/length)):0.f;
        float px=a.x+x*t,pz=a.z+z*t;
        if(px*px+pz*pz<=radius_sqr)return true;
        float cross=a.x*b.z-a.z*b.x;area+=cross;
        positive|=cross>1e-5f;negative|=cross<-1e-5f;
    }
    return std::abs(area)>1e-5f&&!(positive&&negative);
}
inline uint32_t checksum(const void* data,size_t length)
{
    // Fixed IEEE CRC32. xrCore's CPU-specific CRC variants are not a file-format contract.
    static const std::array<uint32_t,256> table=[]{
        std::array<uint32_t,256> out{};
        for(uint32_t i=0;i<256;++i)
        {
            uint32_t c=i;for(int n=0;n<8;++n)c=(c>>1)^((c&1)?0xedb88320u:0u);out[i]=c;
        }
        return out;
    }();
    uint32_t c=~0u;const auto* bytes=static_cast<const uint8_t*>(data);
    for(size_t i=0;i<length;++i)c=(c>>8)^table[(c^bytes[i])&255];
    return ~c;
}
struct Cell { int16_t y; uint8_t clearance, flags; };
static_assert(sizeof(Cell)==4, "PTV1 cell layout");
struct Grid
{
    uint32_t w=0,h=0; float step=0,x0=0,zmax=0;
    std::vector<Cell> cells;
    int index(float x,float z) const
    {
        int c=int(std::floor((x-x0)/step)),r=int(std::floor((zmax-z)/step));
        return c>=0&&r>=0&&c<int(w)&&r<int(h) ? r*int(w)+c : -1;
    }
    float x(int i)const {return x0+(i%w+.5f)*step;}
    float z(int i)const {return zmax-(i/w+.5f)*step;}
    float y(int i)const {return cells[i].y*.01f;}
};
struct Cost
{
    uint32_t off=UINT32_MAX,total=UINT32_MAX;
    // COP_ENGINE_PATCH: COP-ENG-241 - roads preferred, finite dirt detours allowed.
    uint64_t score()const {return uint64_t(total)+uint64_t(off)*3;}
    bool operator<(const Cost& b)const {return score()<b.score()||(score()==b.score()&&total<b.total);}
    bool operator==(const Cost& b)const {return off==b.off&&total==b.total;}
};
struct Entry {Cost g; uint32_t estimate; int id;};
struct Greater
{
    bool operator()(const Entry& a,const Entry& b)const
    {
        uint64_t av=uint64_t(a.estimate)+uint64_t(a.g.off)*3;
        uint64_t bv=uint64_t(b.estimate)+uint64_t(b.g.off)*3;
        if(av!=bv)return av>bv;
        return a.id>b.id;
    }
};
class Search
{
public:
    Grid grid;
    std::vector<int> goals,path;
    std::vector<uint8_t> blocked,goal_mask;
    std::vector<Cost> cost;
    std::vector<int> parent;
    std::priority_queue<Entry,std::vector<Entry>,Greater> open;
    float clearance=1, gx=0,gy=0,gz=0,radius=100;
    uint32_t expanded=0; int start=-1,found=-1,remaining_goals=0,preferred_goal=-1;
    bool exhausted=false;

    bool walk(int id)const
    {
        return id>=0&&id<int(grid.cells.size())&&!blocked[id]&&grid.cells[id].clearance*.25f>=clearance;
    }
    void reset(int source)
    {
        start=source;found=-1;preferred_goal=-1;expanded=0;remaining_goals=0;exhausted=false;path.clear();
        cost.assign(grid.cells.size(),Cost());parent.assign(grid.cells.size(),-1);
        goal_mask.assign(grid.cells.size(),0);
        for(int g:goals)if(walk(g)&&!goal_mask[g]){
            goal_mask[g]=1;++remaining_goals;
            if(better_parking(g,preferred_goal))preferred_goal=g;
        }
        open={};cost[start]={0,0};open.push({{0,0},heuristic(start),start});
    }
    bool finish()
    {
        if(found<0){exhausted=true;return true;}
        for(int p=found;p>=0;p=parent[p])path.push_back(p);
        std::reverse(path.begin(),path.end());return true;
    }
    bool better_parking(int a,int b)const
    {
        if(b<0)return true;
        auto distance=[&](int i){float x=grid.x(i)-gx,y=grid.y(i)-gy,z=grid.z(i)-gz;return x*x+y*y+z*z;};
        float da=distance(a),db=distance(b);
        if(std::abs(da-db)>.01f)return da<db;
        return cost[a]<cost[b];
    }
    uint32_t heuristic(int i)const
    {
        return uint32_t(std::max(0.f,std::hypot(grid.x(i)-gx,grid.z(i)-gz)-radius)/grid.step*1000);
    }
    // One expansion per call lets the owning native API enforce both count and wall-clock limits.
    bool advance()
    {
        while(!open.empty())
        {
            Entry at=open.top();open.pop();
            if(!(at.g==cost[at.id]))continue;
            int i=at.id;
            if(goal_mask[i])
            {
                goal_mask[i]=0;--remaining_goals;
                if(better_parking(i,found))found=i;
                if(i==preferred_goal||!remaining_goals)return finish();
            }
            if(++expanded>3500000)return finish();
            int r=i/grid.w,c=i%grid.w;
            for(int dr=-1;dr<=1;++dr)for(int dc=-1;dc<=1;++dc)
            {
                if(!dr&&!dc)continue;
                int rr=r+dr,cc=c+dc;
                if(rr<0||cc<0||rr>=int(grid.h)||cc>=int(grid.w))continue;
                int j=rr*grid.w+cc;
                if(!walk(j)||std::abs(grid.y(j)-grid.y(i))>grid.step*2.f)continue;
                if(dr&&dc&&(!walk(r*grid.w+cc)||!walk(rr*grid.w+c)))continue;
                uint32_t length=dr&&dc?1414:1000;
                Cost next{at.g.off+((grid.cells[i].flags&grid.cells[j].flags&1)?0:length),at.g.total+length};
                if(next<cost[j])
                {
                    cost[j]=next;parent[j]=i;
                    open.push({next,next.total+heuristic(j),j});
                }
            }
            return false;
        }
        return finish();
    }
};
}
