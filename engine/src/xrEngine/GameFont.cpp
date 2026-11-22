
#include "stdafx.h"
#include "GameFont.h"
#include "string_table.h"

#include "../xrCore/API/xrAPI.h"
#include "../Include/xrRender/RenderFactory.h"
#include "../Include/xrRender/FontRender.h"
#include <freetype/freetype.h>

FT_Library FreetypeLib = nullptr;

bool CGameFont::bFreetypeInitialized = false;

void CGameFont::InitializeFreetype()
{
	FT_Error Error = FT_Init_FreeType(&FreetypeLib);
	R_ASSERT2(Error == 0, "Freetype2 initialize failed");
}

#define DI2PX(x) float(iFloor((x + 1) * float(Device.TargetWidth) * 0.5f))
#define DI2PY(y) float(iFloor((y + 1) * float(Device.TargetHeight) * 0.5f))

ENGINE_API Fvector2	g_current_font_scale = { 1.0f, 1.0f };

CGameFont::CGameFont(const char* section, u32 flags) : Name(section), LineSpacing(1.f), LetterSpacing(0.f)
{
#ifdef DEBUG
	Msg("* Init font %s", section);
#endif

	uFlags = flags;

	// Read font name
	if (pSettings->line_exist(section, "name"))
		Data.Name = xr_strdup(xr_strlwr((char*)pSettings->r_string(section, "name")));

	if (!Data.Name || !xr_strlen(Data.Name))
		Data.Name = xr_strdup(section);

	// Set font shader and style
	Data.Shader = xr_strdup(pSettings->r_string(section, "shader"));

	Data.Style = nullptr;
	if (pSettings->line_exist(section, "style"))
		Data.Style = xr_strdup(pSettings->r_string(section, "style"));

	// Read font size
	Data.Size = 14;
	if (pSettings->line_exist(section, "size"))
		Data.Size = (u16)pSettings->r_u32(section, "size");
	
	if (pSettings->line_exist(section, "opentype"))
		Data.OpenType = pSettings->r_bool(section, "opentype");

	if (pSettings->line_exist(section, "letter_spacing"))
		SetLetterSpacing(pSettings->r_float(section, "letter_spacing"));

	if (pSettings->line_exist(section, "line_spacing"))
		SetLineSpacing(pSettings->r_float(section, "line_spacing"));

	// Init
	pFontRender = RenderFactory->CreateFontRender();
	Prepare(Data.Name, Data.Shader, Data.Style, Data.Size);
}

CGameFont::~CGameFont()
{
	// Shading
	FT_Done_Face(OurFont);

	RenderFactory->DestroyFontRender(pFontRender);
	pFontRender = nullptr;

	xr_free(Data.Shader);
	xr_free(Data.Style);
	xr_free(Data.Name);
}

void CGameFont::ReInit()
{
	Prepare(Data.Name, Data.Shader, Data.Style, Data.Size);
}

void CGameFont::Prepare(const char* name, const char* shader, const char* style, u32 size)
{
	Initialize2(name, shader, style, size);
}



xr_vector<xr_string> split(const xr_string& s, char delim)
{
	xr_vector<xr_string> elems;
	std::stringstream ss(s);
	xr_string item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return std::move(elems);
}
#include <freetype/ftfntfmt.h>

extern u32 TextureDimension;
extern xr_vector<u32> FontBitmap;

void CGameFont::Initialize2(const char* name, const char* shader, const char* style, u32 size)
{
	if (!bFreetypeInitialized)
	{
		InitializeFreetype();
		bFreetypeInitialized = true;
	}

	ZeroMemory(&Style, sizeof(Style));
	Size = size;

	if (style != nullptr)
	{
		xr_string StyleDesc(style);
		xr_vector<xr_string> StyleTokens = split(StyleDesc, '|');
		for (const xr_string& token : StyleTokens)
		{
			if (token == "bold")
			{
				Style.bold = 1;
			}
			else if (token == "italic")
			{
				Style.italic = 1;
			}
			else if (token == "underline")
			{
				Style.underline = 1;
			}
			else if (token == "strike")
			{
				Style.strike = 1;
			}
		}
	}
	ZeroMemory(FontBitmap.data(), sizeof(u32) * (TextureDimension * TextureDimension));

	// есть кучу способов высчитать размер шрифта для скейлинга
	// 1. основываясь на DPI(PPI), однако, как не вычисляй его он всегда считается исходя из разрешения моника(системы) и 23 дюймов(мб с дровами на моник - из реальных дюймов)
	// 2. основываясь на том, как ПЫС делают скейлинг из UI_BASE_HEIGHT/UI_BASE_WIDTH и тд...

	SetProcessDPIAware();
	HDC hDCScreen = GetDC(NULL);

	auto Hmm = (float)GetDeviceCaps(hDCScreen, VERTSIZE);
	auto Wmm = (float)GetDeviceCaps(hDCScreen, HORZSIZE);
	auto Hpx = (float)GetDeviceCaps(hDCScreen, VERTRES);
	auto Wpx = (float)GetDeviceCaps(hDCScreen, HORZRES);

	ReleaseDC(NULL, hDCScreen);

	auto is_res_depend = !!READ_IF_EXISTS(pSettings, r_bool, Name, "res_depend", TRUE);
	auto is_dpi_depend = !!READ_IF_EXISTS(pSettings, r_bool, Name, "dpi_depend", !is_res_depend);

	auto ppi = int(25.4f * sqrt(Hpx * Hpx + Wpx * Wpx) / sqrt(Hmm * Hmm + Wmm * Wmm));

	auto res_scale = is_res_depend ? float(Device.TargetHeight) / 900.0f : 1.0f;
	auto ppi_scale = is_dpi_depend ? float(ppi) / 92.0f : 1.0f;

	auto fHeight = float(size * res_scale * ppi_scale);

	CStringTable::LangName();
	xr_string NameWithExt = CStringTable::LangName() + "\\" + name + ".ttf";

	string_path FullPath;
	FS.update_path(FullPath, _game_fonts_, NameWithExt.c_str());

	IReader* FontFile = FS.r_open(FullPath);
	if (FontFile == nullptr)
	{
		Msg("! Can't open font file %s", name);

		string_path DefPath;
		NameWithExt = CStringTable::LangName() + "\\default.ttf";
		FS.update_path(DefPath, _game_fonts_, NameWithExt.c_str());
		FontFile = FS.r_open(DefPath);

		R_ASSERT3(FontFile != nullptr, "Can't find default font: %s", DefPath);
	}

    // COP_ENGINE_PATCH: COP-ENG-248 - FreeType charmap and glyph keys are Unicode.
    // FreeType keeps the memory face data until FT_Done_Face, including reloads.
    if (OurFont) FT_Done_Face(OurFont);
    OurFont = nullptr;
    GlyphData.clear();
	FT_Error FTError = FT_New_Memory_Face(FreetypeLib, (FT_Byte*)FontFile->pointer(), FontFile->length(), 0, &OurFont);
	R_ASSERT3(FTError == 0, "FT_New_Memory_Face return error", FullPath);
    OurFont->generic.data = FontFile;
    OurFont->generic.finalizer = [](void* object)
    {
        IReader* file = static_cast<IReader*>(static_cast<FT_Face>(object)->generic.data);
        FS.r_close(file);
    };
    FT_Select_Charmap(OurFont, FT_ENCODING_UNICODE);

	u32 TargetX = 0;
	u32 TargetY = 0;
	u32 TargetX2 = 0;
	u32 TargetY2 = 0;

	FT_Size_RequestRec req;
	req.type = FT_SIZE_REQUEST_TYPE_CELL;
	req.width = 0;
	req.height = (uint32_t)fHeight * 64;
	req.horiResolution = 0;
	req.vertResolution = 0;
	FT_Request_Size(OurFont, &req);

#define FT_CEIL(X)  (((X + 63) & -64) / 64)

	float FontSizeInPixels = (float)(OurFont->size->metrics.ascender - OurFont->size->metrics.descender) / 64.0f;

	auto CopyGlyphImageToAtlas = [this, &TargetX, &TargetX2, &TargetY, &TargetY2, FontSizeInPixels](FT_Bitmap& GlyphBitmap)
		{
			TargetX2 = TargetX + GlyphBitmap.width;
			if (TargetX2 >= TextureDimension)
			{
				TargetX = 0;
				TargetX2 = GlyphBitmap.width;
				TargetY += (u32)FontSizeInPixels + 5;

				R_ASSERT2(TargetY <= TextureDimension, "Font too large, or dimension texture is too small");
			}

			u32 SourceX = 0;
			u32 SourceY = 0;

			TargetY2 = TargetY + (u32)FontSizeInPixels;
			u32 TargetYSaved = TargetY;
			TargetY = TargetY + u32(FontSizeInPixels - (float)GlyphBitmap.rows);

			switch (GlyphBitmap.pixel_mode)
			{
			case FT_PIXEL_MODE_GRAY:
			{
				u8* SourceImage = GlyphBitmap.buffer;
				for (u32 y = TargetY; y < TargetY2; y++, SourceY++)
				{
					for (u32 x = TargetX; x < TargetX2; x++, SourceX++)
					{
						u8 SourcePixel = SourceImage[(SourceY * GlyphBitmap.pitch) + SourceX];

						u32 FinalPixel = SourcePixel;
						FinalPixel |= (SourcePixel << 8);
						FinalPixel |= (SourcePixel << 16);
						FinalPixel |= (SourcePixel << 24);

						FontBitmap[(y * TextureDimension) + x] = FinalPixel;
					}
					SourceX = 0;
				}
			}
			break;
			case FT_PIXEL_MODE_BGRA:
			{
				u32* SourceImage = (u32*)GlyphBitmap.buffer;
				for (u32 y = TargetY; y < TargetY2; y++, SourceY++)
				{
					for (u32 x = TargetX; x < TargetX2; x++, SourceX++)
					{
						u32 SourcePixel = SourceImage[(SourceY * GlyphBitmap.pitch) + SourceX];

						u8 Alpha = (SourcePixel & 0x000000FF);
						u8 Red = (SourcePixel & 0x0000FF00) >> 8;
						u8 Green = (SourcePixel & 0x00FF0000) >> 16;
						u8 Blue = (SourcePixel & 0xFF000000) >> 24;

						u32 FinalPixel = Alpha;
						FinalPixel |= (u32(Blue) << 8);
						FinalPixel |= (u32(Green) << 16);
						FinalPixel |= (u32(Red) << 24);

						FontBitmap[(y * TextureDimension) + x] = FinalPixel;
					}
					SourceX = 0;
				}
			}
			break;
			default:
				break;
			}

			TargetY = TargetYSaved;
		};

	//const char* Format = FT_Get_Font_Format(OurFont);
	u32 index = 0;

	auto LoadGlyph = [&](FT_UInt glyphID)
	{
        const FT_UInt FreetypeCharacter = FT_Get_Char_Index(OurFont, glyphID);
        if (FreetypeCharacter == 0 && glyphID != 0) return;

		FTError = FT_Load_Glyph(OurFont, FreetypeCharacter, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL);
		R_ASSERT3(FTError == 0, "FT_Load_Glyph return error", FullPath);
		FT_GlyphSlot Glyph = OurFont->glyph;
		FT_Glyph_Metrics& GlyphMetrics = Glyph->metrics;

		CopyGlyphImageToAtlas(Glyph->bitmap);

		RECT region;
		region.left = TargetX;
		region.right = TargetX + Glyph->bitmap.width;
		region.top = TargetY;
		region.bottom = long(TargetY + FontSizeInPixels);

		ABC widths;
		widths.abcA = FT_CEIL(GlyphMetrics.horiBearingX);
		widths.abcB = Glyph->bitmap.width;
		widths.abcC = FT_CEIL(GlyphMetrics.horiAdvance) - widths.abcB - widths.abcA;

		int GlyphTopScanlineOffset = int(FontSizeInPixels - Glyph->bitmap.rows);
		int yOffset = -Glyph->bitmap_top - GlyphTopScanlineOffset;
		yOffset += (int)FontSizeInPixels; // Return back to the center pos
		yOffset -= (int)(FontSizeInPixels / 4);

		GlyphData[glyphID] = { region, widths, yOffset };

		TargetX = TargetX2;
		TargetX += 4;
	};

	auto glyphID = FT_Get_First_Char(OurFont, &index);
	while(index != 0)
	{
		LoadGlyph(glyphID);
		glyphID = FT_Get_Next_Char(OurFont, glyphID, &index);
	}

	fCurrentHeight = FontSizeInPixels;

	string128 textureName;
	xr_sprintf(textureName, "$user$%s", Name); //#TODO optimize

	auto TargetDemensionY = 16u;

	while (TargetDemensionY < TargetY2) {
		TargetDemensionY *= 2u;
	}
#ifdef DEBUG
	Msg("* Font %s Y size [%d - %d]", Name, TargetDemensionY, TargetY2);
#endif
	R_ASSERT2(TargetDemensionY <= TextureDimension, "Font too large, or dimension texture is too small");

	pFontRender->CreateFontAtlas(TextureDimension, TargetDemensionY, textureName, FontBitmap.data());

	pFontRender->Initialize(shader, textureName);
}

void CGameFont::OutSet(float x, float y)
{
	fCurrentX = x;
	fCurrentY = y;
}

void CGameFont::OutSetI(float x, float y)
{
	OutSet(DI2PX(x), DI2PY(y));
}

void CGameFont::OnRender()
{
	PROF_EVENT("Render Font");
	xrCriticalSectionGuard g(&s_cs);
	if (!strings.empty())
	{
		pFontRender->OnRender(*this);
		strings.clear();
	}
}

u16 CGameFont::GetCutLengthPos(float fTargetWidth, const char* pszText)
{
    // COP_ENGINE_PATCH: COP-ENG-248 - return a byte boundary for both supported storage formats.
    const std::string_view text(pszText);
    const bool utf8 = UTF8Text::IsValid(text);
    size_t offset = 0, accepted = 0;
    float width = 0.f;
    while (offset < text.size())
    {
        uint32_t codepoint;
        if (utf8) UTF8Text::Decode(text, offset, codepoint);
        else codepoint = UTF8Text::LegacyCodepoint(static_cast<unsigned char>(text[offset++]));
        width += WidthOf(static_cast<int>(codepoint)) + (accepted ? GetLetterSpacing() : 0.f);
        if (width > fTargetWidth || offset > 65535) break;
        accepted = offset;
    }
    return static_cast<u16>(accepted);
}

u16 CGameFont::SplitByWidth(u16* puBuffer, u16 uBufferSize, float fTargetWidth, const char* pszText)
{
    // COP_ENGINE_PATCH: COP-ENG-248 - preserve words and complete encoded characters.
    const std::string_view text(pszText);
    const bool utf8 = UTF8Text::IsValid(text);
    size_t offset = 0, start = 0, space = 0;
    float width = 0.f;
    u16 count = 0;
    while (offset < text.size() && count < uBufferSize)
    {
        const size_t previous = offset;
        uint32_t codepoint;
        if (utf8) UTF8Text::Decode(text, offset, codepoint);
        else codepoint = UTF8Text::LegacyCodepoint(static_cast<unsigned char>(text[offset++]));
        const float next = WidthOf(static_cast<int>(codepoint)) + (previous > start ? GetLetterSpacing() : 0.f);
        if (width + next > fTargetWidth && previous > start)
        {
            const size_t split = space > start ? space : previous;
            if (split > 65535) break;
            puBuffer[count++] = static_cast<u16>(split);
            start = offset = split;
            space = 0;
            width = 0.f;
            continue;
        }
        width += next;
        if (codepoint == ' ' || codepoint == '\t') space = offset;
    }
    return count;
}

void CGameFont::MasterOut(
	BOOL bCheckDevice, BOOL bUseCoords, BOOL bScaleCoords, BOOL bUseSkip,
	float _x, float _y, float _skip, const char* fmt, va_list p)
{
	if (bCheckDevice && (!Device.b_is_Active))
		return;

	String rs;

	rs.x = (bUseCoords ? (bScaleCoords ? (DI2PX(_x)) : _x) : fCurrentX);
	rs.y = (bUseCoords ? (bScaleCoords ? (DI2PY(_y)) : _y) : fCurrentY);
	rs.c = dwCurrentColor;
	rs.gradientColor = dwGradientColor;
	rs.height = fCurrentHeight;
	rs.align = eCurrentAlignment;
	rs.gradient = fGradientEnabled;
	rs.gradientMode = fGradientMode;
    // COP_ENGINE_PATCH: COP-ENG-248 - normalize once, without a fixed byte-size cut.
    va_list copy;
    va_copy(copy, p);
    const int vs_sz = std::vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);
    if (vs_sz < 0) return;
    xr_vector<char> formatted(static_cast<size_t>(vs_sz) + 1);
    std::vsnprintf(formatted.data(), formatted.size(), fmt, p);
    rs.string_utf8 = Platform::ANSI_TO_UTF8(formatted.data());
    const size_t prefix = UTF8Text::Prefix(rs.string_utf8.c_str(), sizeof(rs.string) - 1);
    rs.string_utf8.resize(prefix);
    std::memcpy(rs.string, rs.string_utf8.data(), prefix);
    rs.string[prefix] = 0;

	if (vs_sz)
	{
		xrCriticalSectionGuard g(&s_cs);
		strings.push_back(rs);
	}

	if (bUseSkip)
		OutSkip(_skip);
}

#define MASTER_OUT(CHECK_DEVICE,USE_COORDS,SCALE_COORDS,USE_SKIP,X,Y,SKIP,FMT) \
	{ va_list p; va_start ( p , fmt ); \
	  MasterOut( CHECK_DEVICE , USE_COORDS , SCALE_COORDS , USE_SKIP , X , Y , SKIP , FMT, p ); \
	  va_end( p ); }

void CGameFont::OutI(float _x, float _y, const char* fmt, ...)
{
	MASTER_OUT(FALSE, TRUE, TRUE, FALSE, _x, _y, 0.0f, fmt);
};

void CGameFont::Out(float _x, float _y, const char* fmt, ...)
{
	MASTER_OUT(TRUE, TRUE, FALSE, FALSE, _x, _y, 0.0f, fmt);
};

void CGameFont::OutNext(const char* fmt, ...)
{
	MASTER_OUT(TRUE, FALSE, FALSE, TRUE, 0.0f, 0.0f, LineSpacing, fmt);
};

void CGameFont::OutSkip(float val)
{
	fCurrentY += val * CurrentHeight_();
}

float CGameFont::SizeOf_(int cChar)
{
	if (cChar < 0)
	{
		cChar = UTF8Text::LegacyCodepoint(u8(cChar));
	}

	return WidthOf(cChar);
}

float CGameFont::SizeOf_(const char* s)
{
	return WidthOf(s);
}

float CGameFont::CurrentHeight_()
{
	return GetHeight();
}

void CGameFont::SetHeight(float S)
{
	if (uFlags & fsDeviceIndependent)
	{
		fCurrentHeight = S;
	}
}

const CGameFont::Glyph* CGameFont::GetGlyphInfo(int ch)
{
	auto symbolInfoIterator = GlyphData.find(ch);
	if (symbolInfoIterator == GlyphData.end())
	{
        // Use the same visible fallback in measuring and drawing.
        symbolInfoIterator = GlyphData.find(0xFFFD);
        if (symbolInfoIterator == GlyphData.end()) symbolInfoIterator = GlyphData.find('?');
        if (symbolInfoIterator == GlyphData.end()) return nullptr;
    }

    return &symbolInfoIterator->second;
}

float CGameFont::WidthOf(int ch)
{
	if (ch == '\t' || ch == '\n')
		return 0.f;

	if (const Glyph* glyphInfo = GetGlyphInfo(ch))
		return float(glyphInfo->Abc.abcA + glyphInfo->Abc.abcB + glyphInfo->Abc.abcC);

	return 0.f;
}

float CGameFont::WidthOf(const char* str)
{
    // COP_ENGINE_PATCH: COP-ENG-248 - exactly the renderer's codepoint sequence.
    if (!str || !str[0]) return 0.f;
    const xr_string text = Platform::ANSI_TO_UTF8(str);
    const std::string_view view(text.data(), text.size());
    float width = 0.f;
    size_t offset = 0;
    uint32_t codepoint;
    while (offset < view.size())
    {
        UTF8Text::Decode(view, offset, codepoint);
        width += WidthOf(static_cast<int>(codepoint));
        if (offset < view.size()) width += GetLetterSpacing();
    }
    return width;
}
