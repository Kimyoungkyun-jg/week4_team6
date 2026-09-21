#pragma once

#include "Source/ThirdParty/Imgui/imgui.h"
#include "Source/Editor/Core/FEditor.h"
#include "Runtime/Core/FStatRegistry.h"

template <typename T>
struct MinMaxTracker
{
	T Min;
	T Max;

	MinMaxTracker(T Value)
		:Min(Value), Max(Value) {}

	T Update(T Value)
	{
		if (Value < Min) Min = Value;
		if (Value > Max) Max = Value;
		return Value;
	}
	void Reset(T Value)
	{
		Min = Value;
		Max = Value;
	}
};

struct FImguiStatMemory final
{
	inline static MinMaxTracker<uint32> UObjectArrayNum{0};
	inline static MinMaxTracker<uint32> DrawCallNum{0};
	inline static MinMaxTracker<uint32> DrawTriangleCount{0};
	inline static MinMaxTracker<uint32> DrawVertexCount{0};
	inline static MinMaxTracker<uint32> LineBatchNum{0};
	inline static MinMaxTracker<uint32> OpaqueQNum{0};
	inline static MinMaxTracker<uint32> TranslucentQNum{0};
	inline static MinMaxTracker<uint32> TextQNum{0};
	inline static MinMaxTracker<uint32> InstancingQNum{0};
	inline static MinMaxTracker<uint32> InstanceNum{0};

	inline static bool bIsInitialized = false;

	//static void Initialize()
	//{
	//	UObjectArrayNum.Reset(STATS.GetUObjectArrayNum());
	//	DrawCallNum.Reset(STATS.GetDrawCallNum());
	//	DrawTriangleCount.Reset(STATS.GetDrawTriangleCount());
	//	DrawVertexCount.Reset(STATS.GetDrawVertexCount());
	//	LineBatchNum.Reset(STATS.GetLineBatchNum());
	//	OpaqueQNum.Reset(STATS.GetOpaqueQNum());
	//	TranslucentQNum.Reset(STATS.GetTranslucentQNum());
	//	TextQNum.Reset(STATS.GetTextQNum());
	//	InstancingQNum.Reset(STATS.GetInstancingQNum());
	//	InstanceNum.Reset(STATS.GetInstanceNum());
	//}

	const void Process(const FEditor InEditor) const
	{
		//if (!bIsInitialized)
		//{
		//	Initialize();
		//	bIsInitialized = true;
		//}

		const ImGuiViewport* VP = ImGui::GetMainViewport();
		//ImGui::SetNextWindowPos(ImVec2(InEditor.CenterUV.X, InEditor.CenterUV.Y));
		ImGui::SetNextWindowBgAlpha(0.f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);  // 창 테두리 제거
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6, 1));

		// 표 배경색 (반투명)
		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(0, 0, 0, 0)); // 헤더 밑선, 바깥 테두리
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, IM_COL32(0, 0, 0, 0)); // 안쪽 행·열 구분선
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, IM_COL32(10, 10, 10, 210));
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, IM_COL32(5, 5, 5, 210));
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255)); // 초록 글씨

		ImGuiWindowFlags Flags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_AlwaysAutoResize
			| ImGuiWindowFlags_NoBringToFrontOnFocus;
			//| ImGuiWindowFlags_NoSavedSettings
			//| ImGuiWindowFlags_NoFocusOnAppearing
			//| ImGuiWindowFlags_NoNav
			//| ImGuiWindowFlags_NoMove
			//| ImGuiWindowFlags_NoInputs; 

		ImGui::Begin("Stat Memory", nullptr, Flags);

		//ImGui::Text("Cycle counters");
		if (ImGui::BeginTable("Cycle counters", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Cycle counters (flat)", ImGuiTableColumnFlags_WidthFixed, 350.f);
			ImGui::TableSetupColumn("CallCount", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("InclusiveAvg", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("InclusiveMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("ExclusiveAvg", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("ExclusiveMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 0, 255));
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			for (uint32 Row = 0; Row < 5; Row++)
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

		//ImGui::Text("Memory counters");
		if (ImGui::BeginTable("Memory counters", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Memory Counters", ImGuiTableColumnFlags_WidthFixed, 350.f);
			ImGui::TableSetupColumn("UsedMax", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Mem%", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("MemPool", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Pool Capacity", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 0, 255));
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("UObject Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetAllocationBytes()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("StaticMesh Total Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetStaticMeshMemory()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("StaticMesh Index Buffer");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetStaticIndexBufferSize()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("StaticMesh Vertex Buffer");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetStaticVertexBufferSize()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Line Batch Total Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetLineBatchByte()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Texture Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetTextureByte()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Editor Texture Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetEditorTextureByte()));

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Thumbnail Texture Memory");
			ImGui::TableNextColumn(); ImGui::Text("%.2f KB", ByteToMB(STATS.GetMeshThumbnailByte()));

			ImGui::EndTable();
		}

		//ImGui::Text("Live Counters");
		if (ImGui::BeginTable("Render Counters", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Counters", ImGuiTableColumnFlags_WidthFixed, 350.f);
			ImGui::TableSetupColumn("Current", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("Min", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 0, 255));
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Live UObjects");
			ImGui::TableNextColumn(); ImGui::Text("%u", UObjectArrayNum.Update(STATS.GetUObjectArrayNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", UObjectArrayNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", UObjectArrayNum.Min);

			//ImGui::TableNextRow();//
			//ImGui::TableNextColumn(); ImGui::Text("UObjects Alloc Count");
			//ImGui::TableNextColumn(); ImGui::Text("%u", STATS.GetAllocationCount());

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Draw Calls");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawCallNum.Update(STATS.GetDrawCallNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawCallNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawCallNum.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Triangles Rendered");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawTriangleCount.Update(STATS.GetDrawTriangleCount()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawTriangleCount.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawTriangleCount.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Vertices Rendered");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawVertexCount.Update(STATS.GetDrawVertexCount()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawVertexCount.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", DrawVertexCount.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Lines Rendered");
			ImGui::TableNextColumn(); ImGui::Text("%u", LineBatchNum.Update(STATS.GetLineBatchNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", LineBatchNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", LineBatchNum.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Opaque Queue");
			ImGui::TableNextColumn(); ImGui::Text("%u", OpaqueQNum.Update(STATS.GetOpaqueQNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", OpaqueQNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", OpaqueQNum.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Translucent Queue");
			ImGui::TableNextColumn(); ImGui::Text("%u", TranslucentQNum.Update(STATS.GetTranslucentQNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", TranslucentQNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", TranslucentQNum.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Text Queue");
			ImGui::TableNextColumn(); ImGui::Text("%u", TextQNum.Update(STATS.GetTextQNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", TextQNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", TextQNum.Min);

			ImGui::TableNextRow();//
			ImGui::TableNextColumn(); ImGui::Text("Instancing Queue");
			ImGui::TableNextColumn(); ImGui::Text("%u", InstancingQNum.Update(STATS.GetInstancingQNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", InstancingQNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", InstancingQNum.Min);

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Total Instance");
			ImGui::TableNextColumn(); ImGui::Text("%u", InstanceNum.Update(STATS.GetInstanceNum()));
			ImGui::TableNextColumn(); ImGui::Text("");
			ImGui::TableNextColumn(); ImGui::Text("%u", InstanceNum.Max);
			ImGui::TableNextColumn(); ImGui::Text("%u", InstanceNum.Min);

			ImGui::EndTable();
		}

		//ImGui::Text("Static Counters");
		if (ImGui::BeginTable("Static Counters", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX))
		{
			ImGui::TableSetupColumn("Static Counters", ImGuiTableColumnFlags_WidthFixed, 350.f);
			ImGui::TableSetupColumn("Counts", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 100.f);
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 40, 0, 255));
			ImGui::TableHeadersRow();
			ImGui::PopStyleColor();

			ImGui::TableNextRow();
			ImGui::TableNextColumn(); ImGui::Text("Pipeline Count");
			ImGui::TableNextColumn(); ImGui::Text("%u", STATS.GetPipelineNum());

			ImGui::EndTable();
		}
		ImGui::PopStyleColor(6);
		ImGui::PopStyleVar(3);
		ImGui::End();
	}

	static float ByteToMB(uint32 Byte) 
	{
		return static_cast<float>(Byte) / (1024.f * 1024.f);
	}
	static float ByteToKB(uint32 Byte)
	{
		return static_cast<float>(Byte) / (1024.f);
	}
};