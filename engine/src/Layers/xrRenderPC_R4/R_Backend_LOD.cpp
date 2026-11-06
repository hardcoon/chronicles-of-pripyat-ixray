#include "stdafx.h"
#include "R_Backend_LOD.h"

R_LOD::R_LOD()
{
	unmap();
}

void	R_LOD::set_LOD(float LOD)
{
	if (c_LOD)
	{
		float factor = clampr<float>(ceil(LOD*LOD*LOD*LOD*LOD*8.0f), 1, 7);
		RCache.set_c(c_LOD, factor);
	}
}