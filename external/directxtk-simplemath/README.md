# DirectXTK SimpleMath

`SimpleMath.h`, `SimpleMath.inl`, `SimpleMath.cpp` are copied from
https://github.com/microsoft/DirectXTK (commit `e31f076`).

**Used on macOS only.** For Windows SimpleMath from the vcpkg `directxtk` package is used,
which the D3D11 backend also needs for `WICTextureLoader`. DirectXTK has no macOS port,
but SimpleMath itself is portable, so we compile this copy there. DirectXMath
(the layer underneath) comes from the vcpkg `directxmath` port on macOS, and that port
ships the `sal.h` stub that non-Windows builds need.
