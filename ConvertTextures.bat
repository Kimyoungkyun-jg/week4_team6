@echo off

cd /d"%~dp0\Resources\Textures"

for %%f in  (*.png) do (
	..\..\Tools\texconv.exe -m 0 -f BC1_UNORM -y -o "..\Textures" "%%f"
)

cd /d"%~dp0\Resources\Edit"

for %%f in  (*.png) do (
	..\..\Tools\texconv.exe -m 0 -f BC1_UNORM -y -o "..\Edit" "%%f"
)

cd /d"%~dp0\Resources\Textures\Fonts"

for %%f in  (*.png) do (
	..\..\..\Tools\texconv.exe -m 0 -f R8G8B8A8_UNORM -y -o "..\..\Textures" "%%f"
)
