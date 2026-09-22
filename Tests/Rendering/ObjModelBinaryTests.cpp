// Todo: Bin - 이 파일 전체는 디코더의 파싱/직렬화/캐시 분리 검증이다.
#include "Runtime/Rendering/FBinArchive.h"
#include "Runtime/Rendering/FWindowsBinWriter.h"
#include "Runtime/Rendering/FWindowsBinReader.h"
#include "Runtime/Rendering/FObjDecoder.h"
#include "Runtime/Core/Log.h" // Todo: Bin - 통합 테스트 실패 시 파서 로그 출력.
#include <filesystem>
#include <iostream>
#include <stdexcept>

static void Check(bool Success, const char* Message)
{
    if (!Success) throw std::runtime_error(Message);
}

int main()
{
    try
    {
        FObjModelData Model;
        Model.PathFileName = "test-mesh.obj";
        Model.Vertices.resize(3);
        Model.Vertices[1].x = 1.0f;
        Model.Vertices[2].y = 1.0f;
        Model.Vertices[1].u = 0.25f;
        Model.Vertices[2].nz = -0.5f;
        Model.Vertices[2].tz = 0.75f;
        Model.Indices = { 0, 1, 2, 2, 1, 0 };
        FMeshSection Section;
        Section.IndexCount = 3;
        Section.MaterialName = "Body";
        Section.Opacity = 0.5f;
        Section.bIsAlpha = true;
        Section.IlluminationModel = 2;
        Section.LocalBounds.Min = FVector(-1, -2, -3);
        Section.LocalBounds.Max = FVector(1, 2, 3);
        Model.Sections.push_back(Section);
        Section.FirstIndex = 3;
        Section.MaterialName = "Glass";
        Model.Sections.push_back(Section);
        FObjMaterialInfo Material;
        Material.MaterialName = "Body";
        Material.Diffuse = FVector(0.1f, 0.2f, 0.3f);
        Material.Opacity = 0.5f;
        Material.IlluminationModel = 2;
        Material.DiffuseTextureName = "body.png";
        Material.NormalTextureName = "normal.png";
        Material.DecalTexture = "decal.png";
        Model.Materials.push_back(Material);
        Material.MaterialName = "Glass";
        Material.Opacity = 0.2f;
        Model.Materials.push_back(Material);
        Model.Groups.push_back({ "Group" });
        Model.ObjectNames.push_back({ "Object" });
        Model.TextureName = FName("body");
        Model.NormalTextureName = FName("normal");

        FBinArchive Original;
        Check(Original.SerializeObjModel(Model), "serialize");
        Check(Original.GetBytes()[0] == 'O' && Original.GetBytes()[3] == 'M', "file signature");
        FObjModelData Loaded;
        Check(Original.DeserializeObjModel(Loaded), "deserialize");
        Check(Loaded.bIsValid && Loaded.Vertices.size() == 3 && Loaded.Indices == Model.Indices, "geometry");
        Check(Loaded.Vertices[1].u == 0.25f && Loaded.Vertices[2].tz == 0.75f, "vertex attributes");
        Check(Loaded.Sections.size() == 2 && Loaded.Sections[1].FirstIndex == 3
            && Loaded.Sections[0].LocalBounds.Min.Y == -2, "sections");
        Check(Loaded.Materials.empty(), "mesh cache must not contain material definitions");
        Check(Loaded.Groups[0].Name == "Group" && Loaded.ObjectNames[0].Name == "Object", "metadata");
        // Todo: Bin - Materials.bin은 경로 Entry 없이 머티리얼 배열을 직접 저장한다.
        TArray<FObjMaterialInfo> MaterialEntries = Model.Materials;
        FBinArchive MaterialArchive;
        Check(MaterialArchive.SerializeMaterials(MaterialEntries), "materials serialize");
        TArray<FObjMaterialInfo> RestoredMaterials;
        Check(MaterialArchive.DeserializeMaterials(RestoredMaterials), "materials deserialize");
        Check(RestoredMaterials[0].NormalTextureName == "normal.png"
            && RestoredMaterials[1].Opacity == 0.2f
            && RestoredMaterials[1].DecalTexture == "decal.png", "material fields");
        for (size_t Length = 0; Length < MaterialArchive.GetBytes().size(); ++Length)
        {
            FBinArchive Truncated;
            const auto& AllBytes = MaterialArchive.GetBytes();
            Truncated.SetBytes(TArray<uint8>(AllBytes.begin(), AllBytes.begin() + Length));
            Check(!Truncated.DeserializeMaterials(RestoredMaterials), "truncated materials");
        }
        FBinArchive Resaved;
        Check(Resaved.SerializeObjModel(Loaded) && Original.GetBytes() == Resaved.GetBytes(), "byte round trip");

        const auto Bytes = Original.GetBytes();
        // 어느 위치에서 파일이 잘리더라도 실패하고 호출자의 모델을 유지해야 한다.
        for (size_t Length = 0; Length < Bytes.size(); ++Length)
        {
            FBinArchive Truncated;
            Truncated.SetBytes(TArray<uint8>(Bytes.begin(), Bytes.begin() + Length));
            Loaded.PathFileName = "unchanged";
            Check(!Truncated.DeserializeObjModel(Loaded) && Loaded.PathFileName == "unchanged", "truncated input");
        }
        // Todo: Bin - 버전 필드 제거 후 시그니처와 문자열 길이 위치를 손상시킨다.
        for (size_t Offset : { size_t(0), size_t(4) })
        {
            auto Corrupt = Bytes;
            Corrupt[Offset] = 0xff;
            FBinArchive Invalid;
            Invalid.SetBytes(std::move(Corrupt));
            Check(!Invalid.DeserializeObjModel(Loaded), "invalid header or length");
        }
        auto Trailing = Bytes;
        Trailing.push_back(0);
        FBinArchive Extra;
        Extra.SetBytes(std::move(Trailing));
        Check(!Extra.DeserializeObjModel(Loaded), "trailing bytes");
        Model.Indices[0] = 500;
        Check(!Resaved.SerializeObjModel(Model), "invalid vertex index");
        Model.Indices[0] = 0;
        Model.Sections[0].IndexCount = 900;
        Check(!Resaved.SerializeObjModel(Model), "invalid section range");

        const auto Directory = std::filesystem::path("Intermediate") / "ObjBinaryTests";
        std::filesystem::create_directories(Directory);
        const auto Path = Directory / "roundtrip.bin";
        Check(FWindowsBinWriter::Save(Path, Original), "file save");
        FBinArchive FromFile;
        Check(FWindowsBinReader::Load(Path, &FromFile) && FromFile.GetBytes() == Bytes, "file load");
        Check(FromFile.DeserializeObjModel(Loaded), "model file load");
        Check(Resaved.SerializeObjModel(Loaded)
            && FWindowsBinWriter::Save(Directory / "resaved.bin", Resaved), "model file save");
        Check(!FWindowsBinReader::Load(Directory / "missing" / "absent.bin", &FromFile)
            && FromFile.GetBytes() == Bytes, "missing file preserves archive");
        Check(!FWindowsBinWriter::Save(Directory / "missing" / "absent.bin", Original), "write failure");

        // Todo: Bin - 실제 OBJ/MTL 텍스트로 캐시 존재/부재 조합과 머티리얼 공유를 검증한다.
        const auto Fixture = std::filesystem::absolute(Directory / ("fixture-" + std::to_string(GetTickCount64())));
        std::filesystem::create_directories(Fixture);
        auto WriteText = [](const std::filesystem::path& File, const FString& Text) {
            std::ofstream Output(File, std::ios::binary | std::ios::trunc);
            Output << Text;
            Check(static_cast<bool>(Output), "fixture write");
        };
        const FString Geometry =
            "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
            "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\n"
            "usemtl Body\nf 1/1/1 2/2/1 3/3/1\n";
        const FString FirstObj = "mtllib Common.mtl\n" + Geometry;
        WriteText(Fixture / "Common.mtl", "newmtl Body\nKd 1 0 0\nd 0.25\nmap_Kd body.png\n");
        WriteText(Fixture / "Other.mtl", "newmtl Glass\nKd 0 1 0\nd 0.8\n");
        // Todo: Bin - 여러 MTL에 같은 전역 이름이 반복되면 첫 정의를 공유한다.
        WriteText(Fixture / "ZZDuplicate.mtl", "newmtl Body\nKd 0 0 1\nd 0.1\n");
        WriteText(Fixture / "First.obj", FirstObj);
        // Todo: Bin - 머티리얼 이름은 전역적으로 유일하다.
        const FString SecondGeometry =
            "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
            "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\n"
            "usemtl Glass\nf 1/1/1 2/2/1 3/3/1\n";
        WriteText(Fixture / "Second.obj", "mtllib Other.mtl\n" + SecondGeometry);
        FObjDecoder FirstDecoder;
        Check(FirstDecoder.LoadMaterials(Fixture.string()), "initial MTL parse");
        Check(FirstDecoder.GetMaterials().size() == 2
            && FirstDecoder.GetMaterials()[0].MaterialName == "Body"
            && FirstDecoder.GetMaterials()[0].Opacity == 0.25f, "global duplicate material uses first definition");
        const auto MaterialCache = Fixture / "Materials.bin";
        const auto MaterialTime = std::filesystem::last_write_time(MaterialCache);
        FObjModelData FirstMesh, SecondMesh;
        Check(FirstDecoder.LoadObj((Fixture / "First.obj").string(), (Fixture / "First.bin").string(), FirstMesh), "initial OBJ parse");
        Check(FirstDecoder.LoadObj((Fixture / "Second.obj").string(), (Fixture / "Second.bin").string(), SecondMesh), "second OBJ parse");
        Check(FirstMesh.Sections[0].MaterialName == "Body"
            && FirstMesh.Sections[0].Opacity == 0.25f, "section references common material");
        Check(SecondMesh.Sections[0].MaterialName == "Glass"
            && SecondMesh.Sections[0].Opacity == 0.8f, "section references global material name");
        Check(std::filesystem::last_write_time(MaterialCache) == MaterialTime, "OBJ load must not rewrite Materials.bin");

        // MTL을 지워도 Materials.bin이 있으면 파싱 없이 복원해야 한다.
        std::filesystem::remove(Fixture / "Common.mtl");
        std::filesystem::remove(Fixture / "Other.mtl");
        std::filesystem::remove(Fixture / "ZZDuplicate.mtl");
        WriteText(Fixture / "First.obj", "invalid source: cache must be used");
        FObjDecoder CachedDecoder;
        Check(CachedDecoder.LoadMaterials(Fixture.string()), "material cache hit");
        Check(CachedDecoder.LoadObj((Fixture / "First.obj").string(), (Fixture / "First.bin").string(), FirstMesh), "OBJ cache hit without reparsing");
        WriteText(Fixture / "Third.obj", FirstObj);
        FObjModelData ThirdMesh;
        Check(CachedDecoder.LoadObj((Fixture / "Third.obj").string(), (Fixture / "Third.bin").string(), ThirdMesh), "OBJ miss with cached materials");
        Check(ThirdMesh.Sections[0].MaterialName == FirstMesh.Sections[0].MaterialName, "shared material name");
        Check(std::filesystem::last_write_time(MaterialCache) == MaterialTime, "OBJ miss must not rewrite Materials.bin");

        // 머티리얼 캐시만 없을 때는 MTL만 다시 만들고 기존 OBJ 캐시는 사용할 수 있다.
        WriteText(Fixture / "Common.mtl", "newmtl Body\nKd 1 0 0\nd 0.9\n");
        std::filesystem::remove(MaterialCache);
        FObjDecoder RebuiltDecoder;
        Check(RebuiltDecoder.LoadMaterials(Fixture.string()), "material cache rebuild");
        Check(RebuiltDecoder.LoadObj((Fixture / "First.obj").string(), (Fixture / "First.bin").string(), FirstMesh), "independent OBJ cache");
        Check(FirstMesh.Sections[0].Opacity == 0.9f, "cached section uses current shared material");

        WriteText(MaterialCache, "broken");
        WriteText(Fixture / "First.bin", "broken");
        WriteText(Fixture / "First.obj", FirstObj);
        FObjDecoder RecoveryDecoder;
        Check(RecoveryDecoder.LoadMaterials(Fixture.string()), "corrupt material cache recovery");
        Check(RecoveryDecoder.LoadObj((Fixture / "First.obj").string(), (Fixture / "First.bin").string(), FirstMesh), "corrupt OBJ cache recovery");

        std::cout << "PASS: serialization, corruption, file IO, OBJ/MTL cache combinations, shared material references\n";
        return 0;
    }
    catch (const std::exception& Error)
    {
        std::cerr << "FAIL: " << Error.what() << '\n';
        for (const auto& Message : FLogManager::Get().GetLogs()) std::cerr << Message << '\n';
        return 1;
    }
}
