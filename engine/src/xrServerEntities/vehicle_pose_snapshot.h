#pragma once

#include "PHNetState.h"

// COP_ENGINE_PATCH: COP-ENG-159 - lossless-to-float car-only save extension.
// The legacy skeleton remains intact. Relative COMs/rotations are authoritative
// only when this complete, bounded snapshot matches the physical element IDs.
struct SVehiclePoseSnapshot
{
	static constexpr u8 runtime_version = 2;
	static constexpr u8 schema_version = 1;
	static constexpr u16 max_elements = 128;
	static constexpr u16 header_bytes = 21;
	static constexpr u16 element_bytes = 31;

	struct SElement
	{
		u16 bone = u16(-1);
		Fvector position = {};
		Fquaternion rotation = {};
		u8 enabled = 0;
	};

	u16 root = u16(-1);
	VisMask mask;
	xr_vector<SElement> elements;

	void clear()
	{
		root = u16(-1);
		mask.zero();
		elements.clear();
	}

	static bool valid_element(const SElement& e)
	{
		if (e.bone >= max_elements || e.enabled > 1 || !_valid(e.position) || !_valid(e.rotation))
			return false;
		if (_abs(e.position.x) > 10000.f || _abs(e.position.y) > 10000.f || _abs(e.position.z) > 10000.f)
			return false;
		const float norm = e.rotation.x * e.rotation.x + e.rotation.y * e.rotation.y +
			e.rotation.z * e.rotation.z + e.rotation.w * e.rotation.w;
		return norm > 0.99f && norm < 1.01f;
	}

	bool valid() const
	{
		if (root >= max_elements || elements.empty() || elements.size() > max_elements)
			return false;
		bool seen[max_elements] = {};
		for (const auto& e : elements)
		{
			if (!valid_element(e) || seen[e.bone])
				return false;
			seen[e.bone] = true;
		}
		return seen[root];
	}

	bool matches_legacy(const SPHBonesData& legacy) const
	{
		return valid() && root == legacy.root_bone && elements.size() == legacy.bones.size() &&
			mask._visimask.flags == legacy.bones_mask._visimask.flags &&
			(elements.size() <= 64 || mask._visimask_ex.flags == legacy.bones_mask._visimask_ex.flags);
	}

	// The caller uses the same full root position/angles that are written to the
	// car packet. Fmatrix/Fquaternion use X-Ray's convention, including its sign.
	bool append(u16 bone, const SPHNetState& state, const Fmatrix& inverse_root, const Fvector& object_position)
	{
		Fquaternion normalized = state.quaternion;
		if (!_valid(state.position) || !_valid(state.quaternion) || normalized.magnitude() < EPS_S)
			return false;
		SElement e;
		e.bone = bone;
		// Subtract in world axes first. Applying an affine inverse directly at
		// kilometre-scale map coordinates loses low bits by cancellation.
		Fvector delta;
		delta.sub(state.position, object_position);
		inverse_root.transform_dir(e.position, delta);
		normalized.normalize();
		Fmatrix world_rotation, relative;
		world_rotation.rotation(normalized);
		relative.mul_43(inverse_root, world_rotation);
		e.rotation.set(relative).normalize();
		e.enabled = state.enabled ? 1 : 0;
		elements.push_back(e);
		return true;
	}

	SPHNetState world_state(u16 index, const Fmatrix& object) const
	{
		const auto& e = elements[index];
		SPHNetState state;
		object.transform_tiny(state.position, e.position);
		Fmatrix relative, world;
		relative.rotation(e.rotation);
		world.mul_43(object, relative);
		state.quaternion.set(world).normalize();
		state.previous_position = state.position;
		state.previous_quaternion = state.quaternion;
		state.enabled = e.enabled != 0;
		return state;
	}

	void write(NET_Packet& packet) const
	{
		packet.w_u16(u16(header_bytes + elements.size() * element_bytes));
		packet.w_u8(schema_version);
		packet.w_u16(root);
		packet.w_u64(mask._visimask.flags);
		packet.w_u64(mask._visimask_ex.flags);
		packet.w_u16(u16(elements.size()));
		for (const auto& e : elements)
		{
			packet.w_u16(e.bone);
			packet.w_vec3(e.position);
			packet.w_float(e.rotation.x);
			packet.w_float(e.rotation.y);
			packet.w_float(e.rotation.z);
			packet.w_float(e.rotation.w);
			packet.w_u8(e.enabled);
		}
	}

	bool read(NET_Packet& packet)
	{
		clear();
		if (packet.r_elapsed() < sizeof(u16))
			return false;
		const u16 bytes = packet.r_u16();
		if (bytes > packet.r_elapsed())
			return false;
		const u32 end = packet.r_tell() + bytes;
		if (bytes < header_bytes || packet.r_u8() != schema_version)
		{
			packet.r_seek(end);
			return false;
		}
		root = packet.r_u16();
		mask._visimask.flags = packet.r_u64();
		mask._visimask_ex.flags = packet.r_u64();
		const u16 count = packet.r_u16();
		if (count == 0 || count > max_elements || bytes != header_bytes + count * element_bytes)
		{
			packet.r_seek(end);
			clear();
			return false;
		}
		elements.resize(count);
		for (auto& e : elements)
		{
			e.bone = packet.r_u16();
			packet.r_vec3(e.position);
			e.rotation.x = packet.r_float();
			e.rotation.y = packet.r_float();
			e.rotation.z = packet.r_float();
			e.rotation.w = packet.r_float();
			e.enabled = packet.r_u8();
		}
		if (!valid())
		{
			clear();
			return false;
		}
		return true;
	}
};
