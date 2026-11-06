#pragma once


enum ETelekineticState {
	TS_None,
	TS_Raise,
	TS_Keep,
	TS_Fire,
};

class CGameObject;
class CPhysicsShellHolder;
class CTelekineticObject;
class CPHUpdateObject;
class CTelekinesis;
class CTelekineticObject {

	ETelekineticState	state;
public:
	CPhysicsShellHolder *object;
	CTelekinesis		*telekinesis;
	float				target_height;

	u32					time_keep_started;
	u32					time_keep_updated;
	u32					time_raise_started;

	u32					time_to_keep;
	
	u32					time_fire_started;

	float				strength;
	float				m_acceleration_limit;
	float				m_gravity_compensation;
	bool				m_hold_enabled;

	bool				m_rotate;
	
	ref_sound			sound_hold;
	ref_sound			sound_throw;

public:
								CTelekineticObject		();
								~CTelekineticObject		();
	
virtual		bool				init					(CTelekinesis* tele,CPhysicsShellHolder *obj, float s, float h, u32 ttk, bool rot = true); 
			void				set_sound				(const ref_sound &snd_hold, const ref_sound &snd_throw);
			void				set_acceleration_limit	(float limit) { m_acceleration_limit = _max(limit, 0.f); }
			float				acceleration_limit		() const { return m_acceleration_limit; }
			void				set_gravity_compensation(float scale) { m_gravity_compensation = clampr(scale, 0.f, 1.f); }
			void				set_hold_enabled		(bool enabled) { m_hold_enabled = enabled; }

virtual		void				raise					(float step);
virtual		void				raise_update			();

			void				prepare_keep			();
virtual		void				keep					();
virtual		void				keep_update				();
virtual		void				release					();
virtual		void				fire					(const Fvector &target, float power);
			void				fire_t					(const Fvector &target, float time);
virtual		void				fire_update				();
virtual		void				update_state			();
virtual		bool				can_activate			(CPhysicsShellHolder *obj);
			bool				is_released				(){return state==TS_None;}
			ETelekineticState	get_state				() {return state;}
virtual		void				switch_state			(ETelekineticState new_state);
			CPhysicsShellHolder *get_object				() {return object;}

			bool				check_height			();
			bool				check_raise_time_out	();

			bool				time_keep_elapsed		();
			bool				time_fire_elapsed		();

			

			void				enable					();

			bool				operator==				(const CPhysicsShellHolder *obj) {
				return (object == obj);
			}

			void				rotate					();
private:
			void				update_hold_sound		();

};
