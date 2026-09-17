#include "FObjDecoder.h"
#include "Runtime/Core/Log.h"
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <algorithm>

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

	// v, vt, vn 인덱스 묶음 키
	struct FObjIndexKey
	{
		int32 VIndex = 0;
		int32 VTIndex = 0;
		int32 VNIndex = 0;

		bool operator==(const FObjIndexKey& Other) const
		{
			return VIndex == Other.VIndex && VTIndex == Other.VTIndex && VNIndex == Other.VNIndex;
		}
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
	OutData.Vertices.clear();
	OutData.Indices.clear();
	OutData.TextureName = FName("None"); // 초기화
	OutData.bIsValid = false;

	TArray<FVector> RawPositions;
	TArray<FVector2> RawTexCoords;
	TArray<FVector> RawNormals;

	// 중복 정점 검출용 해시맵 (Key -> 생성된 정점 인덱스)
	std::unordered_map<FObjIndexKey, uint32, FObjIndexKeyHasher> UniqueVertexMap;

	std::istringstream Stream(FileContent);
	FString Line;

	while (std::getline(Stream, Line))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}

		std::istringstream LineStream(Line);
		FString Prefix;
		LineStream >> Prefix;
		
		if (Prefix == "mtllib")
		{
			std::string MtlFileName;
			LineStream >> MtlFileName;
			if (!MtlFileName.empty() && !BaseDirectory.empty())
			{
				// OBJ와 같은 폴더에 있는 .mtl 전체 경로 생성
				std::filesystem::path FullMtlPath = std::filesystem::path(BaseDirectory) / MtlFileName;
				// .mtl 파싱해서 텍스처 이름 가져오기
				FName FoundTex = ParseMtlTexture(FullMtlPath);
				if (!FoundTex.IsNone() && FoundTex != FName("None"))
				{
					OutData.TextureName = FoundTex;
				}
			}
		}
		else if (Prefix == "v")
		{
			// 정점 위치
			float X = 0.0f, Y = 0.0f, Z = 0.0f;
			LineStream >> X >> Y >> Z;
			RawPositions.push_back(FVector{ X, Y, Z });
		}
		else if (Prefix == "vt")
		{
			// 텍스처 좌표 (DirectX 좌표계를 위해 V축 반전)
			float U = 0.0f, V = 0.0f;
			LineStream >> U >> V;
			RawTexCoords.push_back(FVector2{ U, 1.0f - V });
		}
		else if (Prefix == "vn")
		{
			// 법선 벡터
			float NX = 0.0f, NY = 0.0f, NZ = 0.0f;
			LineStream >> NX >> NY >> NZ;
			RawNormals.push_back(FVector{ NX, NY, NZ });
		}
		else if (Prefix == "f")
		{
			// 면(Face) 토큰 파싱
			TArray<FObjIndexKey> FaceKeys;
			FString Token;
			while (LineStream >> Token)
			{
				FaceKeys.push_back(ParseFaceToken(Token));
			}

			if (FaceKeys.size() < 3)
			{
				continue;
			}

			// n각형을 삼각형 팬(0, i, i+1)으로 분할
			for (size_t i = 1; i + 1 < FaceKeys.size(); ++i)
			{
				FObjIndexKey Triangle[3] = { FaceKeys[0], FaceKeys[i], FaceKeys[i + 1] };

				for (int j = 0; j < 3; ++j)
				{
					const FObjIndexKey& Key = Triangle[j];
					auto It = UniqueVertexMap.find(Key);
					if (It != UniqueVertexMap.end())
					{
						// 이미 등록된 정점이면 인덱스만 재사용
						OutData.Indices.push_back(It->second);
					}
					else
					{
						// 새로운 정점 생성
						uint32 NewIndex = static_cast<uint32>(OutData.Vertices.size());
						UniqueVertexMap[Key] = NewIndex;
						OutData.Indices.push_back(NewIndex);

						FVertexData Vertex{};
						Vertex.r = 1.0f;
						Vertex.g = 1.0f;
						Vertex.b = 1.0f;
						Vertex.a = 1.0f;

						int32 VIdx = ResolveIndex(Key.VIndex, RawPositions.size());
						if (VIdx >= 0 && VIdx < static_cast<int32>(RawPositions.size()))
						{
							Vertex.x = RawPositions[VIdx].X;
							Vertex.y = RawPositions[VIdx].Y;
							Vertex.z = RawPositions[VIdx].Z;
						}

						int32 VTIdx = ResolveIndex(Key.VTIndex, RawTexCoords.size());
						if (VTIdx >= 0 && VTIdx < static_cast<int32>(RawTexCoords.size()))
						{
							Vertex.u = RawTexCoords[VTIdx].X;
							Vertex.v = RawTexCoords[VTIdx].Y;
						}

						int32 VNIdx = ResolveIndex(Key.VNIndex, RawNormals.size());
						if (VNIdx >= 0 && VNIdx < static_cast<int32>(RawNormals.size()))
						{
							Vertex.nx = RawNormals[VNIdx].X;
							Vertex.ny = RawNormals[VNIdx].Y;
							Vertex.nz = RawNormals[VNIdx].Z;
						}

						OutData.Vertices.push_back(Vertex);
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
	return true;
}
