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

	FString ReadFileToString(std::string_view FileName)
	{
		
		std::filesystem::path FilePath(FileName);

		if (!FilePath.is_absolute())
		{
			const std::filesystem::path AssetDir = GetAssetDir();
			FilePath = AssetDir / FileName;

			if (!IsUnder(FilePath, AssetDir))
			{
				UE_LOG_WARN("FObjDecoder : 에셋 폴더 외부 경로 접근 - %s", FilePath.string().c_str());
				return FString();
			}
		}

		if (!std::filesystem::exists(FilePath))
		{
			UE_LOG_WARN("FObjDecoder : 파일이 존재하지 않음 - %s", FilePath.string().c_str());
			return FString();
		}

		std::ifstream FileStream(FilePath, std::ios::in);
		if (!FileStream.is_open())
		{
			UE_LOG_WARN("FObjDecoder : 파일 열기 실패 - %s", FilePath.string().c_str());
			return FString();
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
		UE_LOG("FObjDecoder : AddVertexList Error (least 3 floats)");
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
		UE_LOG("FObjDecoder : AddUVList Error (need at least 1 float)");
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
		UE_LOG("FObjDecoder : AddNormalList Error (need 3 floats)");
		return;
	}

	ObjInfo.NormalList.push_back(FVector(Values[0], Values[1], Values[2]));
}

// f a b c [d ...]   (각 항목은 v | v/vt | v//vn | v/vt/vn)
// 4각형 이상은 트라이앵글로 나눠 3개씩 저장한다.
void FObjDecoder::ParseFace(std::string_view Line)
{
	struct FCorner { int32 V, VT, VN; };
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
			UE_LOG("FObjDecoder : ParseFace Error (bad token)");
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
		UE_LOG("FObjDecoder : ParseFace Error (need at least 3 corners)");
		return;
	}

	// 트라이앵글: (0,1,2), (0,2,3), (0,3,4) ...
	for (size_t i = 1; i + 1 < Corners.size(); ++i)
	{
		const FCorner& A = Corners[0];
		const FCorner& B = Corners[i];
		const FCorner& C = Corners[i + 1];

		ObjInfo.VertexIndexList.push_back(FVector4(
			static_cast<float>(A.V), static_cast<float>(B.V), static_cast<float>(C.V)));
		ObjInfo.UVIndexList.push_back(FVector(
			static_cast<float>(A.VT), static_cast<float>(B.VT), static_cast<float>(C.VT)));
		ObjInfo.NormalIndexList.push_back(FVector(
			static_cast<float>(A.VN), static_cast<float>(B.VN), static_cast<float>(C.VN)));
		ObjInfo.MaterialList.push_back(CurrentMaterial);
		ObjInfo.GroupList.push_back(CurrentGroup);
		ObjInfo.ObjectNamesList.push_back(CurrentObjectName);
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
	const std::string_view Name = Trim(Line);
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

void FObjDecoder::AddMaterialLib(std::string_view Line)
{
	while (true)
	{
		const std::string_view Name = NextWord(Line);
		if (Name.empty())
		{
			break;
		}

		const FString LibPath = (std::filesystem::path(ObjDirectory) / Name).generic_string();
		ObjInfo.MaterialLibs.push_back(LibPath);

		try
		{
			const FString File = ReadFileToString(LibPath);
			ParseMtlFile(File);
		}
		catch (const std::exception& e)
		{
			UE_LOG_WARN("FObjDecoder : mtllib - %s", e.what());
		}
	}
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

	std::string LowerKeyword(Keyword);
	std::transform(LowerKeyword.begin(), LowerKeyword.end(), LowerKeyword.begin(), ::tolower);

	if (LowerKeyword == "newmtl")
	{
		DefiningMaterial = FindOrAddMaterial(Trim(Line));
		return;
	}

	// 재질 정의 전 속성 라인은 건너뜀
	if (DefiningMaterial < 0)
	{
		return;
	}
	FObjMaterialInfo& Material = ObjInfo.Materials[DefiningMaterial];

	float Values[3] = { 0.0f, 0.0f, 0.0f };

	if (LowerKeyword == "ka")
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Ambient = FVector(Values[0], Values[1], Values[2]);
	}
	else if (LowerKeyword == "kd")
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Diffuse = FVector(Values[0], Values[1], Values[2]);
	}
	else if (LowerKeyword == "ks")
	{
		if (ReadFloats(Line, Values, 3) == 3)
			Material.Specular = FVector(Values[0], Values[1], Values[2]);
	}
	else if (LowerKeyword == "ns")
	{
		if (ReadFloats(Line, Values, 1) == 1)
			Material.SpecularExponent = Values[0];
	}
	else if (LowerKeyword == "d")
	{
		if (ReadFloats(Line, Values, 1) == 1)
			Material.Opacity = Values[0];
	}
	else if (LowerKeyword == "tr")
	{
		// 투명도 반전 계산
		if (ReadFloats(Line, Values, 1) == 1)
			Material.Opacity = 1.0f - Values[0];
	}
	else if (LowerKeyword == "illum")
	{
		int32 Model = 0;
		if (StringToInt(Trim(Line), Model))
			Material.IlluminationModel = Model;
	}
	else if (LowerKeyword == "map_kd")
	{
		Material.DiffuseTexture = ParseTexturePath(Line);
	}
	else if (LowerKeyword == "map_ka")
	{
		Material.AmbientTexture = ParseTexturePath(Line);
	}
	else if (LowerKeyword == "map_ks")
	{
		Material.SpecularTexture = ParseTexturePath(Line);
	}
	else if (LowerKeyword == "map_d")
	{
		Material.AlphaTexture = ParseTexturePath(Line);
	}
	else if (LowerKeyword == "map_bump" || LowerKeyword == "bump" || LowerKeyword == "norm")
	{
		Material.NormalTexture = ParseTexturePath(Line);
	}
	// 기타 속성 처리 생략
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
	else if (Keyword == "mtllib")
		AddMaterialLib(Line);
	else if (Keyword == "usemtl")
		UseMaterial(Line);
	// 아직 처리하지 않는 키워드: vp, s, l, p
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
		return ParseObjFile(File);
	}
	catch (const std::exception& e)
	{
		UE_LOG("FObjDecoder : %s", e.what());
	}
	return FObjInfo{};
}

namespace
{
	// 한 코너를 식별하는 (v, vt, vn) 조합. 같은 조합은 같은 정점을 가리킨다.
	struct FCornerKey
	{
		int32 V, VT, VN;
		bool operator==(const FCornerKey&) const = default;
	};

	struct FCornerKeyHash
	{
		size_t operator()(const FCornerKey& Key) const noexcept
		{
			size_t Hash = std::hash<int32>{}(Key.V);
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.VT));
			Hash = EngineUtil::HashCombine(Hash, std::hash<int32>{}(Key.VN));
			return Hash;
		}
	};

	using FUniqueVertexMap = std::unordered_map<FCornerKey, uint32, FCornerKeyHash>;

	//-1(없음)은 허용.
	bool IsIndexValid(int32 Index, size_t ListSize)
	{
		return Index == -1 || (Index >= 0 && static_cast<size_t>(Index) < ListSize);
	}

	// 정점 하나를 (v, vt, vn) 로 조립한다.
	FVertexData MakeVertex(const FObjInfo& Info, const FCornerKey& Key)
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

		if (Key.VN >= 0)
		{
			const FVector& Normal = Info.NormalList[Key.VN];
			Vertex.nx = Normal.X;
			Vertex.ny = Normal.Y;
			Vertex.nz = Normal.Z;
		}

		return Vertex;
	}

	uint32 GetOrAddVertex(
		const FObjInfo& Info,
		const FCornerKey& Key,
		FUniqueVertexMap& UniqueVertices,
		FObjModelData& Out)
	{
		if (auto It = UniqueVertices.find(Key); It != UniqueVertices.end())
		{
			return It->second;
		}

		const uint32 NewIndex = static_cast<uint32>(Out.Vertices.size());
		Out.Vertices.push_back(MakeVertex(Info, Key));
		UniqueVertices.emplace(Key, NewIndex);
		return NewIndex;
	}
}

bool FObjDecoder::CookStaticMesh(const FObjInfo& Info, FObjModelData& Out)
{
	Out.Vertices.clear();
	Out.Indices.clear();
	//Out.TriangleMaterials.clear();
	Out.Materials = Info.Materials;
	Out.Groups = Info.Groups;
	Out.ObjectNames = Info.ObjectNames;

	const size_t TriangleCount = Info.VertexIndexList.size();
	if (TriangleCount == 0)
	{
		UE_LOG("FObjDecoder : CookStaticMesh - no faces");
		return false;
	}

	if (Info.UVIndexList.size() != TriangleCount || Info.NormalIndexList.size() != TriangleCount)
	{
		UE_LOG("FObjDecoder : CookStaticMesh - index list size mismatch)");
		return false;
	}

	TSortedMap<FSectionKey, TArray<size_t>> Bucket;

	for (size_t Index = 0; Index < TriangleCount; ++Index)
	{
		FSectionKey Key;
		Key.Object = Info.ObjectNamesList[Index];
		Key.Group = Info.GroupList[Index];
		Key.Material = Info.MaterialList[Index];
		Bucket[Key].push_back(Index);
	}

	FUniqueVertexMap UniqueVertices;
	UniqueVertices.reserve(TriangleCount * 3);
	Out.Indices.reserve(TriangleCount * 3);


	uint32 CurrnetIndex = 0;
	for (const auto& [SectionKey, Triangles] : Bucket)
	{
		FMeshSection CurrentSection;
		CurrentSection.FirstIndex = CurrnetIndex;
		CurrentSection.Object = SectionKey.Object;
		CurrentSection.Group = SectionKey.Group;
		int32 MatIdx = SectionKey.Material;
		if (MatIdx < 0 && !Info.Materials.empty())
		{
			MatIdx = 0;
		}
		if (MatIdx >= 0 && MatIdx < static_cast<int32>(Info.Materials.size()))
		{
			const auto& Mat = Info.Materials[MatIdx];
			if (!Mat.DiffuseTexture.empty())
			{
				FString Stem = std::filesystem::path(Mat.DiffuseTexture).stem().string();
				std::transform(Stem.begin(), Stem.end(), Stem.begin(), ::tolower);
				CurrentSection.DiffuseTextureName = FName(Stem);
			}
			if (!Mat.NormalTexture.empty())
			{
				FString Stem = std::filesystem::path(Mat.NormalTexture).stem().string();
				std::transform(Stem.begin(), Stem.end(), Stem.begin(), ::tolower);
				CurrentSection.NormalTextureName = FName(Stem);
			}
			if (!Mat.SpecularTexture.empty())
			{
				FString Stem = std::filesystem::path(Mat.SpecularTexture).stem().string();
				std::transform(Stem.begin(), Stem.end(), Stem.begin(), ::tolower);
				CurrentSection.SpecularTextureName = FName(Stem);
			}
		}
		uint32 EndIndex = CurrnetIndex;

		for (auto Triangle : Triangles)
		{
			const FVector& V = Info.VertexIndexList[Triangle];
			const FVector& VT = Info.UVIndexList[Triangle];
			const FVector& VN = Info.NormalIndexList[Triangle];

			const FCornerKey Corners[3] = {
				{ static_cast<int32>(V.X), static_cast<int32>(VT.X), static_cast<int32>(VN.X) },
				{ static_cast<int32>(V.Y), static_cast<int32>(VT.Y), static_cast<int32>(VN.Y) },
				{ static_cast<int32>(V.Z), static_cast<int32>(VT.Z), static_cast<int32>(VN.Z) },
			};

			// 범위 밖 인덱스가 하나라도 있으면 이 삼각형은 버린다.
			bool bValid = true;
			for (const FCornerKey& Corner : Corners)
			{
				if (Corner.V < 0 || !IsIndexValid(Corner.V, Info.VertexList.size())
					|| !IsIndexValid(Corner.VT, Info.UVList.size())
					|| !IsIndexValid(Corner.VN, Info.NormalList.size()))
				{
					bValid = false;
					break;
				}
			}
			if (!bValid)
			{
				continue;
			}

			for (const FCornerKey& Corner : Corners)
			{
				
				Out.Indices.push_back(GetOrAddVertex(Info, Corner, UniqueVertices, Out));

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

	// 정점별 탄젠트 및 바이탄젠트 누적 계산
	for (auto& V : Out.Vertices)
	{
		V.tx = 0.0f; V.ty = 0.0f; V.tz = 0.0f;
		V.bx = 0.0f; V.by = 0.0f; V.bz = 0.0f;
	}

	for (size_t i = 0; i + 2 < Out.Indices.size(); i += 3)
	{
		const uint32 i0 = Out.Indices[i];
		const uint32 i1 = Out.Indices[i + 1];
		const uint32 i2 = Out.Indices[i + 2];

		FVertexData& v0 = Out.Vertices[i0];
		FVertexData& v1 = Out.Vertices[i1];
		FVertexData& v2 = Out.Vertices[i2];

		const FVector pos0(v0.x, v0.y, v0.z);
		const FVector pos1(v1.x, v1.y, v1.z);
		const FVector pos2(v2.x, v2.y, v2.z);

		const FVector edge1 = pos1 - pos0;
		const FVector edge2 = pos2 - pos0;

		const float du1 = v1.u - v0.u;
		const float dv1 = v1.v - v0.v;
		const float du2 = v2.u - v0.u;
		const float dv2 = v2.v - v0.v;

		const float det = du1 * dv2 - du2 * dv1;
		if (std::abs(det) > 1e-6f)
		{
			const float invDet = 1.0f / det;
			const FVector tangent = (edge1 * dv2 - edge2 * dv1) * invDet;
			const FVector bitangent = (edge2 * du1 - edge1 * du2) * invDet;

			v0.tx += tangent.X; v0.ty += tangent.Y; v0.tz += tangent.Z;
			v1.tx += tangent.X; v1.ty += tangent.Y; v1.tz += tangent.Z;
			v2.tx += tangent.X; v2.ty += tangent.Y; v2.tz += tangent.Z;

			v0.bx += bitangent.X; v0.by += bitangent.Y; v0.bz += bitangent.Z;
			v1.bx += bitangent.X; v1.by += bitangent.Y; v1.bz += bitangent.Z;
			v2.bx += bitangent.X; v2.by += bitangent.Y; v2.bz += bitangent.Z;
		}
	}

	// 정점별 직교화 및 정규화
	for (auto& v : Out.Vertices)
	{
		FVector N(v.nx, v.ny, v.nz);
		FVector T(v.tx, v.ty, v.tz);
		FVector B(v.bx, v.by, v.bz);

		T = T - N * N.Dot(T);
		const float tLen = T.Size();
		T = (tLen > 1e-6f) ? (T / tLen) : FVector(1.0f, 0.0f, 0.0f);

		const float bLen = B.Size();
		B = (bLen > 1e-6f) ? (B / bLen) : N.Cross(T);

		v.tx = T.X; v.ty = T.Y; v.tz = T.Z;
		v.bx = B.X; v.by = B.Y; v.bz = B.Z;
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

	UE_LOG("FObjDecoder : %s  v=%zu vt=%zu vn=%zu tri=%zu",
		AbsolutePath.c_str(),
		Info.VertexList.size(), Info.UVList.size(),
		Info.NormalList.size(), Info.VertexIndexList.size());

	Out.PathFileName = AbsolutePath;

	if (!CookStaticMesh(Info, Out))
	{
		return false;
	}
	
	Out.TextureName = FName("None");
	Out.NormalTextureName = FName("None");
	Out.SpecularTextureName = FName("None");
	if (!Out.Materials.empty())
	{
		if (!Out.Materials.front().DiffuseTexture.empty())
		{
			FString TextureKey = std::filesystem::path(Out.Materials.front().DiffuseTexture).stem().string();
			std::transform(TextureKey.begin(), TextureKey.end(), TextureKey.begin(), ::tolower);
			Out.TextureName = FName(TextureKey);
		}
		if (!Out.Materials.front().NormalTexture.empty())
		{
			FString TextureKey = std::filesystem::path(Out.Materials.front().NormalTexture).stem().string();
			std::transform(TextureKey.begin(), TextureKey.end(), TextureKey.begin(), ::tolower);
			Out.NormalTextureName = FName(TextureKey);
		}
		if (!Out.Materials.front().SpecularTexture.empty())
		{
			FString TextureKey = std::filesystem::path(Out.Materials.front().SpecularTexture).stem().string();
			std::transform(TextureKey.begin(), TextureKey.end(), TextureKey.begin(), ::tolower);
			Out.SpecularTextureName = FName(TextureKey);
		}
	}

	UE_LOG("FObjDecoder : %s  cooked  vertices=%zu indices=%zu materials=%zu",
		AbsolutePath.c_str(), Out.Vertices.size(), Out.Indices.size(), Out.Materials.size());

	for (const FObjMaterialInfo& Material : Out.Materials)
	{
		UE_LOG("FObjDecoder :   material '%s'  Kd=(%.2f %.2f %.2f)  map_Kd='%s'",
			Material.Name.c_str(),
			Material.Diffuse.X, Material.Diffuse.Y, Material.Diffuse.Z,
			Material.DiffuseTexture.c_str());
	}

	return true;
}