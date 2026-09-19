#include "FObjDecoder.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Utility/EngineUtil.h"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

TSortedMap<FString, FObjModelData*> FObjDecoder::ObjStaticMeshMap;

namespace
{
	// 에셋 폴더는 실행 파일 기준으로 잡는다.
	std::filesystem::path GetAssetDir()
	{
		const std::filesystem::path ExeDir(GetExecutableDirectory());
		return ExeDir.parent_path().parent_path().parent_path() / "Resources" / "Assets";
	}

	bool IsUnder(const std::filesystem::path& TargetPath, const std::filesystem::path& BasePath)
	{
		std::error_code Ec;
		const auto NormalizedFile = std::filesystem::weakly_canonical(TargetPath, Ec);
		const auto NormalizedRoot = std::filesystem::weakly_canonical(BasePath, Ec);

		const auto RelativePath = std::filesystem::relative(NormalizedFile, NormalizedRoot, Ec);
		return !RelativePath.empty() && RelativePath.begin()->string() != "..";
	}

	bool ResolveExistingFile(std::string_view FileName, std::filesystem::path& OutPath)
	{
		std::filesystem::path FilePath(FileName);

		if (!FilePath.is_absolute())
		{
			FilePath = GetAssetDir() / FilePath;
		}

		std::error_code Ec;
		if (!std::filesystem::is_regular_file(FilePath, Ec))
		{
			return false;
		}

		OutPath = FilePath;
		return true;
	}

	FString ReadFileToString(std::string_view FileName)
	{
		
		std::filesystem::path FilePath(FileName);

		if (!FilePath.is_absolute())
		{
			const std::filesystem::path AssetDir = GetAssetDir();
			FilePath = AssetDir / FileName;

			if (!IsUnder(FilePath, AssetDir))
			{
				throw std::runtime_error("Attempted to read outside of the asset directory: " + FilePath.string());
			}
		}

		std::ifstream FileStream(FilePath, std::ios::in);
		if (!FileStream.is_open())
		{
			throw std::runtime_error("Failed to open file for reading: " + FilePath.string());
		}

		std::stringstream Buffer;
		Buffer << FileStream.rdbuf();
		return Buffer.str();
	}

	constexpr std::string_view Spaces = " \t\r\n";

	std::string_view Trim(std::string_view Text)
	{
		const size_t First = Text.find_first_not_of(Spaces);
		if (First == std::string_view::npos)
		{
			return {};
		}

		const size_t Last = Text.find_last_not_of(Spaces);
		return Text.substr(First, Last - First + 1);
	}

	// 앞 공백을 건너뛰고 첫 단어를 반환한다. Text 에서는 그 단어까지 제거된다.
	std::string_view NextWord(std::string_view& Text)
	{
		const size_t Start = Text.find_first_not_of(Spaces);
		if (Start == std::string_view::npos)
		{
			Text = {};
			return {};
		}

		const size_t End = Text.find_first_of(Spaces, Start);
		const std::string_view Word = Text.substr(Start, End - Start);
		Text = (End == std::string_view::npos) ? std::string_view{} : Text.substr(End);
		return Word;
	}

	bool StringToFloat(std::string_view Text, float& Value)
	{
		const auto [Ptr, Ec] = std::from_chars(Text.data(), Text.data() + Text.size(), Value);
		return Ec == std::errc{} && Ptr == Text.data() + Text.size();
	}

	bool StringToInt(std::string_view Text, int32& Value)
	{
		const auto [Ptr, Ec] = std::from_chars(Text.data(), Text.data() + Text.size(), Value);
		return Ec == std::errc{} && Ptr == Text.data() + Text.size();
	}

	int32 ReadFloats(std::string_view Line, float* Out, int32 MaxCount)
	{
		int32 Count = 0;
		while (Count < MaxCount)
		{
			const std::string_view Word = NextWord(Line);
			if (Word.empty())
			{
				break;
			}
			if (!StringToFloat(Word, Out[Count]))
			{
				break;
			}
			++Count;
		}
		return Count;
	}

	// OBJ 인덱스는 1부터 시작해서 1빼준다 음수는 끝에서부터 센 상대 인덱스. 0 은 없음(-1 반환).
	int32 ToZeroBased(int32 ObjIndex, size_t ListSize)
	{
		if (ObjIndex > 0)
		{
			return ObjIndex - 1;
		}
		if (ObjIndex < 0)
		{
			return static_cast<int32>(ListSize) + ObjIndex;
		}
		return -1;
	}

	// "v", "v/vt", "v//vn", "v/vt/vn" 하나를 파싱한다. 없는 항목은 0 으로 남는다.
	bool ParseFaceToken(std::string_view Token, int32& V, int32& VT, int32& VN)
	{
		V = VT = VN = 0;
		int32* Slots[3] = { &V, &VT, &VN };

		for (int32 i = 0; i < 3 && !Token.empty(); ++i)
		{
			const size_t Slash = Token.find('/');
			const std::string_view Part = Token.substr(0, Slash);

			if (!Part.empty() && !StringToInt(Part, *Slots[i]))
			{
				return false;
			}

			if (Slash == std::string_view::npos)
			{
				break;
			}
			Token.remove_prefix(Slash + 1);
		}

		return V != 0;
	}

	std::string_view NextLine(std::string_view& Remaining)
	{
		const size_t NewLine = Remaining.find('\n');
		if (NewLine == std::string_view::npos)
		{
			const std::string_view Line = Remaining;
			Remaining = {};
			return Line;
		}

		const std::string_view Line = Remaining.substr(0, NewLine);
		Remaining.remove_prefix(NewLine + 1);
		return Line;
	}

	// 옵션 인자로 볼 수 있는 토큰인지 (숫자 또는 on/off)
	bool IsTextureOptionArg(std::string_view Word)
	{
		float Dummy = 0.0f;
		return Word == "on" || Word == "off" || StringToFloat(Word, Dummy);
	}

	// map_Kd 등의 텍스처 경로. "-s 1 1 1 -o 0 0 0 file name.png" 처럼 앞에 붙은
	// 옵션 그룹(-이름 + 인자들)을 건너뛰고 남은 전체를 경로로 본다. 공백 포함 파일명도 유지된다.
	FString ParseTexturePath(std::string_view Line)
	{
		Line = Trim(Line);

		while (!Line.empty() && Line.front() == '-')
		{
			const std::string_view Option = NextWord(Line);

			// -type / -imfchan 은 인자가 단어 하나 (sphere, r, g, ...)
			if (Option == "-type" || Option == "-imfchan")
			{
				NextWord(Line);
			}
			else
			{
				// 나머지 옵션은 숫자/on/off 인자를 0개 이상 가진다
				while (true)
				{
					std::string_view Peek = Line;
					const std::string_view Arg = NextWord(Peek);
					if (Arg.empty() || !IsTextureOptionArg(Arg))
					{
						break;
					}
					Line = Peek;
				}
			}

			Line = Trim(Line);
		}

		return FString(Line);
	}


	
}

// v x y z [w] [r g b]
void FObjDecoder::AddVertexList(std::string_view Line)
{
	float Values[7] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	const int32 Count = ReadFloats(Line, Values, 7);

	if (Count < 3)
	{
		// 인덱스가 밀리지 않도록 자리는 채우고 오류만 남긴다.
		ObjInfo.VertexList.push_back(FVector4(0.0f, 0.0f, 0.0f, 1.0f));
		UE_LOG_WARN("FObjDecoder : AddVertexList Error (least 3 floats)");
		return;
	}

	ObjInfo.VertexList.push_back(FVector4(Values[0], Values[1], Values[2], Values[3]));

	// 색상은 x y z r g b (6개) 또는 x y z w r g b (7개) 형식일 때만 존재
	if (Count == 6)
	{
		ObjInfo.ColorList.push_back(FVector(Values[3], Values[4], Values[5]));
		ObjInfo.VertexList.back().W = 1.0f;
	}
	else if (Count == 7)
	{
		ObjInfo.ColorList.push_back(FVector(Values[4], Values[5], Values[6]));
	}
}


// vt u [v] [w]

void FObjDecoder::AddUVList(std::string_view Line)
{
	float Values[3] = { 0.0f, 0.0f, 0.0f };
	const int32 Count = ReadFloats(Line, Values, 3);

	if (Count < 1)
	{
		ObjInfo.UVList.push_back(FVector(0.0f, 0.0f, 0.0f));
		UE_LOG_WARN("FObjDecoder : AddUVList Error (need at least 1 float)");
		return;
	}

	ObjInfo.UVList.push_back(FVector(Values[0], Values[1], Values[2]));
}

// vn x y z
void FObjDecoder::AddNormalList(std::string_view Line)
{
	float Values[3] = { 0.0f, 0.0f, 0.0f };
	const int32 Count = ReadFloats(Line, Values, 3);

	if (Count < 3)
	{
		ObjInfo.NormalList.push_back(FVector(0.0f, 0.0f, 0.0f));
		UE_LOG_WARN("FObjDecoder : AddNormalList Error (need 3 floats)");
		return;
	}

	ObjInfo.NormalList.push_back(FVector(Values[0], Values[1], Values[2]));
}


bool FObjDecoder::IsConvexDot(FVector PrevVertexPosition, FVector EarVertexPosition, FVector NextVertexPosition, FVector Normal)
{
	FVector A = EarVertexPosition - PrevVertexPosition;
	FVector B = NextVertexPosition - EarVertexPosition;

	float result = (A.Cross(B)).Dot(Normal);

	return (result > 0);
}

bool FObjDecoder::IsPointInTriangle(FVector A, FVector B, FVector C, FVector Q, FVector Normal)
{
	float d1;
	float d2;
	float d3;

	d1 = ((B - A).Cross(Q - A)).Dot(Normal);
	d2 = ((C - B).Cross(Q - B)).Dot(Normal);
	d3 = ((A - C).Cross(Q - C)).Dot(Normal);

	return (d1 >= 0 && d2 >= 0 && d3 >= 0);

}

void FObjDecoder::StartEarClipping(const TArray<FCorner>& Corners)
{
	TArray<int32>	Prev;
	TArray<int32>	Next;
	FVector			Normal;
	const TArray<FVector4>& Vertex = ObjInfo.VertexList;

	int32 N = static_cast<int32>(Corners.size());

	for (int i = 0; i < N; i++)
	{
		Prev.push_back(i - 1);
		Next.push_back(i + 1);

		const FVector4& P = Vertex[Corners[i].V];
		const FVector4& Q = Vertex[Corners[(i + 1) % N].V];

		Normal.X += (P.Y - Q.Y) * (P.Z + Q.Z);
		Normal.Y += (P.Z - Q.Z) * (P.X + Q.X);
		Normal.Z += (P.X - Q.X) * (P.Y + Q.Y);
	}

	Prev[0] = static_cast<int32>(N - 1);
	Next[static_cast<int32>(N - 1)] = 0;

	int32 Remaining = static_cast<int32>(N);// 남은 정점 수
	int32 FailCount = 0; // 연속으로 귀를 못 찾은 횟수
	int32 Current = 0; // 지금 귀인지 검사 중인 정점

	while (Remaining >= 3)
	{
		const int32 PrevIndex = Prev[Current];
		const int32 NextIndex = Next[Current];

		bool bIsEar = true;

		if (Remaining > 3 && FailCount >= Remaining)
		{
			UE_LOG_WARN("FObjDecoder : EarClipping stuck, %d corners left", Remaining);
			break;
		}

		if (Remaining > 3 && FailCount < Remaining)
		{
			FVector PrevVertexPosition = Vertex[Corners[PrevIndex].V];
			FVector EarVertexPosition = Vertex[Corners[Current].V];
			FVector NextVertexPosition = Vertex[Corners[NextIndex].V];
			
			if (IsConvexDot(PrevVertexPosition, EarVertexPosition, NextVertexPosition, Normal))
			{
				for (int32 j = Next[NextIndex]; j != PrevIndex; j = Next[j])
				{
					if (IsPointInTriangle(PrevVertexPosition, EarVertexPosition, NextVertexPosition, Vertex[Corners[j].V] , Normal))
					{
						bIsEar = false;
						break;
					}
				}
			}
			else
			{
				bIsEar = false;
			}	
		}
		if (bIsEar)
		{
			const FCorner& A = Corners[PrevIndex];
			const FCorner& B = Corners[Current];
			const FCorner& C = Corners[NextIndex];

			ObjInfo.VertexIndexList.push_back(FTriangleIndices(A.V, B.V, C.V));
			ObjInfo.UVIndexList.push_back(FTriangleIndices(A.VT, B.VT, C.VT));
			ObjInfo.NormalIndexList.push_back(FTriangleIndices(A.VN, B.VN, C.VN));
			ObjInfo.MaterialList.push_back(CurrentMaterial);
			ObjInfo.GroupList.push_back(CurrentGroup);
			ObjInfo.ObjectNamesList.push_back(CurrentObjectName);
			ObjInfo.SmoothingGroupsList.push_back(CurrentSmoothingGroup);

			Next[PrevIndex] = NextIndex;
			Prev[NextIndex] = PrevIndex;
			--Remaining;

			Current = PrevIndex;
			FailCount = 0;
		}
		else
		{
			Current = NextIndex;
			FailCount++;
		}
	}
}

// f a b c [d ...]   (각 항목은 v | v/vt | v//vn | v/vt/vn)
// 4각형 이상은 트라이앵글로 나눠 3개씩 저장한다.
void FObjDecoder::ParseFace(std::string_view Line)
{	
	TArray<FCorner> Corners;

	while (true)
	{
		const std::string_view Token = NextWord(Line);
		if (Token.empty())
		{
			break;
		}

		int32 V, VT, VN;
		if (!ParseFaceToken(Token, V, VT, VN))
		{
			UE_LOG_WARN("FObjDecoder : ParseFace Error (bad token)");
			return;
		}

		Corners.push_back({
			ToZeroBased(V,  ObjInfo.VertexList.size()),
			ToZeroBased(VT, ObjInfo.UVList.size()),
			ToZeroBased(VN, ObjInfo.NormalList.size()),
		});
	}

	if (Corners.size() < 3)
	{
		UE_LOG_WARN("FObjDecoder : ParseFace Error (need at least 3 corners)");
		return;
	}

	

	// 트라이앵글: (0,1,2), (0,2,3), (0,3,4) ...
	if (Corners.size() == 3)
	{
		const FCorner& A = Corners[0];
		const FCorner& B = Corners[1];
		const FCorner& C = Corners[2];

		ObjInfo.VertexIndexList.push_back(FTriangleIndices(A.V, B.V, C.V));
		ObjInfo.UVIndexList.push_back(FTriangleIndices(A.VT, B.VT, C.VT));
		ObjInfo.NormalIndexList.push_back(FTriangleIndices(A.VN, B.VN, C.VN));
		ObjInfo.MaterialList.push_back(CurrentMaterial);
		ObjInfo.GroupList.push_back(CurrentGroup);
		ObjInfo.ObjectNamesList.push_back(CurrentObjectName);
		ObjInfo.SmoothingGroupsList.push_back(CurrentSmoothingGroup);
	}
	else
	{
		StartEarClipping(Corners);
	}
}

int32 FObjDecoder::FindOrAddGroup(std::string_view Name)
{
	for (size_t i = 0; i < ObjInfo.Groups.size(); ++i)
	{
		if (ObjInfo.Groups[i].Name == Name)
		{
			return static_cast<int32>(i);
		}
	}

	FObjGroupInfo Group{};
	Group.Name = FString(Name);
	ObjInfo.Groups.push_back(Group);
	return static_cast<int32>(ObjInfo.Groups.size() - 1);
}

void FObjDecoder::UseGroup(std::string_view Line)
{
	const std::string_view Name = NextWord(Line);
	if (Name.empty())
	{
		CurrentGroup = -1;
		return;
	}

	CurrentGroup = FindOrAddGroup(Name);
}

int32 FObjDecoder::FindOrAddObjectName(std::string_view Name)
{
	for (size_t i = 0; i < ObjInfo.ObjectNames.size(); ++i)
	{
		if (ObjInfo.ObjectNames[i].Name == Name)
		{
			return static_cast<int32>(i);
		}
	}

	FObjObjectInfo ObjectName{};
	ObjectName.Name = FString(Name);
	ObjInfo.ObjectNames.push_back(ObjectName);
	return static_cast<int32>(ObjInfo.ObjectNames.size() - 1);
}

void FObjDecoder::UseObjectName(std::string_view Line)
{
	const std::string_view Name = Trim(Line);
	if (Name.empty())
	{
		CurrentObjectName = -1;
		return;
	}

	CurrentObjectName = FindOrAddObjectName(Name);
}

void FObjDecoder::SetSmoothingGroup(std::string_view Line)
{
	const std::string_view Name = Trim(Line);
	if (Name.empty() || !StringToInt(Name, CurrentSmoothingGroup))
	{
		CurrentSmoothingGroup = 0;
		return;
	}

}

// mtllib a.mtl [b.mtl ...]
// 공백으로 나열된 여러 파일일 수도 있고, 공백이 든 파일명 하나일 수도 있다.
// 쪼개서 열리는 것만 읽고, 하나도 못 열면 줄 전체를 파일명 하나로 다시 시도한다.
void FObjDecoder::AddMaterialLib(std::string_view Line)
{
	const std::string_view FullLine = Trim(Line);
	if (FullLine.empty())
	{
		return;
	}

	int32 LoadedCount = 0;

	// 공백으로 쪼개서 각각 시도
	std::string_view Remaining = FullLine;
	while (true)
	{
		const std::string_view Name = NextWord(Remaining);
		if (Name.empty())
		{
			break;
		}

		const FString LibPath = (std::filesystem::path(ObjDirectory) / Name).generic_string();

		std::filesystem::path ResolvedPath;
		if (!ResolveExistingFile(LibPath, ResolvedPath))
		{
			continue;
		}

		ObjInfo.MaterialLibs.push_back(LibPath);
		ParseMtlFile(ReadFileToString(LibPath));
		++LoadedCount;
	}

	if (LoadedCount > 0)
	{
		return;
	}

	// 하나도 못 열은 경우 전체 경로로 열어보기
	const FString WholePath = (std::filesystem::path(ObjDirectory) / FullLine).generic_string();

	std::filesystem::path ResolvedPath;
	if (ResolveExistingFile(WholePath, ResolvedPath))
	{
		ObjInfo.MaterialLibs.push_back(WholePath);
		ParseMtlFile(ReadFileToString(WholePath));
		return;
	}

	UE_LOG_WARN("FObjDecoder : mtllib not found - %s", FString(FullLine).c_str());
}

// usemtl name
void FObjDecoder::UseMaterial(std::string_view Line)
{
	const std::string_view Name = Trim(Line);
	if (Name.empty())
	{
		CurrentMaterial = -1;
		return;
	}

	CurrentMaterial = FindOrAddMaterial(Name);
}

int32 FObjDecoder::FindOrAddMaterial(std::string_view Name)
{
	for (size_t i = 0; i < ObjInfo.Materials.size(); ++i)
	{
		if (ObjInfo.Materials[i].Name == Name)
		{
			return static_cast<int32>(i);
		}
	}

	FObjMaterialInfo Material{};
	Material.Name = FString(Name);
	ObjInfo.Materials.push_back(Material);
	return static_cast<int32>(ObjInfo.Materials.size() - 1);
}

void FObjDecoder::ParseMtlFile(const FString& File)
{
	DefiningMaterial = -1;

	std::string_view Remaining = File;
	while (!Remaining.empty())
	{
		ParseMtlLine(NextLine(Remaining));
	}

	DefiningMaterial = -1;
}

void FObjDecoder::ParseMtlLine(std::string_view Line)
{
	if (const size_t Annotation = Line.find('#'); Annotation != std::string_view::npos)
	{
		Line = Line.substr(0, Annotation);
	}

	Line = Trim(Line);
	if (Line.empty())
	{
		return;
	}

	const std::string_view Keyword = NextWord(Line);
	if (Keyword.empty())
	{
		return;
	}

	if (Keyword == "newmtl")
	{
		DefiningMaterial = FindOrAddMaterial(Trim(Line));
		return;
	}

	// newmtl 이 나오기 전의 속성 줄은 붙일 곳이 없다.
	if (DefiningMaterial < 0)
	{
		return;
	}
	FObjMaterialInfo& Material = ObjInfo.Materials[DefiningMaterial];

	float Values[3] = { 0.0f, 0.0f, 0.0f };

	if (Keyword == "Ka") // 주변 색상
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Ambient = FVector(Values[0], Values[1], Values[2]);
	}
	else if (Keyword == "Kd") // 확산 색상
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Diffuse = FVector(Values[0], Values[1], Values[2]);
	}
	else if (Keyword == "Ks") // 반사색
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Specular = FVector(Values[0], Values[1], Values[2]);
	}
	else if (Keyword == "Ns") // 반사율
	{
		if (ReadFloats(Line, Values, 1) == 1)
			Material.SpecularExponent = Values[0];
	}
	else if (Keyword == "d") // 투명성
	{
		if (ReadFloats(Line, Values, 1) == 1)
			Material.Opacity = Values[0];
	}
	else if (Keyword == "Tr") // 투명성
	{
		// Tr 은 d 와 반대
		if (ReadFloats(Line, Values, 1) == 1)
			Material.Opacity = 1.0f - Values[0];
	}
	else if (Keyword == "Ke") // 발광
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Emissive = FVector(Values[0], Values[1], Values[2]);
	}
	else if (Keyword == "Tf") // 투과 필터 색상
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.TransmissionFilter = FVector(Values[0], Values[1], Values[2]);
	}
	else if (Keyword == "Ni") // 굴절률
	{
		if (ReadFloats(Line, Values, 1) == 1)
			Material.OpticalDensity = Values[0];
	}
	else if (Keyword == "illum") // 조명 모델
	{
		int32 Model = 0;
		if (StringToInt(Trim(Line), Model))
			Material.IlluminationModel = Model;
	}
	else if (Keyword == "map_Kd") // 디퓨즈 컬러 맵
	{
		Material.DiffuseTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_Ka") // 주변 색상 맵
	{
		Material.AmbientTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_Ks") // 반사 색상 맵
	{
		Material.SpecularTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_d") // 알파 택스쳐 맵
	{
		Material.AlphaTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_bump" || Keyword == "bump") // 범프 맵
	{
		Material.NormalTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_Ke") // 발광 텍스처
	{
		Material.EmissiveTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "map_Ns") // 반사광 하이라이트 구성 요소
	{
		Material.SpecularExponentTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "refl") // 구형 반사 맵
	{
		Material.ReflectionTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "disp") // 변위 맵
	{
		Material.DisplacementTexture = ParseTexturePath(Line);
	}
	else if (Keyword == "decal") // 스텐실 데칼 텍스처
	{
		Material.DecalTexture = ParseTexturePath(Line);
	}
}

void FObjDecoder::ParseLine(std::string_view Line)
{
	// 줄 끝 주석 제거: "v 0 0 0  # 1 a"
	const size_t Annotation = Line.find('#');
	if (Annotation != std::string_view::npos)
	{
		Line = Line.substr(0, Annotation);
	}

	Line = Trim(Line);
	if (Line.empty())
	{
		return;
	}

	const std::string_view Keyword = NextWord(Line);
	if (Keyword.empty())
	{
		return;
	}

	if (Keyword == "v")
		AddVertexList(Line);
	else if (Keyword == "vt")
		AddUVList(Line);
	else if (Keyword == "vn")
		AddNormalList(Line);
	else if (Keyword == "f")
		ParseFace(Line);
	else if (Keyword == "g")
		UseGroup(Line);
	else if (Keyword == "o")
		UseObjectName(Line);
	else if (Keyword == "s")
		SetSmoothingGroup(Line);
	else if (Keyword == "mtllib")
		AddMaterialLib(Line);
	else if (Keyword == "usemtl")
		UseMaterial(Line);
	// 아직 처리하지 않는 키워드: vp(파라미터 공간 정점), l(선), p(점)
}

FObjInfo FObjDecoder::ParseObjFile(const FString& File)
{
	ObjInfo = FObjInfo{};
	CurrentMaterial = -1;
	DefiningMaterial = -1;
	CurrentGroup = -1;
	CurrentObjectName = -1;


	std::string_view Remaining = File;
	while (!Remaining.empty())
	{
		ParseLine(NextLine(Remaining));
	}

	return ObjInfo;
}

FObjInfo FObjDecoder::StartObjFileParser(const FString& PathFileName)
{
	try
	{
		// mtllib 은 obj 파일이 있는 폴더 기준이므로 그 폴더를 기억해 둔다.
		ObjDirectory = std::filesystem::path(PathFileName).parent_path().generic_string();

		const FString File = ReadFileToString(PathFileName);
		return (ParseObjFile(File));
	}
	catch (const std::exception& e)
	{
		UE_LOG_WARN("FObjDecoder : %s", e.what());
	}
	return FObjInfo{};
}

namespace
{
	// 한 코너를 식별하는 (v, vt, vn) 조합. 같은 조합은 같은 정점을 가리킨다.
	struct FCornerKey
	{
		int32 V, VT, NormalKind, NormalId;
		bool operator==(const FCornerKey&) const = default;
	};

	struct FCornerKeyHash
	{
		size_t operator()(const FCornerKey& Key) const noexcept
		{
			size_t Hash = std::hash<int32>{}(Key.V);
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.VT));
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.NormalKind));
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.NormalId));
			return Hash;
		}
	};

	using FVertexMap = std::unordered_map<FCornerKey, uint32, FCornerKeyHash>;

	struct FSmoothingKey
	{
		int32  VertexIndexNumber;
		int32 SmoothingGroupNumber;
		bool operator==(const FSmoothingKey&) const = default;
	};

	struct FSmoothingKeyHash
	{
		size_t operator()(const FSmoothingKey& Key) const noexcept
		{
			size_t Hash = std::hash<int32>{}(Key.SmoothingGroupNumber);
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.VertexIndexNumber));
			return Hash;
		}
	};

	using FSmoothingMap = std::unordered_map<FSmoothingKey, FVector, FSmoothingKeyHash>;

	//-1(없음)은 허용.
	bool IsIndexValid(int32 Index, size_t ListSize)
	{
		return Index == -1 || (Index >= 0 && static_cast<size_t>(Index) < ListSize);
	}

	// 정점 하나를 (v, vt, vn) 로 조립한다.
	FVertexData MakeVertex(const FObjInfo& Info, const FCornerKey& Key, TArray<FVector>& NormalVectorList, FSmoothingMap& SmoothingMap)
	{
		FVertexData Vertex{};

		const FVector4& Pos = Info.VertexList[Key.V];
		Vertex.x = Pos.X;
		Vertex.y = Pos.Y;
		Vertex.z = Pos.Z;

		// v 줄에 색상이 있었다면(x y z r g b) 정점 색으로 쓴다. 없으면 흰색.
		if (static_cast<size_t>(Key.V) < Info.ColorList.size())
		{
			const FVector& Color = Info.ColorList[Key.V];
			Vertex.r = Color.X;
			Vertex.g = Color.Y;
			Vertex.b = Color.Z;
		}

		if (Key.VT >= 0)
		{
			// OBJ 의 UV 원점은 좌하단, D3D 는 좌상단이라 V 를 뒤집는다.
			const FVector& UV = Info.UVList[Key.VT];
			Vertex.u = UV.X;
			Vertex.v = 1.0f - UV.Y;
		}

		
		if (Key.NormalKind == EXPLICIT)
		{
			const FVector& Normal = Info.NormalList[Key.NormalId];
			Vertex.nx = Normal.X;
			Vertex.ny = Normal.Y;
			Vertex.nz = Normal.Z;
		}
		else if (Key.NormalKind == SMOOTH)
		{
			FSmoothingKey SmoothingKey;

			SmoothingKey.SmoothingGroupNumber = Key.NormalId;
			SmoothingKey.VertexIndexNumber = Key.V;

			if (auto It = SmoothingMap.find(SmoothingKey); It != SmoothingMap.end())
			{
				FVector NormalVector = It->second;

				if (NormalVector.SizeSquared() > 1e-12f) // 선이거나 면이 마주봐서 0이된 경우 나누는거 방지
				{
					NormalVector /= NormalVector.Size();
					Vertex.nx = NormalVector.X;
					Vertex.ny = NormalVector.Y;
					Vertex.nz = NormalVector.Z;
				}
			}
		}
		else if (Key.NormalKind == FLAT)
		{
			FVector NormalVector = NormalVectorList[Key.NormalId];

			if (NormalVector.SizeSquared() > 1e-12f)
			{
				NormalVector /= NormalVector.Size();
				Vertex.nx = NormalVector.X;
				Vertex.ny = NormalVector.Y;
				Vertex.nz = NormalVector.Z;
			}
		}

		return Vertex;
	}

	uint32 GetOrAddVertex(
		const FObjInfo& Info,
		const FCornerKey& Key,
		FVertexMap& Vertices,
		FObjModelData& Out,
		TArray<FVector>& NormalVectorList,
		FSmoothingMap& SmoothingMap)
	{
		if (auto It = Vertices.find(Key); It != Vertices.end())
		{
			return It->second;
		}

		const uint32 NewIndex = static_cast<uint32>(Out.Vertices.size());
		Out.Vertices.push_back(MakeVertex(Info, Key, NormalVectorList, SmoothingMap));
		Vertices.emplace(Key, NewIndex);
		return NewIndex;
	}

	FCornerKey MakeCornerKey(int32 V, int32 VT, int32 VN, int32 S, int32 Triangle)
	{
		FCornerKey CornerKey;


		CornerKey.V = V;
		CornerKey.VT = VT;
		if (VN >= 0)
		{
			CornerKey.NormalKind = EXPLICIT;
			CornerKey.NormalId = VN;
		}
		else if (S > 0)
		{
			CornerKey.NormalKind = SMOOTH;
			CornerKey.NormalId = S;
		}
		else
		{
			CornerKey.NormalKind = FLAT;
			CornerKey.NormalId = Triangle;
		}
		return (CornerKey);
	}
}


void CalculateNormalVector(const FObjInfo& Info, TArray<FVector>& NormalVectorList, FSmoothingMap& SmoothingMap)
{
	const size_t TriangleCount = Info.VertexIndexList.size();
	for (size_t Index = 0; Index < TriangleCount; ++Index)
	{
		const FTriangleIndices& V = Info.VertexIndexList[Index];


		if (!IsIndexValid(V.Index[0], Info.VertexList.size())
			|| !IsIndexValid(V.Index[1], Info.VertexList.size())
			|| !IsIndexValid(V.Index[2], Info.VertexList.size()))
		{			
			NormalVectorList.push_back(FVector(0, 0, 0));
			continue;
		}

		const FVector VertexA = Info.VertexList[V.Index[0]];
		const FVector VertexB = Info.VertexList[V.Index[1]];
		const FVector VertexC = Info.VertexList[V.Index[2]];
		const int32 SmoothingGroupNumber = Info.SmoothingGroupsList[Index];
		
		NormalVectorList.push_back((VertexB - VertexA).Cross(VertexC - VertexA));

		if (SmoothingGroupNumber > 0)
		{
			FSmoothingKey SmoothingKey;
			for (int i = 0; i < 3; i++)
			{
				SmoothingKey.SmoothingGroupNumber = SmoothingGroupNumber;
				SmoothingKey.VertexIndexNumber = static_cast<int>(V.Index[i]);
				SmoothingMap[SmoothingKey] += NormalVectorList.back();
			}
		}
	}
}


bool FObjDecoder::CookStaticMesh(const FObjInfo& Info, FObjModelData& Out)
{
	Out.Vertices.clear();
	Out.Indices.clear();
	Out.Sections.clear();
	Out.Materials = Info.Materials;
	Out.Groups = Info.Groups;
	Out.ObjectNames = Info.ObjectNames;

	const int32 TriangleCount = static_cast<int32>(Info.VertexIndexList.size());
	if (TriangleCount == 0)
	{
		UE_LOG_WARN("FObjDecoder : CookStaticMesh - no faces");
		return false;
	}

	if (Info.UVIndexList.size() != TriangleCount || Info.NormalIndexList.size() != TriangleCount)
	{
		UE_LOG_WARN("FObjDecoder : CookStaticMesh - index list size mismatch)");
		return false;
	}

	TArray<FVector> NormalVectorList;

	FSmoothingMap SmoothingMap;

	CalculateNormalVector(Info, NormalVectorList, SmoothingMap);



	TSortedMap<FSectionKey, TArray<int32>> Bucket;

	for (int32 Index = 0; Index < TriangleCount; ++Index)
	{
		FSectionKey Key;
		Key.Object = Info.ObjectNamesList[Index];
		Key.Group = Info.GroupList[Index];
		Key.Material = Info.MaterialList[Index];
		Bucket[Key].push_back(Index);
	}

	FVertexMap Vertices;
	Vertices.reserve(TriangleCount * 3);
	Out.Indices.reserve(TriangleCount * 3);


	uint32 CurrnetIndex = 0;
	for (const auto& [SectionKey, Triangles] : Bucket)
	{
		FMeshSection CurrentSection;
		CurrentSection.FirstIndex = CurrnetIndex;
		CurrentSection.Object = SectionKey.Object;
		CurrentSection.Group = SectionKey.Group;
		CurrentSection.MaterialIndex = SectionKey.Material;
		uint32 EndIndex = CurrnetIndex;

		for (int32 Triangle : Triangles)
		{
			const FTriangleIndices& V = Info.VertexIndexList[Triangle];
			const FTriangleIndices& VT = Info.UVIndexList[Triangle];
			const FTriangleIndices& VN = Info.NormalIndexList[Triangle];
			const int32& S = Info.SmoothingGroupsList[Triangle];


			// 범위 밖 인덱스가 하나라도 있으면 이 삼각형은 버린다.
			bool bValid = true;
			for (uint32 i = 0; i < 3; i++)
			{
				if (V.Index[i] < 0 || !IsIndexValid(V.Index[i], Info.VertexList.size())
					|| !IsIndexValid(VT.Index[i], Info.UVList.size())
					|| !IsIndexValid(VN.Index[i], Info.NormalList.size()))
				{
					bValid = false;
					break;
				}
			}
			if (!bValid)
			{
				continue;
			}

			const FCornerKey Corners[3] = {
				MakeCornerKey(static_cast<int32>(V.Index[0]), static_cast<int32>(VT.Index[0]), static_cast<int32>(VN.Index[0]), S, Triangle),
				MakeCornerKey(static_cast<int32>(V.Index[1]), static_cast<int32>(VT.Index[1]), static_cast<int32>(VN.Index[1]), S, Triangle),
				MakeCornerKey(static_cast<int32>(V.Index[2]), static_cast<int32>(VT.Index[2]), static_cast<int32>(VN.Index[2]), S, Triangle)
			};

			for (const FCornerKey& Corner : Corners)
			{
				Out.Indices.push_back(GetOrAddVertex(Info, Corner, Vertices, Out, NormalVectorList, SmoothingMap));

				for (int i = 0; i < 3; i++)
				{
					if (Info.VertexList[Corner.V][i] < CurrentSection.LocalBounds.Min[i])
						CurrentSection.LocalBounds.Min[i] = Info.VertexList[Corner.V][i];
					if (Info.VertexList[Corner.V][i] > CurrentSection.LocalBounds.Max[i])
						CurrentSection.LocalBounds.Max[i] = Info.VertexList[Corner.V][i];
				}
			}

			EndIndex += 3;
		}
		CurrentSection.IndexCount = EndIndex - CurrnetIndex;
		CurrnetIndex = EndIndex;
		Out.Sections.push_back(CurrentSection);
	}

	return (!Out.Indices.empty());
}

bool FObjDecoder::DecodeFromFile(const FString& AbsolutePath, FObjModelData& Out)
{
	//if (auto It = ObjStaticMeshMap.find(AbsolutePath); It != ObjStaticMeshMap.end())
	//{
	//	return It->second;
	//}

	FObjDecoder Decoder;
	const FObjInfo Info = Decoder.StartObjFileParser(AbsolutePath);
	Out.PathFileName = AbsolutePath;

	if (!CookStaticMesh(Info, Out))
	{
		return false;
	}
	
	Out.TextureName = FName("None");
	if (!Out.Materials.empty() && !Out.Materials.front().DiffuseTexture.empty())
	{
		FString TextureKey = std::filesystem::path(Out.Materials.front().DiffuseTexture).stem().string();
		std::transform(TextureKey.begin(), TextureKey.end(), TextureKey.begin(), ::tolower);
		Out.TextureName = FName(TextureKey);
	}
	// ObjStaticMeshMap.emplace(AbsolutePath, Out);
	return true;
}