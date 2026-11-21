////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_storage_manager.cpp
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator storage manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_storage_manager.h"
#include "alife_simulator_header.h"
#include "alife_time_manager.h"
#include "alife_spawn_registry.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_group_registry.h"
#include "alife_registry_container.h"
#include "xrServer.h"
#include "Level.h"
#include "../xrEngine/x_ray.h"
#include "saved_game_wrapper.h"
#include "../xrEngine/IGame_Persistent.h"
#include "autosave_manager.h"
#include "save_format239.h"
#include "save_pair_transaction.h"
#include "../xrEngine/string_table.h"

XRCORE_API string_path g_bug_report_file;

using namespace ALife;

extern string_path g_last_saved_game;

CALifeStorageManager::~CALifeStorageManager()
{
}

void CALifeStorageManager::save(LPCSTR save_name_no_check, bool update_name)
{
    // COP_ENGINE_PATCH: COP-ENG-239 - stage the complete pair before replacing a save.
    m_last_save_succeeded = false;
    LPCSTR game_saves_path = FS.get_path("$game_saves$")->m_Path;
    string_path save_name, previous_name;
    strncpy_s(save_name, sizeof(save_name), save_name_no_check, sizeof(save_name)-5-xr_strlen(IXRAY_DEF_SAVE_EXTENSION)-xr_strlen(game_saves_path));
    xr_strcpy(previous_name, m_save_name);
    if (save_name[0]) xr_strconcat(m_save_name, save_name, IXRAY_DEF_SAVE_EXTENSION);
    if (!m_save_name[0]) { Msg("! [SAVE239] No save name"); return; }

    string_path full_name;
    FS.update_path(full_name, "$game_saves$", m_save_name);
    const save_pair239::Paths paths(full_name);
    const auto fail = [&](LPCSTR reason)
    {
        Msg("! [SAVE239] Save cancelled: %s; previous save preserved [%s]", reason, m_save_name);
        xr_strcpy(m_save_name, previous_name);
        FS.rescan_path(game_saves_path, TRUE);
    };
    if (!save_pair239::prepare(paths)) { fail("could not prepare save transaction"); return; }

    // An old Lua module cannot accidentally overwrite the final sidecar: it has
    // no stage_save entry point, so a mixed installation fails before any write.
    luabind::functor<bool> stage_state;
    if (!ai().script_engine().functor("alife_storage_manager.CALifeStorageManager_stage_save", stage_state))
    { fail("save scripts do not support transactional staging"); return; }
    if (!stage_state((LPCSTR)m_save_name)) { fail("could not stage custom save state"); return; }

    CMemoryWriter stream;
    header().save(stream);
    time_manager().save(stream);
    spawns().save(stream);
    if (!objects().save(stream)) { fail("an object exceeds save capacity"); return; }
    registry().save(stream);
    // Retain the original second state capture after server-object serialization.
    if (!stage_state((LPCSTR)m_save_name)) { fail("could not finalize custom save state"); return; }

    const u32 source_count = stream.tell();
    const u32 dest_capacity = rtc_csize(source_count);
    void* dest_data = xr_malloc(dest_capacity);
    const u32 dest_count = (u32)rtc_compress(dest_data, dest_capacity, stream.pointer(), source_count);
    if (!dest_count || dest_count > dest_capacity)
    { xr_free(dest_data); fail("save compression failed"); return; }
    const uint32_t file_header[3] = { save_format239::extended_magic, ALIFE_VERSION, source_count };
    const bool written = save_pair239::Files::write_save(paths.staged_main, file_header, dest_data, dest_count);
    xr_free(dest_data);
    if (!written) { fail("could not write complete staged save"); return; }

    if (!save_pair239::commit(paths, save_pair239::Files::exists(paths.staged_data)))
    { fail("could not commit save pair"); return; }
    FS.rescan_path(game_saves_path, TRUE);
    m_last_save_succeeded = true;
    xr_strcpy(g_last_saved_game, save_name);
    Msg("* Game %s is successfully saved to file '%s' [SAVE239 capacity=%u]", m_save_name, full_name, NET_PacketSizeLimit - 1);
    luabind::functor<void> after_save;
    if (ai().script_engine().functor("alife_storage_manager.CALifeStorageManager_after_save", after_save))
        after_save((LPCSTR)m_save_name);
    if (!update_name) xr_strcpy(m_save_name, previous_name);
}

void CALifeStorageManager::load(void* buffer, const u32& buffer_size, LPCSTR file_name)
{
	// So we can get the fname to make our own custom save states
	luabind::functor<void> funct;
	ai().script_engine().functor("alife_storage_manager.CALifeStorageManager_load", funct);
	if (funct)
	{
		funct(file_name);
	}

	IReader source(buffer, buffer_size);
	header().load(source);
	time_manager().load(source);
	spawns().load(source, file_name);
	graph().on_load();
	objects().load(source);

	VERIFY(can_register_objects());
	can_register_objects(false);
	CALifeObjectRegistry::OBJECT_REGISTRY::iterator	B = objects().objects().begin();
	CALifeObjectRegistry::OBJECT_REGISTRY::iterator	E = objects().objects().end();
	CALifeObjectRegistry::OBJECT_REGISTRY::iterator	I;
	for (I = B; I != E; ++I) {
		ALife::_OBJECT_ID id = (*I).second->ID;
		(*I).second->ID = server().PerformIDgen(id);
		VERIFY(id == (*I).second->ID);
		register_object((*I).second, false);
	}

	registry().load(source);

	can_register_objects(true);

	for (I = B; I != E; ++I)
		(*I).second->on_register();

	if (!g_pGameLevel)
		return;

	Level().autosave_manager().on_game_loaded();
}

bool CALifeStorageManager::load(LPCSTR save_name_no_check)
{
	LPCSTR game_saves_path		= FS.get_path("$game_saves$")->m_Path;

	string_path					save_name;
	strncpy_s					(save_name, sizeof(save_name), save_name_no_check, sizeof(save_name)-5-xr_strlen(IXRAY_DEF_SAVE_EXTENSION)-xr_strlen(game_saves_path));

	CTimer timer;
	timer.Start();

	string_path					saveBackup;
	xr_strcpy					(saveBackup,m_save_name);
	if (!save_name[0]) {
		if (!xr_strlen(m_save_name))
		{
			Log("There is no file name specified!");
			return false;
		}
	}
	else
	{
		xr_strconcat(m_save_name, save_name, IXRAY_DEF_SAVE_EXTENSION);
	}

	string_path file_name;
	FS.update_path(file_name, "$game_saves$", m_save_name);

	if (!save_pair239::recover(save_pair239::Paths(file_name))) { Msg("! [SAVE239] Unfinished save could not be rolled back"); return false; }
	FS.rescan_path(game_saves_path, TRUE);

	xr_strcpy(g_last_saved_game, save_name);
	xr_strcpy(g_bug_report_file, file_name);

	xr_strcpy					(g_last_saved_game, save_name);
	xr_strcpy					(g_bug_report_file, file_name);

    IReader* stream = FS.r_open(file_name);
    if (!stream)
    {
        Msg("* Cannot open saved game %s", file_name);
        xr_strcpy(m_save_name, saveBackup);
        return false;
    }

	constexpr pcstr mismatch = "Saved game version mismatch or saved game is corrupted";
	const bool gameSaveIsValid = CSavedGameWrapper::valid_saved_game(*stream);
	VERIFY3(gameSaveIsValid, mismatch, file_name);

	if (!gameSaveIsValid)
	{
		Msg("! %s [%s]", mismatch, file_name);

		xr_strcpy(m_save_name, saveBackup);
		return false;
	}

	string512					temp;
	xr_strconcat(temp, g_pStringTable->translate("st_loading_saved_game").c_str(), " \"", save_name, IXRAY_DEF_SAVE_EXTENSION, "\"");
	g_pGamePersistent->SetLoadStageTitle(temp);
	g_pGamePersistent->LoadTitle();

	unload();
	reload(m_section);

	u32 source_count = stream->r_u32();
	void* source_data = xr_malloc(source_count);
	rtc_decompress(source_data, source_count, stream->pointer(), stream->length() - 3 * sizeof(u32));
	FS.r_close(stream);
	load(source_data, source_count, file_name);
	xr_free(source_data);

	groups().on_after_game_load();

	VERIFY(graph().actor());

	Msg("* Game %s is successfully loaded from file '%s' (%.3fs)", save_name, file_name, timer.GetElapsed_sec());

	return true;
}

void CALifeStorageManager::save(NET_Packet& net_packet)
{
	m_last_save_succeeded = false;
	if (!prepare_objects_for_save()) return;

	shared_str game_name;
	net_packet.r_stringZ(game_name);
	save(*game_name, !!net_packet.r_u8());
}

bool CALifeStorageManager::prepare_objects_for_save()
{
	Level().ClientSend();
	return Level().ClientSave();
}
