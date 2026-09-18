#include "FObjDecoder.h"
#include "Runtime/Core/Log.h"
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <string_view>
#include <ranges>
#include <charconv>
#include <cassert>


namespace
{
	// .mtl 파일을 읽어 map_Kd의 파일명을 추출하는 함수
	FName ParseMtlTexture(const std::filesystem::path& MtlPath)
	{
		std::ifstream File(MtlPath);
		if (!File.is_open())
		{
			return FName("None");
		}
		std::string Line;
		while (std::getline(File, Line))
		{
			if (Line.empty() || Line[0] == '#')
				continue;
			std::istringstream Stream(Line);
			std::string Prefix;
			Stream >> Prefix;
			// map_Kd: 디퓨즈(기본) 컬러 텍스처 맵
			if (Prefix == "map_Kd")
			{
				std::string TextureFileName;
				Stream >> TextureFileName;
				if (!TextureFileName.empty())
				{
					// 경로가 포함되어 있어도 순수 파일명(stem)만 추출
					// 예: "textures/MasterYi_Head.png" -> "MasterYi_Head"
					std::string StemName = std::filesystem::path(TextureFileName).stem().string();
					return FName(StemName);
				}
			}
		}
		return FName("None");
	}

	constexpr int32 INVALID_INDEX = -1;

	// v, vt, vn 인덱스 묶음 키
	struct FObjIndexKey
	{
		int32 VIndex = INVALID_INDEX;
		int32 VTIndex = INVALID_INDEX;
		int32 VNIndex = INVALID_INDEX;

		bool operator==(const FObjIndexKey& Other) const = default;
	};

	struct FObjIndexKeyHasher
	{
		size_t operator()(const FObjIndexKey& Key) const
		{
			size_t H1 = std::hash<int32>()(Key.VIndex);
			size_t H2 = std::hash<int32>()(Key.VTIndex);
			size_t H3 = std::hash<int32>()(Key.VNIndex);
			return H1 ^ (H2 << 1) ^ (H3 << 2);
		}
	};

	// "v/vt/vn", "v//vn", "v/vt", "v" 형태의 토큰 파싱
	FObjIndexKey ParseFaceToken(const FString& Token)
	{
		FObjIndexKey Key{};
		size_t FirstSlash = Token.find('/');
		if (FirstSlash == FString::npos)
		{
			Key.VIndex = std::stoi(Token);
			return Key;
		}

		Key.VIndex = std::stoi(Token.substr(0, FirstSlash)); //첫번째 슬래시 전까지 위치

		size_t SecondSlash = Token.find('/', FirstSlash + 1);
		if (SecondSlash == FString::npos)
		{
			// v/vt 형식
			FString VTStr = Token.substr(FirstSlash + 1); 
			if (!VTStr.empty())
			{
				Key.VTIndex = std::stoi(VTStr); //2번째 슬래시 없으면 바로 uv로
			}
			return Key;
		}

		// v/vt/vn 또는 v//vn 형식
		FString VTStr = Token.substr(FirstSlash + 1, SecondSlash - FirstSlash - 1);
		if (!VTStr.empty())
		{
			Key.VTIndex = std::stoi(VTStr); //2번째 숫자가 있는 경우
		}

		FString VNStr = Token.substr(SecondSlash + 1);
		if (!VNStr.empty())
		{
			Key.VNIndex = std::stoi(VNStr);
		}

		return Key;
	}

	// 1-based 및 음수 상대 인덱스를 0-based 인덱스로 변환
	int32 ResolveIndex(int32 Index, size_t TotalCount)
	{
		if (Index > 0)
		{
			return Index - 1;
		}
		if (Index < 0)
		{
			return static_cast<int32>(TotalCount) + Index;
		}
		return -1;
	}
}

bool FObjDecoder::DecodeFromFile(const FString& FilePath, FObjModelData& OutData)
{
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		UE_LOG("OBJ 파일 열기 실패: %s", FilePath.c_str());
		return false;
	}

	// OBJ 파일이 위치한 부모 폴더 경로를 구함
	std::string BaseDir = std::filesystem::path(FilePath).parent_path().string();

	std::stringstream Buffer;
	Buffer << File.rdbuf();

	// 부모 폴더 경로(BaseDir)를 함께 넘겨줌
	return DecodeFromString(Buffer.str(), OutData, BaseDir);
}


bool FObjDecoder::DecodeFromString(const FString& FileContent, FObjModelData& OutData, const FString& BaseDirectory)
{
	TArray<FVector> Positions;
	TArray<FVector2> UVs;
	TArray<FVector> Normals;
	TMap<FVertexKey, uint32> VertexCache;

	std::istringstream File(FileContent);
	FString Line;
	while (std::getline(File, Line))
	{
		std::istringstream SS(Line);
		FString Keyword;
		FString Word;
		SS >> Keyword;

		if (Keyword == "o")
		{
			FString Input;
			while (SS >> Word)
			{
				Input.append(Word).append("_");
			}
			FName InName(Input);
			OutData.ObjectName = InName;
		}
		else if (Keyword == "v")
		{
			float x, y, z;
			SS >> x >> y >> z;
			Positions.push_back(FVector(x, y, z));
		}
		else if (Keyword == "vt")
		{
			float u, v;
			SS >> u >> v;
			UVs.push_back(FVector2(u, v));
		}
		else if (Keyword == "vn")
		{
			float x, y, z;
			SS >> x >> y >> z;
			Normals.push_back(FVector(x, y, z));
		}
		else if (Keyword == "f")
		{
			FString InString;
			TArray<FVertexKey> VertexKeyList;
			while (SS >> InString)
			{
				FVertexKey VertexKey;
				uint32 i = 0;
				bool bIsValid = true;
				for (auto Parsed : std::string_view(InString) | std::views::split('/'))
				{
					std::string_view StringNum(Parsed.begin(), Parsed.end());
					int32 ResolvedIndex = ResolveIndex(StringNum, VertexCache.size());
					if (bIsValid
						|| (i == 0 && ResolvedIndex == INVALID_INDEX) 
						|| (i != 1 && !StringNum.empty() && ResolvedIndex == INVALID_INDEX))
					{
						bIsValid = false;
						continue;
					}
					VertexKey[i++] = ResolvedIndex;
				}
				if (bIsValid)
				{
					VertexKeyList.push_back(VertexKey);
				}
			}
			if (VertexKeyList.size() < 3)
			{
				continue;
			}
			for (uint32 i = 0; i < VertexKeyList.size() - 2; i++)
			{
				FVertexKey Triangle[3] = { VertexKeyList[0], VertexKeyList[i + 1], VertexKeyList[i + 2] };

				for (uint32 j = 0; j < 3; j++)
				{
					auto it = VertexCache.find(Triangle[j]);
					int32 Index = OutData.Vertices.size();
					if (it == VertexCache.end())
					{
						FVertexKey Key = Triangle[j];
						VertexCache[Key] = Index;
						auto VertexData = MakeVertex(Key, Positions, UVs, Normals);
						OutData.Vertices.push_back(*VertexData);
						OutData.Indices.push_back(Index);
					}
					else
					{
						OutData.Indices.push_back(it->second);
					}
				}
			}
		}
	}

	if (OutData.Vertices.empty() || OutData.Indices.empty())
	{
		return false;
	}

	// 로컬 AABB 바운딩 박스 계산
	ComputeStaticBounds(OutData);

	return true;
}

int32 FObjDecoder::ResolveIndex(const std::string_view& String, const uint32 Count)
{
	if (String.empty())
	{
		return INVALID_INDEX;
	}
	uint32 InInteger = 0;
	const auto [Ptr, Ec] = std::from_chars(String.data(), String.data() + String.size(), InInteger);
	if (Ec != std::errc{} || InInteger == 0)
	{
		return INVALID_INDEX;
	}
	const long long Resolved = (InInteger > 0) 
		? static_cast<long long>(InInteger) - 1
		: static_cast<long long>(Count + InInteger);
	if (Resolved < 0 || Resolved > Count)
	{
		return INVALID_INDEX;
	}
	return static_cast<int32>(Resolved);
}

TSharedPtr<FVertexData> FObjDecoder::MakeVertex(const FVertexKey& Key, const TArray<FVector> Positions, const TArray<FVector2> UVs, const TArray<FVector> Normals)
{
	FVertexData VertexData{};
	VertexData.x = Positions[Key.PosIndex].X; // TODO: FVertexData를 FVector화 하기 (대공사)
	VertexData.y = Positions[Key.PosIndex].Y;
	VertexData.z = Positions[Key.PosIndex].Z;
	if (Key.UVIndex != INVALID_INDEX)
	{
		VertexData.u = UVs[Key.UVIndex].X;
		VertexData.v = UVs[Key.UVIndex].Y;
	}
	if (Key.NormalIndex != INVALID_INDEX)
	{
		VertexData.nx = Normals[Key.NormalIndex].X;
		VertexData.ny = Normals[Key.NormalIndex].Y;
		VertexData.nz = Normals[Key.NormalIndex].Z;
	}
	return MakeShared<FVertexData>(VertexData);
}

// 로컬 AABB 바운딩 박스 계산
void FObjDecoder::ComputeStaticBounds(FObjModelData& OutData)
{
	FVector MinBound{ (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
	FVector MaxBound{ (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)(), (std::numeric_limits<float>::lowest)() };

	for (const auto& V : OutData.Vertices)
	{
		MinBound.X = (std::min)(MinBound.X, V.x);
		MinBound.Y = (std::min)(MinBound.Y, V.y);
		MinBound.Z = (std::min)(MinBound.Z, V.z);

		MaxBound.X = (std::max)(MaxBound.X, V.x);
		MaxBound.Y = (std::max)(MaxBound.Y, V.y);
		MaxBound.Z = (std::max)(MaxBound.Z, V.z);
	}

	OutData.LocalBounds.Min = MinBound;
	OutData.LocalBounds.Max = MaxBound;
	OutData.bIsValid = true;
}