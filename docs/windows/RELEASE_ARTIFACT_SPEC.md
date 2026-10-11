# Windows SDK release assets

SDK identity comes from `SDK_VERSION`; the first SDK candidate is `0.1.0-rc.1`. The owner publication tag is `sdk/windows/v0.1.0-rc.1`, bound to the exact clean tested source commit. GitHub publication is a prerelease and is not marked latest.

Five assets are published:

- `aeostara-windows-sdk-0.1.0-rc.1-msvc-v143-x64.zip`
- `Aeostara.Windows.SDK.0.1.0-rc.1.nupkg`
- `aeostara-windows-sdk-developer-kit-0.1.0-rc.1.zip`
- `release-manifest.json`
- `SHA256SUMS`

The binary ZIP contains one versioned SDK root with `include/AeostaraCore`, `lib/x64/Debug`, `lib/x64/Release`, `lib/cmake/Aeostara`, `share/aeostara` and a standalone README. The native NuGet package contains equivalent header/library/manifest bytes plus `build/native` integration metadata. The developer kit contains only the synthetic public SDK client, build metadata, integration guide and legal notices.

Private implementation bodies, tests, CLI binaries, upstream schema/reference corpora, controlling instruction/evidence package, raw incident captures, development collector and external toolchain/dependency caches are excluded. Public headers and static library debug information remain deliberate ABI/developer payload. The applicable license is Apache-2.0; the ordinary static SDK is unsigned.

The SDK manifest lists source identity, compatibility tuple, semantic pins/supplement and payload hashes. The release manifest lists the three package hashes; `SHA256SUMS` covers those packages and the release manifest. A checksum file does not hash itself. Repeated staging controls ZIP timestamps and entry order. A separate clean compiler build measures actual debug/archive differences; compiler bit repeatability is never inferred from deterministic packaging.

Package consumers must use the matching qualified Microsoft runtime/toolchain. There is no NuGet.org publication. The local folder feed and download verification are part of release acceptance. After publication, every asset is downloaded afresh, hashed against prepared bytes, inspected and exercised through the downloaded public SDK.
