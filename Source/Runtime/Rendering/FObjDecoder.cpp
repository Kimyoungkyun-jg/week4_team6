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

bool FObjDecoder::DecodeFromFile(const FString& FilePath, FObjVertexInfo& VetexInfoOut, TArray<FObjMaterialInfo>& MaterialInfoOut)
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

	return DecodeObjFile(ObjBuffer.str(), BaseDir.string(), VetexInfoOut, MaterialInfoOut);
}


bool FObjDecoder::DecodeObjFile(const FString& FileContent, const FString& BaseDirectory, FObjVertexInfo& VertexInfoOut, TArray<FObjMaterialInfo>& MaterialInfoOut)
{
	TArray<FVector> Positions;
	TArray<FVector2> UVs;
	TArray<FVector> Normals;
	TMap<FVertexKey, uint32> VertexCache;
	int32 CurrentMaterialIndex = DEFAULT_INDEX;
	FName CurrentGroupName{ "None" };
	TMap<FName, int32> MaterialNameToIndex;
	MaterialNameToIndex["Default"] = DEFAULT_INDEX;
	VertexInfoOut.Vertices.clear();
	VertexInfoOut.Indices.clear();
	MaterialInfoOut.clear();
	MaterialInfoOut.push_back(FObjMaterialInfo{});

	// Start Parse
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
			if (SS >> Word)
			{
				Input.append(Word);
			}
			while (SS >> Word)
			{
				Input.append("_").append(Word);
			}
			FName InName(Input);
			VertexInfoOut.ObjectName = InName;
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
					auto [It, bInserted] = VertexCache.try_emplace(Key, VertexInfoOut.Vertices.size());
					if (bInserted)
					{
						auto VertexData = MakeVertex(Key, Positions, UVs, Normals);
						VertexInfoOut.Vertices.push_back(VertexData);
					}
					CheckSection(VertexInfoOut, CurrentMaterialIndex, CurrentGroupName);
					VertexInfoOut.Indices.push_back(It->second);
					VertexInfoOut.Sections.back().IndexCount++;
				}
			}
		}
		else if (Keyword == "mtllib")
		{
			FString Input;
			while (SS >> Input)
			{
				std::filesystem::path MtlPath = std::filesystem::path(BaseDirectory) / Input;
				if (MtlPath.empty())
				{
					continue;
				}
				uint32 MaterialCountBefore = MaterialInfoOut.size();
				DecodeMtlFile(MtlPath.string(), MaterialInfoOut);
				for (uint32 i = MaterialCountBefore; i < MaterialInfoOut.size(); i++)
				{
					MaterialNameToIndex[MaterialInfoOut[i].MaterialName] = static_cast<uint32>(i);
				}
			}
		}
		else if (Keyword == "usemtl")
		{
			FString Input;
			if (SS >> Word)
			{
				Input.append(Word);
			}
			while (SS >> Word)
			{
				Input.append("_").append(Word);
			}
			FName InName(Input);
			if (auto it = MaterialNameToIndex.find(InName); it != MaterialNameToIndex.end())
			{
				CurrentMaterialIndex = it->second;
			}
			else
			{
				CurrentMaterialIndex = DEFAULT_INDEX;
				UE_LOG("usemtl %s mtl 재질 찾기 실패", Input.c_str());
			}
		}
		else if (Keyword == "g")
		{
			FString Input;
			if (SS >> Word)
			{
				Input.append(Word);
			}
			while (SS >> Word)
			{
				Input.append("_").append(Word);
			}
			FName InName(Input);
			CurrentGroupName = InName;
		}
	}
	// Parse Ended


	if (VertexInfoOut.Vertices.empty() || VertexInfoOut.Indices.empty())
	{
		return false;
	}

	// 로컬 AABB 바운딩 박스 계산
	ComputeStaticBounds(VertexInfoOut);

	UE_LOG("%s.OBJ 파일 열기 성공: 버텍스 %d개, 인덱스%d개", VertexInfoOut.ObjectName.ToString().c_str(), VertexInfoOut.Vertices.size(), VertexInfoOut.Indices.size());

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

bool FObjDecoder::DecodeMtlFile(const FString& FilePath, TArray<FObjMaterialInfo>& OutData)
{
	int32 CurrentIndex = INVALID_INDEX;

	std::error_code Ec;
	std::ifstream File(FilePath);
	if (!File.is_open())
	{
		UE_LOG("MTL 파일 열기 실패: %s", FilePath.c_str());
		return false;
	}
	std::stringstream FileContent;
	FileContent << File.rdbuf();

	std::istringstream Stream(FileContent.str());
	FString Line;
	while (std::getline(Stream, Line))
	{
		std::istringstream SS(Line);
		FString Keyword;
		FString Word;
		SS >> Keyword;

		if (Keyword == "newmtl")
		{
			FString Input;
			if (SS >> Word)
			{
				Input.append(Word);
			}
			while (SS >> Word)
			{
				Input.append("_").append(Word);
			}
			FName InName(Input);
			OutData.push_back(FObjMaterialInfo{InName});
			CurrentIndex = static_cast<int32>(OutData.size() - 1);
		}
		else if (CurrentIndex == INVALID_INDEX)
		{
			continue;
		}
		else if (Keyword == "Ka")
		{
			float x, y, z;
			SS >> x >> y >> z;
			OutData[CurrentIndex].KaAmbient = FVector(x, y, z);
		}
		else if (Keyword == "Kd")
		{
			float x, y, z;
			SS >> x >> y >> z;
			OutData[CurrentIndex].KdDiffuse = FVector(x, y, z);
		}
		else if (Keyword == "Ks")
		{
			float x, y, z;
			SS >> x >> y >> z;
			OutData[CurrentIndex].KsSpecular = FVector(x, y, z);
		}
		else if (Keyword == "Ke")
		{
			float x, y, z;
			SS >> x >> y >> z;
			OutData[CurrentIndex].KeEmissive = FVector(x, y, z);
		}
		else if (Keyword == "Ns")
		{
			float s;
			SS >> s;
			OutData[CurrentIndex].NsShininess = s;
		}
		else if (Keyword == "d")
		{
			float d;
			SS >> d;
			OutData[CurrentIndex].DOpacity = d;
		}
		else if (Keyword == "Tr")
		{
			float tr;
			SS >> tr;
			OutData[CurrentIndex].DOpacity = 1 - tr;
		}
		else if (Keyword == "illum")
		{
			int32 illum;
			SS >> illum;
			OutData[CurrentIndex].Illumination = illum;
		}
		else if (Keyword == "map_Kd")
		{
			std::string TextureFileName;
			SS >> TextureFileName;
			if (!TextureFileName.empty())
			{
				// 경로가 포함되어 있어도 순수 파일명(stem)만 추출
				// 예: "textures/MasterYi_Head.png" -> "MasterYi_Head"
				std::string StemName = std::filesystem::path(TextureFileName).stem().string();
				OutData[CurrentIndex].TextureName = StemName;
			}
		}
	}
	UE_LOG("%s .MTL 파일 열기 성공: mtl %d 개", FilePath.c_str(), OutData.size());
	return true;
}

void FObjDecoder::CheckSection(FObjVertexInfo& OutData, int32 InMaterialIndex, FName InGroupName)
{
	if (!OutData.Sections.empty())
	{
		FObjMeshSection& LastSection = OutData.Sections.back();
		if (LastSection.MaterialIndex == InMaterialIndex)
		{
			return;
		}
	}
	FObjMeshSection NewSection = {};
	NewSection.StartIndex = OutData.Indices.size();
	NewSection.IndexCount = 0;
	NewSection.MaterialIndex = InMaterialIndex;
	NewSection.GroupName = InGroupName;
	OutData.Sections.push_back(NewSection);
}