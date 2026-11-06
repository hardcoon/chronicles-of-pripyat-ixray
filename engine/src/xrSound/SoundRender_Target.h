#ifndef SoundRender_TargetH
#define SoundRender_TargetH
#pragma once

#include "SoundRender.h"
#include "UserMusic183.h"

class CSoundRender_Target
{
protected:
	CSoundRender_Emitter*		m_pEmitter;
	BOOL						rendering;
public:
	float						priority;
protected:
	OggVorbis_File				m_ovf;
	IReader*					m_wave;
    UserMusic183Decoder* m_user_music183 = nullptr; // COP_ENGINE_PATCH: COP-ENG-183
	void						attach				();
	void						dettach				();

public:
								CSoundRender_Target	();
	virtual 					~CSoundRender_Target();

	OggVorbis_File*				get_data			() { return &m_ovf; }
    UserMusic183Decoder* get_user_music183() { return m_user_music183; }
	CSoundRender_Emitter*		get_emitter			() const { return m_pEmitter;	}
	BOOL						get_Rendering		() const { return rendering;	}

	virtual BOOL				_initialize			()=0;
	virtual void				_destroy			()=0;
	virtual void				_restart			()=0;

	virtual void				start				(CSoundRender_Emitter* E)=0;
	virtual void				render				()=0;
	virtual void				rewind				()=0;
	virtual void				stop				()=0;
	virtual void				update				()=0;
	virtual void				fill_parameters		()=0;
};
#endif