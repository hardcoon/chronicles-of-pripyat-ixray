# Исходники движка Chronicles of Pripyat

Версия игры **2026.11.06-dev.108** · версия движка **IX-Ray Modified [eng-223]**.

В каталоге [`engine/`](engine/) находится снимок движка, из которого собрана публичная версия игры. В нём сохранены исходники игровых модулей и редакторов, рендереров, шейдеров, заголовки SDK и файлы CMake. Архивы игры, лаунчер и обновления для игроков доступны в [Releases](https://github.com/hardcoon/chronicles-of-pripyat-ixray/releases).

### Сборка для Windows x64

Для публичной сборки использовались MSVC 14.44.35207, Windows SDK 10.0.26100.0 и CMake 3.31.6. Установите Visual Studio 2022 с компонентами для разработки на C++, Git и CMake. Команды ниже выполняются в PowerShell из корня этого снимка. При первом конфигурировании CMake загружает зависимости из NuGet и GitHub.

```powershell
Set-Location engine
git clone --no-checkout https://github.com/ForserX/luabind-latest.git dep/luabind
git -C dep/luabind checkout --detach 128ba59572e508f0c550f4ff8a5235c07c77de3d
$luabind = (Resolve-Path dep/luabind).Path
cmake --preset Engine-x64-Windows "-DFETCHCONTENT_SOURCE_DIR_LUABIND=$luabind"
cmake --build --preset Engine-x64-Windows-Release --target xrGame
cmake --build --preset Engine-x64-Windows-RelWithDebInfo --target xrRender_R4
```

Версия SDL закреплена в CMake как `release-3.2.22`. Для дополнительных утилит компиляции уровней `xrLC` и `xrLC_Light` поместите два файла из [SDK исходной ревизии IX-Ray](https://github.com/ixray-team/ixray-1.6-stcop/tree/955cc805fd8e5609389598b57dff765fbc205b29/sdk) в `engine/sdk/libraries/x64/meshoptimizer.lib` и `engine/sdk/binaries/x64/meshoptimizer.dll`.

### Происхождение кода

За основу взята [ревизия IX-Ray 1.6 STCoP](https://github.com/ixray-team/ixray-1.6-stcop/tree/955cc805fd8e5609389598b57dff765fbc205b29). Снимок соответствует публичной сборке eng-223 выпуска dev.108. Шейдеры движка находятся в [`engine/gamedata/shaders/`](engine/gamedata/shaders/). Дополнительные игровые настройки, аддоны и их переопределения поставляются с самой игрой.

Авторские уведомления IX-Ray и сторонних библиотек сохранены в исходных файлах. [Лицензия IX-Ray](engine/doc/LICENSE.rus.md) · [Участники проекта](engine/doc/CONTRIBUTORS.rus.md)
