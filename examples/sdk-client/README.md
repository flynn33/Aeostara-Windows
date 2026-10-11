# Installed SDK client

This client includes installed public headers and links the distributed static library. It exercises an authorized four-coordinate carrier repair, policy rejection with unchanged bytes, a forced post-write verification failure followed by independently checked restoration, and a successful read-only STABLE/NO_ACTION lifecycle. The read-only case captures its actual native target, validates and plans without operations, measures zero configuration writes, target lease requests or backup calls, independently reconstructs its required business audit from storage, and exports a redacted JSON/Markdown incident pair. Every case exports a paired incident bundle. The data is synthetic and opt-in; the adopted carrier profile does not authorize arbitrary application configuration repair.

Use Visual Studio 2022 v143 with MSVC 19.44.35223, VCToolsVersion 14.44.35207, Windows SDK 10.0.26100.0, C++20, exceptions and RTTI. Select x64, Debug with /MDd or Release with /MD. Debug and Release libraries must match the host configuration. Debug use requires the installed development runtime.

For CMake, set `CMAKE_PREFIX_PATH` to the extracted SDK root:

```powershell
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 -T v143 '-DCMAKE_SYSTEM_VERSION=10.0.26100.0' '-DCMAKE_PREFIX_PATH=C:/SDK/Aeostara'
cmake --build build --config Release
.\build\Release\AeostaraSdkClient.exe synthetic-input.json C:\SDK\new-example-output
```

For native NuGet, place the release `.nupkg` in a local folder feed, restore `packages.config` to this example's `packages` directory, then build the project. No remote feed is required:

```powershell
nuget restore packages.config -Source C:\SDK\local-feed -PackagesDirectory packages -NonInteractive
msbuild AeostaraSdkClient.vcxproj /p:Configuration=Release /p:Platform=x64
.\bin\Release\AeostaraSdkClient.exe synthetic-input.json C:\SDK\new-nuget-output
```

Each output directory must be new. The example writes only its synthetic configuration, backup, audit and diagnostic files beneath that directory. An actual export failure is reported as incomplete; a successful restore does not turn a failed healing attempt into success. Representative target-machine storage, ACL, crash and environment validation remains an owner deployment step.
