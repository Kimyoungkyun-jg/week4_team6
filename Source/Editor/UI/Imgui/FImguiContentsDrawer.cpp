#include "FImguiContentsDrawer.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FTexture.h"
#include "ThirdParty/stb/stb_image.h"
#include <algorithm>
#include <cctype>
#include "FImguiDragDrop.h"
#include "Editor/Application/FEditorApplication.h"
FImguiContentsDrawer::FImguiContentsDrawer() : LeftPanelWidth(200.0f)
{
	RootPath = std::filesystem::current_path() / "Resources";
	CurrentPath = RootPath;

}

void FImguiContentsDrawer::Process(FEditor& Editor)
{
	ImGui::Begin("Content Drawer");

	// GetContentRegionAvail은 Begin 다음에 불러야 이 창의 남은 영역이 나온다.
	// Begin 이전에 부르면 직전 창의 값이라 자식 패널이 창 밖으로 삐져나간다.
	ImVec2 contentSize = ImGui::GetContentRegionAvail();

	ImGui::BeginChild("LeftPanel", ImVec2(LeftPanelWidth, contentSize.y), true);
	RenderFolderTree();
	ImGui::EndChild();

	ImGui::SameLine();

	// 폭 0은 남은 공간을 전부 쓰라는 뜻
	ImGui::BeginChild("RightPanel", ImVec2(0.0f, contentSize.y), true);
	RenderContentView();
	ImGui::EndChild();

	ImGui::End();

}

void FImguiContentsDrawer::RefreshEntries()
{
	Entries.clear();
	CachedPath = CurrentPath;
	bNeedsRefresh = false;

	// StaticMesh 폴더일 때는 라이브러리의 썸네일 맵 기준 목록 표시
	if (CurrentPath.filename() == "StaticMesh")
	{
		// 디스크에 StaticMesh 폴더가 없으면 실제 디렉터리 생성
		std::error_code Ec;
		if (!std::filesystem::exists(CurrentPath, Ec))
		{
			std::filesystem::create_directories(CurrentPath, Ec);
		}

		for (const auto& [Key, Thumb] : FRenderResourceLibrary::Get().GetAllMeshThumbnailMap())
		{
			FContentEntry Item;
			Item.DisplayName = Key.ToString();
			Item.Extension = ".staticmesh";
			Item.bIsDirectory = false;
			Item.Path = CurrentPath / (Key.ToString() + ".staticmesh");
			Entries.push_back(std::move(Item));
		}
		std::sort(Entries.begin(), Entries.end(),
			[](const FContentEntry& A, const FContentEntry& B)
			{
				return A.DisplayName < B.DisplayName;
			});
		return;
	}


	if (CurrentPath.filename() == "Materials")
	{
		std::error_code Ec;
		if (!std::filesystem::exists(CurrentPath, Ec))
		{
			std::filesystem::create_directories(CurrentPath, Ec);
		}

		for (const auto& [Key, Thumb] : FRenderResourceLibrary::Get().GetAllMaterialThumbnailMap())
		{
			FContentEntry Item;
			Item.DisplayName = Key;
			Item.Extension = ".material";
			Item.bIsDirectory = false;
			Item.Path = CurrentPath / (Key + ".material");
			Entries.push_back(std::move(Item));
		}
		std::sort(Entries.begin(), Entries.end(),
			[](const FContentEntry& A, const FContentEntry& B)
			{
				return A.DisplayName < B.DisplayName;
			});
		return;
	}



	std::error_code Ec;
	for (const auto& Entry : std::filesystem::directory_iterator(CurrentPath, Ec))
	{
		FContentEntry Item;
		Item.Path = Entry.path();
		Item.bIsDirectory = Entry.is_directory(Ec);
		Item.DisplayName = WideToUTF8(Entry.path().filename().wstring());

		if (!Item.bIsDirectory)
		{
			Item.Extension = WideToUTF8(Entry.path().extension().wstring());
			std::transform(Item.Extension.begin(), Item.Extension.end(), Item.Extension.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		}

		Entries.push_back(std::move(Item));
	}

	// 폴더 먼저, 그 다음 파일. 각각 이름순.
	std::sort(Entries.begin(), Entries.end(),
		[](const FContentEntry& A, const FContentEntry& B)
		{
			if (A.bIsDirectory != B.bIsDirectory) { return A.bIsDirectory; }
			return A.DisplayName < B.DisplayName;
		});
}

TSharedPtr<FTexture> FImguiContentsDrawer::GetOrLoadThumbnail(const FContentEntry& Item)
{
	if (Item.bIsDirectory)
	{
		return nullptr;
	}
	if (Item.Extension != ".dds" && Item.Extension != ".png" && Item.Extension != ".jpg" && Item.Extension != ".jpeg")
	{
		return nullptr;
	}

	// 라이브러리 키는 소문자 stem이다. (FRenderResourceLibrary::CreateTextures와 동일)
	FString Key = Item.Path.stem().string();
	std::transform(Key.begin(), Key.end(), Key.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	FRenderResourceLibrary& Lib = FRenderResourceLibrary::Get();
	if (TSharedPtr<FTexture> Existing = Lib.GetTexture(Key))
	{
		return Existing;
	}

	// 아직 없으면 디스크에서 읽는다. 프레임당 개수를 제한해 멈춤을 막는다.
	if (LoadsThisFrame >= MaxLoadsPerFrame)
	{
		return nullptr;
	}

	FRenderer* Renderer = Lib.GetRenderer();
	if (!Renderer)
	{
		return nullptr;
	}

	++LoadsThisFrame;

	TSharedPtr<FTexture> Texture = Renderer->CreateTexture(Item.Path.wstring().c_str());
	//stbi_image_free(Pixels);

	// 성공이든 실패든 등록해 둔다. 실패면 nullptr이 캐시되어 재시도를 막는다.
	Lib.RegisterTexture(Key, Texture);
	return Texture;
}

void FImguiContentsDrawer::RenderContentView()
{
	LoadsThisFrame = 0;

	// 좌측 트리에서 폴더를 바꿨으면 다시 읽는다.
	if (bNeedsRefresh || CachedPath != CurrentPath)
	{
		RefreshEntries();
	}

	// ---------------- 상단 바 ----------------
	// 루트 기준 상대 경로를 보여준다.
	const std::filesystem::path Relative = std::filesystem::relative(CurrentPath, RootPath.parent_path());
	ImGui::TextUnformatted(WideToUTF8(Relative.wstring()).c_str());

	ImGui::SameLine();
	if (ImGui::SmallButton("Refresh")) { bNeedsRefresh = true; }

	if (CurrentPath != RootPath)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Up")) { CurrentPath = CurrentPath.parent_path(); }
	}


	ImGui::Separator();

	if (Entries.empty())
	{
		ImGui::TextDisabled("비어 있습니다.");
		return;
	}

	// ---------------- 타일 그리드 ----------------
	const ImGuiStyle& Style = ImGui::GetStyle();
	const float TileWidth = ThumbnailSize + Style.ItemSpacing.x;
	const float Avail = ImGui::GetContentRegionAvail().x;

	// 패널을 좁히면 0이 되어 나눗셈이 깨지므로 최소 1로 막는다.
	int Columns = static_cast<int>(Avail / TileWidth);
	if (Columns < 1) { Columns = 1; }

	// 폴더 진입은 순회 중에 CurrentPath를 바꾸면 안 되므로 따로 모아 뒀다가 끝나고 적용한다.
	std::filesystem::path PendingNavigate;

	for (int Index = 0; Index < static_cast<int>(Entries.size()); ++Index)
	{
		const FContentEntry& Item = Entries[Index];

		// 같은 이름이 있어도 ID가 겹치지 않도록 인덱스로 구분한다.
		ImGui::PushID(Index);
		ImGui::BeginGroup();

		const bool bSelected = (SelectedPath == Item.Path);

		const TSharedPtr<FTexture> Thumbnail = GetOrLoadThumbnail(Item);

		// 썸네일 SRV 결정
		ID3D11ShaderResourceView* DisplaySRV = nullptr;

		if (Item.bIsDirectory)
		{
			if (auto FolderTex = FRenderResourceLibrary::Get().GetEditTexture("foldericon"))
			{
				DisplaySRV = FolderTex->GetSRV();
			}
		}
		else if (Item.Extension == ".staticmesh")
		{
			// 라이브러리에서 3D 스냅샷 썸네일 조회
			const FName MeshKey(Item.DisplayName);
			if (auto MeshTex = FRenderResourceLibrary::Get().GetMeshThumbnail(MeshKey))
			{
				DisplaySRV = MeshTex->GetSRV();
			}
		}
		else if (Item.Extension == ".material")
		{
			// 머티리얼 썸네일 SRV 조회
			if (auto MatTex = FRenderResourceLibrary::Get().GetMaterialThumbnail(Item.DisplayName))
			{
				DisplaySRV = MatTex->GetSRV();
			}
		}
		else
		{
			if (const TSharedPtr<FTexture> Thumbnail = GetOrLoadThumbnail(Item))
			{
				DisplaySRV = Thumbnail->GetSRV();
			}
		}

		if (DisplaySRV)          
		{
			// 선택 상태를 배경색으로 표시
			const ImGuiStyle& S = ImGui::GetStyle();
			ImGui::PushStyleColor(ImGuiCol_Button,
				bSelected ? S.Colors[ImGuiCol_ButtonActive] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

			const ImTextureID TexId =
				static_cast<ImTextureID>(reinterpret_cast<intptr_t>(DisplaySRV));

			if (ImGui::ImageButton("##thumb", TexId, ImVec2(ThumbnailSize, ThumbnailSize)))
			{
				SelectedPath = Item.Path;
			}

			ImGui::PopStyleColor();
		}
		else
		{
			// 이미지가 아니거나 아직 로드 전이면 종류를 글자로 보여준다.
			const char* Caption = Item.bIsDirectory
				? "[DIR]"
				: (Item.Extension.empty() ? "FILE" : Item.Extension.c_str() + 1);

			if (ImGui::Selectable(Caption, bSelected, ImGuiSelectableFlags_AllowDoubleClick,
				ImVec2(ThumbnailSize, ThumbnailSize)))
			{
				SelectedPath = Item.Path;
			}
		}

		if (!Item.bIsDirectory && ImGui::BeginDragDropSource())
		{
			FContentDragPayload DragData;
			DragData.Kind = (Item.Extension == ".staticmesh") ? FContentDragPayload::EKind::Mesh
				: (DisplaySRV ? FContentDragPayload::EKind::Texture : FContentDragPayload::EKind::Unknown);

			const FString PathUtf8 = WideToUTF8(Item.Path.wstring());
			std::snprintf(DragData.Path, sizeof(DragData.Path), "%s", PathUtf8.c_str());

			FString Key = Item.DisplayName;
			std::snprintf(DragData.Key, sizeof(DragData.Key), "%s", Key.c_str());

			ImGui::SetDragDropPayload(ContentDragPayloadType, &DragData, sizeof(DragData));

			// 드래그 중 미리보기
			if (DisplaySRV)
			{
				const ImTextureID PreviewId =
					static_cast<ImTextureID>(reinterpret_cast<intptr_t>(DisplaySRV));
				ImGui::Image(PreviewId, ImVec2(48.0f, 48.0f));
				ImGui::SameLine();
			}
			ImGui::TextUnformatted(Item.DisplayName.c_str());

			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered() && !ImGui::IsDragDropActive())
		{
			ImGui::SetTooltip("%s", Item.DisplayName.c_str());
		}

		// 더블클릭 처리
		if (Item.bIsDirectory && ImGui::IsItemHovered() &&
			ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			PendingNavigate = Item.Path;
		}
		else if (!Item.bIsDirectory && ImGui::IsItemHovered() &&
			ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (Item.Extension == ".staticmesh" || Item.Extension == ".obj")
			{
				const FName MeshId(Item.DisplayName);
				UStaticMesh* Mesh = FRenderResourceLibrary::Get().GetUStaticMesh(MeshId);
				if (Mesh)
				{
					FEditorApplication::Get().OpenPreviewWindow(Mesh);
				}
			}
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", Item.DisplayName.c_str());
		}

		// 이름이 길면 썸네일 폭 안에서 줄바꿈한다.
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ThumbnailSize);
		ImGui::TextUnformatted(Item.DisplayName.c_str());
		ImGui::PopTextWrapPos();

		ImGui::EndGroup();
		ImGui::PopID();

		// 행의 마지막이 아니면 옆에 붙인다.
		if ((Index + 1) % Columns != 0 && Index + 1 < static_cast<int>(Entries.size()))
		{
			ImGui::SameLine();
		}
	}

	if (!PendingNavigate.empty())
	{
		CurrentPath = PendingNavigate;
	}
}

void FImguiContentsDrawer::RenderFolderTree()
{
	ImGui::Text("Folders");
	ImGui::Separator();

	// 루트 폴더부터 재귀적으로 렌더링
	if (std::filesystem::exists(RootPath))
	{
		RenderFolderTreeNode(RootPath);
	}
}

void FImguiContentsDrawer::RenderFolderTreeNode(const std::filesystem::path& FolderPath)
{

	FString folderName = FolderPath == RootPath ? "All" : WideToUTF8(FolderPath.filename().wstring());

	// 하위 폴더가 있는지 먼저 확인한다.
	// 없으면 잎 노드로 만들어 열리지 않는 화살표가 생기지 않게 한다.
	// 접근 권한 문제로 던지지 않도록 error_code 버전을 쓴다.
	std::error_code Ec;
	bool bHasSubFolder = false;
	for (const auto& Entry : std::filesystem::directory_iterator(FolderPath, Ec))
	{
		if (Entry.is_directory(Ec))
		{
			bHasSubFolder = true;
			break;
		}
	}

	ImGuiTreeNodeFlags Flags =
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (!bHasSubFolder)
	{
		// NoTreePushOnOpen을 같이 주면 TreePop을 부르지 않아도 된다.
		Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (CurrentPath == FolderPath)
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (FolderPath == RootPath)
	{
		Flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	// 이름이 같은 폴더가 여러 곳에 있을 수 있으므로 전체 경로를 ID로 쓴다.
	ImGui::PushID(WideToUTF8(FolderPath.wstring()).c_str());

	const bool bOpened = ImGui::TreeNodeEx(folderName.c_str(), Flags);

	// 화살표를 눌러 접고 펴는 것과 폴더를 선택하는 것을 구분한다.
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		CurrentPath = FolderPath;
	}

	if (bOpened && bHasSubFolder)
	{
		for (const auto& Entry : std::filesystem::directory_iterator(FolderPath, Ec))
		{
			if (Entry.is_directory(Ec))
			{
				RenderFolderTreeNode(Entry.path());
			}
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}


