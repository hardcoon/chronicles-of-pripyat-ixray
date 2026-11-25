#pragma once
#include "object_interfaces.h"
#include "map_location_defs.h"
#include "PdaMapFilters.h"

class CMapLocationWrapper;
class CInventoryOwner;
class CMapLocation;

class CMapManager
{
	CMapLocationWrapper*	m_locations_wrapper;
	Locations*				m_locations;
	xr_vector<CMapLocation*> m_deffered_destroy_queue;
    // COP_ENGINE_PATCH: COP-ENG-260 - level-owned values, never borrowed UI/Lua handles.
    PdaMapFilters::Mask m_pdaMapFilters = PdaMapFilters::All;
    PdaMapFilters::Mask m_pdaFilterControls = PdaMapFilters::All;
    xr_map<shared_str, PdaMapFilters::Category> m_pdaGroups;
    xr_map<shared_str, PdaMapFilters::Category> m_pdaSpotGroups;
    void LoadPdaFilterGroups();
    void LoadUserMarkerPreference();
public:

							CMapManager					();
							~CMapManager				();
	void			Update						();
	/*ICF */Locations&		Locations					();//{return *m_locations;}
	CMapLocation*			AddMapLocation				(const shared_str& spot_type, u16 id);
	CMapLocation*			AddRelationLocation			(CInventoryOwner* pInvOwner);
	void					RemoveRelationLocation		(CInventoryOwner* pInvOwner);	
	CMapLocation*			AddUserLocation				(const shared_str& spot_type, const shared_str& level_name, Fvector position);
	void					RemoveMapLocation			(const shared_str& spot_type, u16 id);
	bool					HasMapLocation				(const shared_str& spot_type, u16 id);
	void					RemoveMapLocationByObjectID (u16 id); //call on destroy object
	void					RemoveMapLocation			(CMapLocation* ml);
	CMapLocation*			GetMapLocation				(const shared_str& spot_type, u16 id);
	void					GetMapLocations				(const shared_str& spot_type, u16 id, xr_vector<CMapLocation*>& res);
	void					DisableAllPointers			();
	bool					GetMapLocationsForObject	(u16 id, xr_vector<CMapLocation*>& res);
	void					OnObjectDestroyNotify		(u16 id);
	void					ResetStorage				() {m_locations = NULL;};
#ifdef DEBUG
	void					Dump						();
#endif
	void					Destroy						(CMapLocation*);

	xrCriticalSection		UpdateCS;

	void MapLocationsForEach(LPCSTR spot_type, u16 id, const luabind::functor<bool>& functor);
	void AllLocationsForEach(const luabind::functor<bool>& functor);
    PdaMapFilters::Mask PdaFilterMask() const { return m_pdaMapFilters; }
    void SetPdaFilterControls(PdaMapFilters::Mask controls) { m_pdaFilterControls = controls; }
    bool PdaFilterEnabled(PdaMapFilters::Category category) const
        { return PdaMapFilters::Visible(m_pdaMapFilters, m_pdaFilterControls, category); }
    PdaMapFilters::Category PdaFilterGroup(const shared_str& name) const;
    PdaMapFilters::Category PdaSpotGroup(const shared_str& type) const;
    bool SetPdaFilter(PdaMapFilters::Category category, bool enabled);
    void LoadPdaFilters(const u8 (&bytes)[4]);
	static void script_register(lua_State* L);
};
