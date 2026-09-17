#include "FObjDecoder.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Rendering/FRenderer.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

TSortedMap<FString, FStaticMeshDecoder*> FObjDecoder::ObjStaticMeshMap;

namespace
{
	// 에셋 폴더는 실행 파일 기준으로 잡는다. (Binaries/x64/Debug → 3단계 위가 프로젝트 루트)
	std::filesystem::path GetAssetDir()
	{
		const std::filesystem::path ExeDir(GetExecutableDirectory());
		return ExeDir.parent_path().parent_path().parent_path() / "Resources" / "Asset";
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
		const std::filesystem::path AssetDir = GetAssetDir();
		const std::filesystem::path FilePath = AssetDir / FileName;

		if (!IsUnder(FilePath, AssetDir))
		{
			throw std::runtime_error("Attempted to read outside of the asset directory: " + FilePath.string());
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

	// 줄에서 float 를 최대 MaxCount 개 읽는다. 읽은 개수를 반환한다.
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
			static_cast<float>(A.V), static_cast<float>(B.V), static_cast<float>(C.V), 0.0f));
		ObjInfo.UVIndexList.push_back(FVector(
			static_cast<float>(A.VT), static_cast<float>(B.VT), static_cast<float>(C.VT)));
		ObjInfo.NormalIndexList.push_back(FVector(
			static_cast<float>(A.VN), static_cast<float>(B.VN), static_cast<float>(C.VN)));
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
	// 아직 처리하지 않는 키워드: vp, o, g, s, mtllib, usemtl, newmtl
}

FObjInfo FObjDecoder::ParseObjFile(const FString& File)
{
	ObjInfo = FObjInfo{};

	std::string_view Remaining = File;
	while (!Remaining.empty())
	{
		std::string_view Line;

		const size_t NewLine = Remaining.find('\n');
		if (NewLine == std::string_view::npos)
		{
			Line = Remaining;
			Remaining = {};
		}
		else
		{
			Line = Remaining.substr(0, NewLine);
			Remaining.remove_prefix(NewLine + 1);
		}

		ParseLine(Line);
	}

	return ObjInfo;
}

FObjInfo FObjDecoder::StartObjFileParser(const FString& PathFileName)
{
	try
	{
		const FString File = ReadFileToString(PathFileName);
		return ParseObjFile(File);
	}
	catch (const std::exception& e)
	{
		UE_LOG("FObjDecoder : %s", e.what());
	}
	return FObjInfo{};
}

FStaticMeshDecoder* FObjDecoder::LoadObjStaticMeshAsset(const FString& PathFileName)
{
	FObjDecoder Decoder;
	const FObjInfo Info = Decoder.StartObjFileParser(PathFileName);

	UE_LOG("FObjDecoder : %s  v=%zu vt=%zu vn=%zu tri=%zu",
		PathFileName.c_str(),
		Info.VertexList.size(), Info.UVList.size(),
		Info.NormalList.size(), Info.VertexIndexList.size());

	//  FObjInfo → FStaticMeshDecoder 변환 및 ObjStaticMeshMap 넣기
	return nullptr;
}
