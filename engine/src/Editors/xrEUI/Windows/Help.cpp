#include "stdafx.h"
#include "Help.h"

static CUIHelp* InstHelp = nullptr;

void TextCentered(const char* text)
{
	auto windowWidth = ImGui::GetWindowSize().x;
	auto textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text(text);
}

CUIHelp::CUIHelp() :
	IEditorWnd()
{
	bOpen = false;
}

void CUIHelp::Draw()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(180, 0));
	if (ImGui::Begin("About SDK##Help", &bOpen))
	{
		TextCentered("IX-Ray SDK 1.0");
		ImGui::Separator();

		ImGui::Text("Based on:");
		ImGui::SameLine();
		ImGui::HyperLink("RedPanda SDK 0.8", "https://github.com/RedPandaProjects/XRayEngine/tree/master", true);

		ImGui::Text("Used references from:");
		ImGui::Text("*"); ImGui::SameLine();
		ImGui::HyperLink("OMP SDK", "https://github.com/xray-omp/omp-sdk", true);
		ImGui::Text("*"); ImGui::SameLine();
		ImGui::HyperLink("Hybrid SDK", "https://github.com/Roman-n/HybridXRay", true);
		ImGui::Text("*"); ImGui::SameLine();
		ImGui::HyperLink("B.O.R.S.C.H.T.", "https://bitbucket.org/stalker/xray-borscht/commits/branch/borscht", true);
		ImGui::Text("*"); ImGui::SameLine();
		ImGui::HyperLink("TSMP SDK", "https://github.com/tsmp/xraySdkEditors", true);
	}
	ImGui::PopStyleVar();

	ImGui::End();
}

CUIHelp& CUIHelp::Instance()
{
	if (InstHelp == nullptr)
	{
		InstHelp = new CUIHelp;
	}

	return *InstHelp;
}

void CUIHelp::Show()
{
	bOpen = true;
}
