#include "StdAfx.h"
#include "pch_script.h"
#include "Car.h"
#include "Actor.h"
#include "Level.h"
#include "ai_space.h"
#include "../xrScripts/script_engine.h"
#include "ai_object_location.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "game_graph.h"
#include "level_graph.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "vehicle_travel_graph.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xrEngine/GameMtlLib.h"
#include "../xrCore/Collision/Frustum.h"
#include <chrono>

// COP_ENGINE_PATCH: COP-ENG-240 - one bounded job owned by the actual occupied car.
struct VehicleTravelJob
{
    vehicle_travel::Search search;
    Fvector start,goal;
    u16 actor=0;
    std::vector<int> candidates;
    size_t candidate=0,validate=0;
    int phase=0,retries=0;
    bool obstructed=false;
    LPCSTR last_failure="";
    Fvector last_bad=Fvector().set(0,0,0);
    float yaw=0;
};
namespace
{
luabind::object table(){return luabind::newtable(ai().script_engine().lua());}
luabind::object status(LPCSTR text,LPCSTR detail=""){auto result=table();result["status"]=text;result["detail"]=detail;return result;}
luabind::object pending(const VehicleTravelJob* job)
{
    auto result=status("pending");result["phase"]=job->phase;result["expanded"]=job->search.expanded;
    result["candidate"]=job->candidate;result["validated"]=job->validate;result["retries"]=job->retries;
    result["goals"]=job->search.goals.size();result["last_failure"]=job->last_failure;
    result["bad_x"]=job->last_bad.x;result["bad_y"]=job->last_bad.y;result["bad_z"]=job->last_bad.z;return result;
}
bool ray(const Fvector& a,const Fvector& d,float range,collide::rq_result& hit,CCar* car)
{return !!Level().ObjectSpace.RayPick(a,d,range,collide::rqtStatic,hit,car);}
bool support(const Fvector& from,collide::rq_result& hit,CCar* car)
{
    Fvector start=from;float passed=0;
    for(int layer=0;layer<8&&passed<4.f;++layer)
    {
        if(!ray(start,Fvector().set(0,-1,0),4.f-passed,hit,car))return false;
        const auto* material=GMLib.GetMaterialByIdx(Level().ObjectSpace.GetStaticTris()[hit.element].material);
        if(material->Flags.test(SGameMtl::flLiquid))return false;
        if(!material->Flags.test(SGameMtl::flPassable)){hit.range+=passed;return true;}
        float skip=hit.range+.02f;start.y-=skip;passed+=skip;
    }
    return false;
}
bool solid_body(const Fvector& center,const Fmatrix& pose,const Fvector& size)
{
    // Same oriented volume as ObjectSpace::BoxQuery, retaining material identities
    // so passable grass/bush faces cannot turn into rigid car obstacles.
    Fplane planes[6];
    planes[0].build(Fvector().mad(center,pose.i,-size.x*.5f),Fvector().invert(pose.i));
    planes[1].build(Fvector().mad(center,pose.i,size.x*.5f),pose.i);
    planes[2].build(Fvector().mad(center,pose.j,size.y*.5f),pose.j);
    planes[3].build(Fvector().mad(center,pose.j,-size.y*.5f),Fvector().invert(pose.j));
    planes[4].build(Fvector().mad(center,pose.k,size.z*.5f),pose.k);
    planes[5].build(Fvector().mad(center,pose.k,-size.z*.5f),Fvector().invert(pose.k));
    CFrustum frustum;frustum.CreateFromPlanes(planes,6);
    static CDB::COLLIDER query;query.frustum_options(CDB::OPT_FULL_TEST);
    query.frustum_query(Level().ObjectSpace.GetStaticModel(),frustum);
    for(auto* result=query.r_begin();result!=query.r_end();++result)
    {
        const auto* material=GMLib.GetMaterialByIdx(Level().ObjectSpace.GetStaticTris()[result->id].material);
        if(material->Flags.test(SGameMtl::flPassable|SGameMtl::flLiquid))continue;
        return true;
    }
    return false;
}
// COP_ENGINE_PATCH: COP-ENG-241 - placement checks are only for actual stops.
u32 placement_vertex(const Fvector& floor)
{
    auto& graph=ai().level_graph();
    if(!graph.valid_vertex_position(floor))return u32(-1);
    u32 vertex=graph.vertex_id(floor);
    const float step=graph.header().cell_size();
    for(int radius=1;radius<=2&&!graph.valid_vertex_id(vertex);++radius)
        for(int x=-radius;x<=radius&&!graph.valid_vertex_id(vertex);++x)
            for(int z=-radius;z<=radius&&!graph.valid_vertex_id(vertex);++z)
            {
                Fvector p=floor;p.x+=x*step;p.z+=z*step;
                if(graph.valid_vertex_position(p))vertex=graph.vertex_id(p);
            }
    return vertex;
}
Fvector point(const vehicle_travel::Grid& grid,int i)
{return Fvector().set(grid.x(i),grid.y(i),grid.z(i));}
bool offline_clear(u16 car_id,const Fmatrix& pose,const Fbox& box)
{
    auto* simulator=ai().get_alife();if(!simulator)return false;
    Fmatrix inverse;inverse.invert(pose);
    const auto level_id=ai().level_graph().level_id();
    // This bounded registry query runs only for candidate parking/settlement, never per-frame travel.
    // The world is paused, so remote ALife blockers cannot materialize into the chosen car pose.
    for(const auto& row:simulator->objects().objects())
    {
        auto* object=row.second;
        if(!object||object->ID==car_id||object->m_bOnline||object->ID_Parent!=0xffff)continue;
        const Fvector& position=object->o_Position;
        if(std::abs(position.x-pose.c.x)>24.f||std::abs(position.z-pose.c.z)>24.f||std::abs(position.y-pose.c.y)>12.f)continue;
        if(!ai().game_graph().valid_vertex_id(object->m_tGraphID)||ai().game_graph().vertex(object->m_tGraphID)->level_id()!=level_id)continue;
        bool vehicle=smart_cast<CSE_ALifeCar*>(object)!=nullptr;
        if(!vehicle&&!smart_cast<CSE_ALifeCreatureAbstract*>(object))continue;
        float radius=vehicle?5.f:.75f;
        Fvector local=position;local.y+=vehicle?1.f:.8f;inverse.transform_tiny(local);
        if(local.x+radius>box.min.x-.15f&&local.x-radius<box.max.x+.15f
            &&local.z+radius>box.min.z-.15f&&local.z-radius<box.max.z+.15f
            &&local.y+(vehicle?2.f:1.f)>box.min.y&&local.y-(vehicle?2.f:1.f)<box.max.y)return false;
    }
    return true;
}
}

Fbox CCar::TravelBounds()
{
    if(!m_pPhysicsShell)return BoundingBox();
    Fmatrix form,inv;m_pPhysicsShell->GetGlobalTransformDynamic(&form);inv.invert(form);
    Fvector size,world_center,center;get_box(m_pPhysicsShell,form,size,world_center);inv.transform_tiny(center,world_center);
    Fbox box;box.min=Fvector().mad(center,size,-.5f);box.max=Fvector().mad(center,size,.5f);return box;
}
bool CCar::TravelPose(const Fvector& floor,float yaw,bool dynamic,Fmatrix& pose)
{
    if(!IsGameTypeSingle()||!m_pPhysicsShell||!g_Alive()||!_valid(floor)||!_valid(yaw))return false;
    const Fbox box=TravelBounds(); Fvector size;box.getsize(size);
    if(size.x<.5f||size.z<1.f||size.x>8.f||size.z>18.f||size.y>8.f)return false;
    pose.setHPB(yaw,0,0);pose.c.set(floor);
    float highest=-flt_max,lowest=flt_max;
    const float xs[]={box.min.x*.8f,0.f,box.max.x*.8f};
    const float zs[]={box.min.z*.8f,0.f,box.max.z*.8f};
    for(float x:xs)for(float z:zs)
    {
        Fvector sample;pose.transform_tiny(sample,Fvector().set(x,2.f,z));
        collide::rq_result hit;
        if(!support(sample,hit,this))return false;
        float y=sample.y-hit.range;
        if(std::abs(y-floor.y)>1.5f)return false;
        highest=std::max(highest,y);lowest=std::min(lowest,y);
    }
    if(highest-lowest>1.15f)return false;
    // A small support lift keeps tyres clear of the floor; gravity settles it normally.
    pose.c.y=highest-box.min.y+.08f;
    Fvector center;box.getcenter(center);
    // Wheels touch the floor. Test the entire body above the tyre contact band.
    float bottom=std::max(box.min.y+.40f,box.min.y+size.y*.16f);
    center.y=(bottom+box.max.y)*.5f;
    Fvector body=Fvector().set(size.x+.16f,box.max.y-bottom,size.z+.16f),world;
    pose.transform_tiny(world,center);
    if(solid_body(world,pose,body))return false;
    if(dynamic)
    {
        if(!offline_clear(ID(),pose,box))return false;
        xr_vector<CObject*> nearby;
        float radius=body.magnitude()*.5f+.2f;
        Level().ObjectSpace.GetNearest(nearby,world,radius,this);
        if(nearby.size()>128)return false;
        Fmatrix inv;inv.invert(pose);
        for(CObject* other:nearby)
        {
            if(!other||other==Owner()||other->H_Parent()||other->getDestroy())continue;
            CGameObject* obj=smart_cast<CGameObject*>(other);
            if(!obj||(!obj->cast_entity_alive()&&!obj->cast_physics_shell_holder()))continue;
            Fvector c;other->Center(c);inv.transform_tiny(c);
            float r=other->Radius();
            if(c.x+r>box.min.x-.15f&&c.x-r<box.max.x+.15f&&c.z+r>box.min.z-.15f&&c.z-r<box.max.z+.15f
                &&c.y+r>box.min.y&&c.y-r<box.max.y+.1f)return false;
        }
    }
    return true;
}
luabind::object CCar::TravelProbe(const Fvector& floor,float yaw)
{
    // COP_ENGINE_PATCH: COP-ENG-241 - nearest stop, fixed forward route heading.
    if(!_valid(floor)||!_valid(yaw))return luabind::object();
    Fmatrix pose;Fvector chosen=floor;
    bool found=ai().level_graph().valid_vertex_id(placement_vertex(chosen))&&TravelPose(chosen,yaw,true,pose);
    const float radii[]={2,4,6,8,12,16,24,32,48,64,96};
    for(float radius:radii)
    {
        if(found)break;
        for(int direction=0;direction<16&&!found;++direction)
        {
            float angle=direction*PI_MUL_2/16;
            Fvector p=floor;p.x+=std::sin(angle)*radius;p.z+=std::cos(angle)*radius;
            u32 vertex=placement_vertex(p);if(!ai().level_graph().valid_vertex_id(vertex))continue;
            chosen=ai().level_graph().vertex_position(vertex);
            found=TravelPose(chosen,yaw,true,pose);
        }
    }
    if(!found)return luabind::object();
    auto result=table();Fvector size;TravelBounds().getsize(size);
    result["position"]=pose.c;result["floor"]=chosen;result["yaw"]=yaw;
    result["width"]=size.x;result["length"]=size.z;result["height"]=size.y;
    return result;
}
void CCar::TravelRouteCancel(){delete m_travel_job;m_travel_job=nullptr;}
luabind::object CCar::TravelRouteBegin(const Fvector& goal,float radius)
{
    TravelRouteCancel();
    if(!IsMyCar()||!OwnerActor())return status("invalid_vehicle","not_driver");
    if(!g_Alive()||!_valid(goal)||radius<1.f||radius>100.f)return status("invalid_vehicle","invalid_target");
    if(!Device.Paused())return status("invalid_vehicle","requires_pause");
    // The facade checked that the driver was stationary before the short fade.
    // Remove any slope drift accumulated during that fade, after the world is frozen.
    Fvector zero=Fvector().set(0,0,0);
    m_pPhysicsShell->set_LinearVel(zero);m_pPhysicsShell->set_AngularVel(zero);
    ResetKeys();NeutralDrive();ReleaseHandBreak();
    string_path name;xr_sprintf(name,"pf_travel\\%s.nav",Level().name().c_str());
    IReader* file=FS.r_open("$game_config$",name);
    if(!file)return status("no_navigation","missing_nav_file");
    auto job=std::make_unique<VehicleTravelJob>();auto& grid=job->search.grid;
    bool valid=file->length()>=36;LPCSTR invalid_reason="nav_header";
    if(valid)
    {
        u32 magic=file->r_u32(),version=file->r_u32();grid.w=file->r_u32();grid.h=file->r_u32();
        grid.step=file->r_float();grid.x0=file->r_float();grid.zmax=file->r_float();
        u32 checksum=file->r_u32(),ai_bytes=file->r_u32();
        const size_t count=size_t(grid.w)*grid.h;
        valid=magic==0x31565450&&version==1&&grid.w>1&&grid.h>1&&grid.w<=4096&&grid.h<=4096
            &&grid.step>=1.f&&grid.step<=4.f&&file->length()==36+count*sizeof(vehicle_travel::Cell);
        if(valid)
        {
            invalid_reason="ai_mesh_size";
            IReader* source=FS.r_open("$level$","level.ai");
            valid=source&&source->length()==ai_bytes;if(source)FS.r_close(source);
        }
        if(valid)
        {
            invalid_reason="nav_checksum";
            grid.cells.resize(count);file->r(grid.cells.data(),count*sizeof(vehicle_travel::Cell));
            valid=vehicle_travel::checksum(grid.cells.data(),count*sizeof(vehicle_travel::Cell))==checksum;
        }
    }
    FS.r_close(file);
    if(!valid)return status("no_navigation",invalid_reason);
    // The map is an abstract trip, not a swept-hull driving simulation.
    auto& search=job->search;search.clearance=.25f;
    search.blocked.assign(grid.cells.size(),0);search.gx=goal.x;search.gy=goal.y;search.gz=goal.z;search.radius=radius;
    job->start=Position();job->goal=goal;job->actor=Owner()->ID();
    float pitch,bank;XFORM().getHPB(job->yaw,pitch,bank);
    float best=flt_max;int seed=-1;
    int base=grid.index(Position().x,Position().z);
    if(base<0)return status("no_route","start_outside_grid");
    int row=base/grid.w,col=base%grid.w,reach=int(std::ceil(100.f/grid.step));
    for(int dr=-reach;dr<=reach;++dr)for(int dc=-reach;dc<=reach;++dc)
    {
        int r=row+dr,c=col+dc;if(r<0||c<0||r>=int(grid.h)||c>=int(grid.w))continue;
        int id=r*grid.w+c;if(!search.walk(id))continue;
        Fvector p=point(grid,id);float d=Position().distance_to_sqr(p);
        if(d<best){best=d;seed=id;}
    }
    if(seed<0||best>10000.f)return status("no_route","no_start_floor");
    search.start=seed;
    int center=grid.index(goal.x,goal.z);if(center<0)return status("no_parking");
    row=center/grid.w;col=center%grid.w;reach=int(std::ceil(radius/grid.step));
    for(int dr=-reach;dr<=reach;++dr)for(int dc=-reach;dc<=reach;++dc)
    {
        int r=row+dr,c=col+dc;if(r<0||c<0||r>=int(grid.h)||c>=int(grid.w))continue;
        int id=r*grid.w+c;Fvector p=point(grid,id);
        if(search.walk(id)&&p.distance_to(goal)<=radius-.25f)
        {job->candidates.push_back(id);}
    }
    std::sort(job->candidates.begin(),job->candidates.end(),[&](int a,int b)
    {
        return point(grid,a).distance_to_sqr(goal)<point(grid,b).distance_to_sqr(goal);
    });
    if(job->candidates.empty())return status("no_parking","no_floor_near_beacon");
    m_travel_job=job.release();return status("pending");
}

luabind::object CCar::TravelRouteStep()
{
    auto* job=m_travel_job;
    if(!job||!IsMyCar()||!Owner()||Owner()->ID()!=job->actor||Position().distance_to_sqr(job->start)>4.f)
    {TravelRouteCancel();return status("invalid_vehicle");}
    auto& search=job->search;auto& grid=search.grid;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(4);
    if(job->phase==0)
    {
        // Only route endpoints: no parking/vehicle-hull tests before starting.
        for(int budget=0;budget<512&&job->candidate<job->candidates.size();++budget)
        {
            search.goals.push_back(job->candidates[job->candidate++]);
            if(std::chrono::steady_clock::now()>=deadline)break;
        }
        if(job->candidate==job->candidates.size())
        {
            if(search.goals.empty()){TravelRouteCancel();return status("no_parking","all_parking_blocked");}
            search.reset(search.start);job->phase=1;
        }
        return pending(job);
    }
    if(job->phase==1)
    {
        for(int n=0;n<65536;++n)
        {
            if(search.advance())
            {
                if(search.exhausted){TravelRouteCancel();return status("no_route","search_exhausted");}
                job->phase=2;job->validate=0;job->obstructed=false;break;
            }
            if((n&127)==0&&std::chrono::steady_clock::now()>=deadline)break;
        }
        return pending(job);
    }
    // No physics checks along the virtual path. TravelProbe/Move certify actual spawn.
    auto result=status("ready"),points=table();int written=0;
    // Keep every bend; omit only collinear grid nodes. Distances remain geometric.
    for(size_t k=0;k<search.path.size();++k)
    {
        int id=search.path[k];
        if(k>0&&k+1<search.path.size()&&id-search.path[k-1]==search.path[k+1]-id)continue;
        auto p=table();p["x"]=grid.x(id);p["y"]=grid.y(id);p["z"]=grid.z(id);points[++written]=p;
    }
    if(written>2046||written<1){TravelRouteCancel();return status("no_route");}
    result["points"]=points;result["expanded"]=search.expanded;result["retries"]=job->retries;
    Fvector size;TravelBounds().getsize(size);result["width"]=size.x;result["length"]=size.z;
    result["parking_distance"]=point(grid,search.found).distance_to(job->goal);
    TravelRouteCancel();return result;
}

bool CCar::TravelMove(const Fvector& floor,float yaw)
{
    if(!IsMyCar()||!OwnerActor()||CurrentVel().magnitude()>.5f||!m_pPhysicsShell||!g_Alive())return false;
    Fmatrix target;if(!TravelPose(floor,yaw,true,target))return false;
    auto& navigation=ai().level_graph();
    if(!navigation.valid_vertex_position(floor))return false;
    u32 vertex=navigation.vertex_id(floor);
    if(!navigation.valid_vertex_id(vertex))
    {
        const float step=navigation.header().cell_size();
        for(int radius=1;radius<=2&&!navigation.valid_vertex_id(vertex);++radius)
            for(int dx=-radius;dx<=radius&&!navigation.valid_vertex_id(vertex);++dx)
                for(int dz=-radius;dz<=radius&&!navigation.valid_vertex_id(vertex);++dz)
                {
                    Fvector p=floor;p.x+=dx*step;p.z+=dz*step;
                    if(navigation.valid_vertex_position(p))vertex=navigation.vertex_id(p);
                }
    }
    if(!navigation.valid_vertex_id(vertex))return false;
    const auto game_vertex=ai().cross_table().vertex(vertex).game_vertex_id();
    if(!ai().game_graph().valid_vertex_id(game_vertex))return false;
    // COP_ENGINE_PATCH: COP-ENG-241 - finish old bone callbacks before moving
    // their physics bodies. As in RestoreNetState, the next normal visual update
    // refreshes interpolation before evaluating the relocated skeleton.
    auto* kinematics=PKinematics(Visual());
    Msg("[pf_vehicle_move] car=%u stage=old_bones floor=(%.3f,%.3f,%.3f) heading=%.6f",ID(),floor.x,floor.y,floor.z,yaw);
    kinematics->CalculateBones_Invalidate();kinematics->CalculateBones(TRUE);
    // All checks precede mutation. Move every physical element by one rigid delta;
    // doors/wheels, inventory, radio owner, holder and IDs remain the same objects.
    Fmatrix current,inv,delta;m_pPhysicsShell->GetGlobalTransformDynamic(&current);
    Fvector axis;axis.crossproduct(current.i,current.j);
    if(!_valid(current)||std::abs(axis.dotproduct(current.k))<.0001f)return false;
    inv.invert(current);delta.mul_43(target,inv);
    Msg("[pf_vehicle_move] car=%u stage=rigid_move current=(%.3f,%.3f,%.3f) axes=(%.6f,%.6f,%.6f)",ID(),current.c.x,current.c.y,current.c.z,current.i.magnitude(),current.j.magnitude(),current.k.magnitude());
    m_pPhysicsShell->TransformPosition(delta,mh_clear);
    Fvector zero=Fvector().set(0,0,0);
    m_pPhysicsShell->set_LinearVel(zero);m_pPhysicsShell->set_AngularVel(zero);
    m_CarNetUpdates.clear();ResetKeys();NeutralDrive();ReleaseHandBreak();
    m_pPhysicsShell->GetGlobalTransformDynamic(&XFORM());
    kinematics->CalculateBones_Invalidate();
    Msg("[pf_vehicle_move] car=%u stage=moved current=(%.3f,%.3f,%.3f) axes=(%.6f,%.6f,%.6f)",ID(),Position().x,Position().y,Position().z,XFORM().i.magnitude(),XFORM().j.magnitude(),XFORM().k.magnitude());
    Owner()->XFORM().mul_43(XFORM(),m_sits_transforms[0]);
    // Cars deliberately return FALSE from UsedAI_Locations(). spatial_move alone
    // would leave their spawn vertex behind after a multi-kilometre transfer.
    ai_location().level_vertex(vertex);ai_location().game_vertex(game_vertex);
    spatial_move();Owner()->spatial_move();
    // Synchronize server/ALife coordinates immediately, including a save on the same frame.
    for(CGameObject* object:{static_cast<CGameObject*>(this),Owner()})
    {
        auto* server=Level().Server?smart_cast<CSE_ALifeDynamicObject*>(Level().Server->ID_to_entity(object->ID())):nullptr;
        if(server)
        {
            server->o_Position=object->Position();object->XFORM().getXYZ(server->o_Angle);
            server->m_tNodeID=object->ai_location().level_vertex_id();
            server->m_tGraphID=object->ai_location().game_vertex_id();
        }
    }
    Msg("[pf_vehicle_move] car=%u stage=complete",ID());return true;
}
