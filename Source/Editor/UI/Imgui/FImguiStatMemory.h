#pragma once

#include "Source/ThirdParty/Imgui/imgui.h"
#include "Source/Editor/Core/FEditor.h"

struct FImguiStatMemory
{
	uint32 RowCount = 5;

	void Process(FEditor InEditor)
	{
		ImGui::Begin("Stat Memory");
		ImGui::Text("Cycle counters");
		if (ImGui::BeginTable("Cycle counters", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Cycle counters (flat)", ImGuiTableColumnFlags_WidthFixed, 250.f);
			ImGui::TableSetupColumn("CallCount", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("InclusiveAvg", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("InclusiveMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("ExclusiveAvg", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("ExclusiveMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableHeadersRow();
			for (uint32 Row = 0; Row < RowCount; Row++)
			{
				ImGui::TableNextRow();
				for (uint32 Column = 0; Column < 6; Column++)
				{
					ImGui::TableNextColumn();
					switch (Column)
					{
					case 0:
						break;
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						break;
					case 5:
						break;
					default:
						break;
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::Text("Memory counters");
		if (ImGui::BeginTable("Memory counters", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Memory Counters", ImGuiTableColumnFlags_WidthFixed, 250.f);
			ImGui::TableSetupColumn("UsedMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Mem%", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("MemPool", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Pool Capacity", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableHeadersRow();
			for (uint32 Row = 0; Row < RowCount; Row++)
			{
				ImGui::TableNextRow();
				for (uint32 Column = 0; Column < 5; Column++)
				{
					ImGui::TableNextColumn();
					switch (Column)
					{
					case 0:
						break;
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						break;
					default:
						break;
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::Text("Counters");
		if (ImGui::BeginTable("Counters", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Counters", ImGuiTableColumnFlags_WidthFixed, 250.f);
			ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Min", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableHeadersRow();
			for (uint32 Row = 0; Row < RowCount; Row++)
			{
				ImGui::TableNextRow();
				for (uint32 Column = 0; Column < 4; Column++)
				{
					ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					switch (Column)
					{
					case 0:
						break;
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					default:
						break;
					}
				}
			}
			ImGui::EndTable();
		}
		ImGui::End();
	}
};