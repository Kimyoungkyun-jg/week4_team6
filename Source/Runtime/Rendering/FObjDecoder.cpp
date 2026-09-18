#include "FObjDecoder.h"
#include "Runtime/Core/Log.h"
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>
#include <string_view>
#include <ranges>
#include <charconv>
#include <cassert>



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

bool FObjDecoder::DecodeFromFile(const FString& FilePath, FObjVertexInfo& VetexInfoOut, FObjMaterialInfo& MaterialInfoOut)
{
	std::error_code Ec;
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		UE_LOG("OBJ 파일 열기 실패: %s", FilePath.c_str());
		return false;
	}

	const std::filesystem::path ObjPath(FilePath);
	const std::filesystem::path BaseDir = ObjPath.parent_path();

	std::stringstream ObjBuffer;
	ObjBuffer << File.rdbuf();

	if (!DecodeObjFile(ObjBuffer.str(), VetexInfoOut))
	{
		return false;
	}

	std::filesystem::path Candidate = ObjPath;
	Candidate.replace_extension(".mtl");
	if (std::filesystem::is_regular_file(Candidate, Ec))
	{
		std::ifstream MtlFile(Candidate);
		if (MtlFile.is_open())
		{
			std::stringstream MtlBuffer;
			MtlBuffer << MtlFile.rdbuf();
			DecodeMtlFile(MtlBuffer.str(), MaterialInfoOut);
		}
	}

	return true;
}


bool FObjDecoder::DecodeObjFile(const FString& FileContent, FObjVertexInfo& OutData)
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
				const int32 Counts[3] = { static_cast<uint32>(Positions.size()), static_cast<uint32>(UVs.size()), static_cast<uint32>(Normals.size()) };
				for (auto Parsed : std::string_view(InString) | std::views::split('/'))
				{
					if (!bIsValid || i >= 3)
					{
						bIsValid = false;
						break;
					}
					std::string_view StringNum(Parsed.begin(), Parsed.end());
					int32 ResolvedIndex = ResolveIndex(StringNum, Counts[i]);
					if((i == 0 && ResolvedIndex == INVALID_INDEX) 
						|| ((i == 1 || i == 2) && !StringNum.empty() && ResolvedIndex == INVALID_INDEX))
					{
						bIsValid = false;
						break;
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
					FVertexKey Key = Triangle[j];
					auto [It, bInserted] = VertexCache.try_emplace(Key, OutData.Vertices.size());
					if (bInserted)
					{
						auto VertexData = MakeVertex(Key, Positions, UVs, Normals);
						OutData.Vertices.push_back(VertexData);
					}
					OutData.Indices.push_back(It->second);
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
	int32 InInteger = 0;
	const auto [Ptr, Ec] = std::from_chars(String.data(), String.data() + String.size(), InInteger);
	if (Ec != std::errc{} || InInteger == 0)
	{
		return INVALID_INDEX;
	}
	const long long Resolved = (InInteger > 0) 
		? static_cast<long long>(InInteger) - 1
		: static_cast<long long>(Count) + InInteger;
	if (Resolved < 0 || Resolved >= Count)
	{
		return INVALID_INDEX;
	}
	return static_cast<int32>(Resolved);
}

FVertexData FObjDecoder::MakeVertex(const FVertexKey& Key, const TArray<FVector>& Positions, const TArray<FVector2>& UVs, const TArray<FVector>& Normals)
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
	return VertexData;
}

// 로컬 AABB 바운딩 박스 계산
void FObjDecoder::ComputeStaticBounds(FObjVertexInfo& OutData)
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

bool FObjDecoder::DecodeMtlFile(const FString& FileContent, FObjMaterialInfo& OutData)
{
	return 1;
}