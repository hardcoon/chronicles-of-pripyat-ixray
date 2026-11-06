#pragma once

class FRbmkGoapPlanner;

class FRbmkGoapProperty
{
public:
						FRbmkGoapProperty	();
	virtual				~FRbmkGoapProperty	();
	virtual bool		GetProperty			() const;

	FRbmkGoapPlanner*	Owner;
	shared_str			Name;
};
class FRbmkGoapPropertyConst:public FRbmkGoapProperty
{
public:
				FRbmkGoapPropertyConst	();
	bool		GetProperty				() const override;

	bool		Value;
};
class FRbmkGoapPropertyMember:public FRbmkGoapProperty
{
public:
	bool		GetProperty				() const override;

	bool*		Value = nullptr;
	bool		Equality = true;
};



struct FRbmkGoapParameter
{
	shared_str	Name;
	bool		Value = false;
};
struct FRbmkGoapParameters
{
	void							Add				(const shared_str&Name ,bool Value);
	void							Add				(const FRbmkGoapParameters & Parameters);
	void							Add				(const FRbmkGoapParameters & Parameters,FRbmkGoapPlanner* ValuesFrom);
	void							SubFromCache	(FRbmkGoapPlanner* From);
	void							ResetHash		();
	bool							operator<		(const FRbmkGoapParameters&Right) const;
	bool							operator==		(const FRbmkGoapParameters&Right) const;
	bool							NotContradict	(const FRbmkGoapParameters&With,FRbmkGoapPlanner* AppendFrom) const;
	bool							NotContradict	(const FRbmkGoapParameters&With) const;
	bool							AllMatch		(const FRbmkGoapParameters&In) const;

	xr_vector<FRbmkGoapParameter>	Parameters;
	bool							InvalidHash = true;
	uint32_t						Hash = 0xFFFFFFFF;
};

class FRbmkGoapAction
{
public:
								FRbmkGoapAction		();
	virtual						~FRbmkGoapAction	();
	virtual void				Active				();
	virtual void				Update				();
	virtual void				End					();
	FRbmkGoapAction&			AddCondition		(const shared_str&Name ,bool Value);
	FRbmkGoapAction&			AddEffect			(const shared_str&Name ,bool Value);

	FRbmkGoapPlanner*			Owner;
	shared_str					Name;
	FRbmkGoapParameters			Conditions;
	FRbmkGoapParameters			Effects;
};
class FRbmkGoapPlanner
{
public:
																		FRbmkGoapPlanner		();
																		~FRbmkGoapPlanner		();

	void																AddProperty				(FRbmkGoapProperty* InProperty);
		template<class C> C*											AddProperty				(const shared_str& Name) { C* Property = new C; Property->Name = Name; AddProperty(Property); return Property; }
		void															RemoveProperty			(FRbmkGoapProperty* InProperty);
	
		void															AddAction				(FRbmkGoapAction* InAction);
		template<class C> C&											AddAction				(const shared_str& Name) { C* Action = new C(); Action->Name = Name; AddAction(Action); return *Action; }
		void															RemoveAction			(FRbmkGoapAction* InProperty);

		void															SetTarget				(const shared_str&Name ,bool Value);
		const shared_str&												GetTarget				(bool *OutValue = nullptr) const;
		void															Update					();
		void															PrintError				();
		bool															IsReached				() const { return Path.size() < 2;}
		FRbmkGoapAction*												GetCurrentAction		() const {return CurrentAction;}
		bool															FindOrAddCacheProperty	( FRbmkGoapProperty*Property);
		void															AddCacheProperty		(const shared_str& name);
		void															Clear					();

		void*															Owner;
		xr_vector<xr_unique_ptr<FRbmkGoapProperty>>					Properties;
		bool															NeedRefresh = false;
		xr_vector<std::pair<FRbmkGoapProperty*,bool>>					CacheProperties;
private:
		bool															ActualCurrentAction	();
		bool															Search				();


		xr_vector<xr_unique_ptr<FRbmkGoapAction>>						Actions;
		FRbmkGoapParameter												Target;

		FRbmkGoapAction*												CurrentAction;
		FRbmkGoapParameters												CurrentParameters;

		/**************************************************************
		 *** For A* ***
		 *************************************************************/
		xr_vector<FRbmkGoapAction*>										Path;
		xr_vector<xr_unique_ptr<FRbmkGoapParameters>>					CacheParameters;
		xr_vector<std::pair<int32_t, FRbmkGoapParameters*>>				TempPriorityNode;
		xr_hash_map<FRbmkGoapParameters*, FRbmkGoapParameters*>			TempCameFrom;
		xr_hash_map<FRbmkGoapParameters*, int32_t>						TempCostSoFar;
		xr_hash_map<FRbmkGoapParameters*, FRbmkGoapAction*>				TempEdges;
	
		xr_hash_map<shared_str,FRbmkGoapProperty*>						Names2Properties;
		xr_hash_map<shared_str,FRbmkGoapAction*>						Names2Actions;

};
