#include "FImguiPropertyWindow.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Actors/AActor.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include <string>
#include "FImguiDragDrop.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"

void FImguiPropertyWindow::Process(FEditor& Editor)
{
	ImGui::Begin("Jungle Property Window");

	if (AActor* SelectedActor = Editor.GetSelectedActor())
	{
		ShowActorHeader(*SelectedActor);
		ImGui::Separator();

		ShowComponentHierarchy(*SelectedActor);
		ImGui::Separator();

		ShowComponentSections(Editor, *SelectedActor);
	}
	else
	{
		ImGui::TextDisabled("No selection");
	}

	ShowGizmoSettings(Editor);

	ImGui::End();
}

void FImguiPropertyWindow::ShowActorHeader(const AActor& Actor) const
{
	const char* ActorClassName = Actor.GetClass() ? Actor.GetClass()->GetDisplayName().c_str() : "None";
	ImGui::Text("Actor Class: %s", ActorClassName);
	ImGui::Text("Actor UUID: %u", Actor.GetUUID());
}

void FImguiPropertyWindow::ShowComponentHierarchy(const AActor& Actor) const
{
	ImGui::TextDisabled("Components Hierarchy");

	const USceneComponent* RootComp = Actor.GetRootComponent();
	if (RootComp)
	{
		const char* RootName = RootComp->GetClass() ? RootComp->GetClass()->GetDisplayName().c_str() : "RootComponent";
		ImGui::BulletText("[Root] %s (ID: %u)", RootName, RootComp->GetUUID());
	}

	for (const USceneComponent* Comp : Actor.GetAttachedComponents())
	{
		if (!Comp || Comp == RootComp)
		{
			continue;
		}

		const char* SubName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "SubComponent";
		ImGui::Indent(15.0f);
		ImGui::BulletText("└── [Sub] %s (ID: %u)", SubName, Comp->GetUUID());
		ImGui::Unindent(15.0f);
	}
}

void FImguiPropertyWindow::ShowComponentSections(FEditor& Editor, AActor& Actor)
{
	USceneComponent* RootComp = Actor.GetRootComponent();

	for (USceneComponent* Comp : Actor.GetAttachedComponents())
	{
		if (!Comp)
		{
			continue;
		}

		const bool bIsRoot = (Comp == RootComp);
		const char* CompTypeName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";

		// ### 뒤쪽이 실제 ID 라서, 앞의 표시 이름이 바뀌어도 접힘 상태가 유지된다.
		std::string SectionTitle = (bIsRoot ? "[Root] " : "[Sub] ") + std::string(CompTypeName)
			+ " (ID: " + std::to_string(Comp->GetUUID()) + ")###CompHeader_" + std::to_string(Comp->GetUUID());

		if (!ImGui::CollapsingHeader(SectionTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}

		// 컴포넌트마다 위젯 ID 를 분리해야 같은 라벨끼리 충돌하지 않는다.
		ImGui::PushID(Comp);
		ShowComponentDetails(Editor, Actor, *Comp, bIsRoot);
		ImGui::PopID();

		ImGui::Spacing();
	}
}

void FImguiPropertyWindow::ShowComponentDetails(FEditor& Editor, AActor& Actor,
	USceneComponent& Comp, bool bIsRoot)
{
	ShowTransform(Editor, Comp, bIsRoot);

	if (Comp.IsA<UTextInstanceComponent>())
	{
		ShowTextSettings(static_cast<UTextInstanceComponent&>(Comp));
	}

	if (Comp.IsA<USpotLightComponent>())
	{
		ShowSpotLightSettings(static_cast<USpotLightComponent&>(Comp));
	}
	else if (Comp.IsA<UPrimitiveComponent>())
	{
		ShowPrimitiveSettings(Actor, static_cast<UPrimitiveComponent&>(Comp), bIsRoot);
	}
}

void FImguiPropertyWindow::ShowTransform(FEditor& Editor, USceneComponent& Comp, bool bIsRoot) const
{
	ImGui::TextDisabled("Transform");

	if (bIsRoot)
	{
		// 루트 컴포넌트 트랜스폼은 에디터 기즈모와 동기화
		ImGui::DragFloat3("Translation", &Editor.SelectedTransform.Location.X, 0.01f);
		if (ImGui::DragFloat3("Rotation (deg)", &Editor.SelectedEulerDegDisplay.X, 0.5f))
		{
			Editor.SelectedTransform.Rotation = FQuaternion::FromEulerXYZDeg(Editor.SelectedEulerDegDisplay);
		}
		ImGui::DragFloat3("Scale", &Editor.SelectedTransform.Scale3D.X, 0.01f);
		return;
	}

	// 서브 컴포넌트 상대 트랜스폼 편집
	FTransform& RelTransform = Comp.GetRelativeTransform();
	ImGui::DragFloat3("Rel Location", &RelTransform.Location.X, 0.01f);

	FVector RelEuler = RelTransform.Rotation.ToEulerXYZDeg();
	if (ImGui::DragFloat3("Rel Rotation (deg)", &RelEuler.X, 0.5f))
	{
		RelTransform.Rotation = FQuaternion::FromEulerXYZDeg(RelEuler);
	}
	ImGui::DragFloat3("Rel Scale", &RelTransform.Scale3D.X, 0.01f);
}

void FImguiPropertyWindow::ShowTextSettings(UTextInstanceComponent& TextComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Text Settings");


	const char* fontItems[] = { "bazziotf", "dnfbitbitv2", "maplestorybold" };
	static int currFontIndex = 0;
	if (ImGui::Combo("Font", &currFontIndex, fontItems, IM_ARRAYSIZE(fontItems)))
	{
		const FName Materials[] = { FName("Instance_Text_Bazzi"), FName("Instance_Text_DNF"), FName("Instance_Text_Maple") };
		const char* selectedFont = fontItems[currFontIndex];
		TextComp.SetMaterialID((Materials[currFontIndex]));
		TextComp.SetFont(FName(selectedFont));
	}

	static char utfBuffer[512]{};
	WideCharToMultiByte(CP_UTF8, 0, TextComp.GetText().c_str(), -1, &utfBuffer[0], sizeof(utfBuffer), NULL, NULL);

	if (ImGui::InputText("Text Content", &utfBuffer[0], sizeof(utfBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		FString Buffer{ &utfBuffer[0] };
		uint32 convertResult = MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), NULL, 0);
		FWString newText(convertResult, 0);
		MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), newText.data(), convertResult);
		TextComp.SetText(newText);
	}
}

void FImguiPropertyWindow::ShowSpotLightSettings(USpotLightComponent& LightComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Spot Light Settings");

	FVector LightCol = LightComp.GetLightColor();
	if (ImGui::ColorEdit3("Light Color", &LightCol.X))
	{
		LightComp.SetLightColor(LightCol);
	}

	float LightIntensity = LightComp.GetIntensity();
	if (ImGui::DragFloat("Intensity", &LightIntensity, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetIntensity(LightIntensity);
	}

	float SpotAngle = LightComp.GetSpotAngle();
	if (ImGui::SliderFloat("Spot Angle", &SpotAngle, 1.0f, 89.0f))
	{
		LightComp.SetSpotAngle(SpotAngle);
	}

	float LightRange = LightComp.GetRange();
	if (ImGui::DragFloat("Range", &LightRange, 0.1f, 0.1f, 100.0f))
	{
		LightComp.SetRange(LightRange);
	}
}

void FImguiPropertyWindow::ShowPrimitiveSettings(AActor& Actor, UPrimitiveComponent& PrimComp,
	bool bIsRoot) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Primitive Settings");

	FVector CurrentColor = PrimComp.GetColor();
	if (ImGui::ColorEdit3("Color", &CurrentColor.X))
	{
		PrimComp.SetColor(CurrentColor);
		if (bIsRoot)
		{
			Actor.SetColor(CurrentColor);
		}
	}

	ShowTextureSlot(PrimComp);
}

void FImguiPropertyWindow::ShowTextureSlot(UPrimitiveComponent& PrimComp) const
{
	constexpr float SlotSize = 64.0f;
	TSharedPtr<FMaterial> Material = FRenderResourceLibrary::Get().GetMaterial(PrimComp.GetPureRenderData().MaterialId);
	TSharedPtr<FTexture> CurrentTexture = Material ? Material->GetTexture() : nullptr;

	ImGui::Spacing();
	ImGui::TextDisabled("Texture");

	if (CurrentTexture && CurrentTexture->GetSRV())
	{
		// ImGui 1.93의 ImTextureID는 ImU64라서 포인터를 정수로 한 번 거친다.
		const ImTextureID TexId = static_cast<ImTextureID>(
			reinterpret_cast<intptr_t>(CurrentTexture->GetSRV()));
		ImGui::Image(TexId, ImVec2(SlotSize, SlotSize));
	}
	else
	{
		// 비어 있어도 드롭받을 아이템은 있어야 하므로 자리를 만든다.
		ImGui::Button("No\nTexture", ImVec2(SlotSize, SlotSize));
	}

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget())
	{
		return;
	}

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		// 타입 이름이 같아도 크기가 다르면 다른 구조체일 수 있다.
		if (Material && Payload->DataSize == static_cast<int>(sizeof(FContentDragPayload)))
		{
			const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

			if (Dropped->Kind == FContentDragPayload::EKind::Texture)
			{
				Material->SetTextureByName(Dropped->Key);
			}
		}
	}
	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowGizmoSettings(FEditor& Editor) const
{
	static const char* GizmoModes[4] = { "None", "Translation", "Rotation", "Scale" };
	int SelectedItem = static_cast<int>(Editor.GetGizmo().Mode);
	if (ImGui::Combo("Gizmo Mode", &SelectedItem, GizmoModes, 4))
	{
		Editor.GetGizmo().Mode = static_cast<EGizmoMode>(SelectedItem);
	}

	if (SelectedItem == 3) // Scale
	{
		static const char* GizmoSpaces[] = { "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace()) - 1;
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 1))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem - 1));
		}
	}
	else if (SelectedItem != 0) // Translation, Rotation
	{
		static const char* GizmoSpaces[] = { "World", "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace());
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 2))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem));
		}
	}
}
