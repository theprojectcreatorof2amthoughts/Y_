# stdmodel — Modeling Mathematics Library

`stdmodel/` contains small, dependency-free helpers for procedural and 3D modeling scripts written in Y_.

## Modules

- `vector2.hly` — 2D dot/cross products, component arithmetic, squared lengths and distances, perpendicular vectors, and interpolation.
- `vector3.hly` — 3D dot/cross products, component arithmetic, squared lengths and distances, and interpolation.
- `primitives.hly` — dimensions, surface areas, and volumes for boxes, spheres, cylinders, cones, toruses, pyramids, and prisms.
- `transforms.hly` — component-wise translation, scaling, pivot scaling, range remapping, and clamping.
- `mesh.hly` — triangle area/cross-product calculations and basic mesh topology/count helpers.

## Importing

The build embeds every discovered `.hly` file, so import modules by their repository-relative path:

```yl
!#inclib %stdmodel/vector3.hly%
!#inclib %stdmodel/primitives.hly%
!#inclib %stdmodel/transforms.hly%

stash dot = model_vec3_dot(1, 2, 3, 4, 5, 6)
stash volume = model_box_volume(2, 3, 4)
stash moved_x = model_point_translate_x(10, 5)

writescrn >^ dot
writescrn >^ volume
writescrn >^ moved_x
```

## Current Y_ limitations

Y_ currently has no arrays, structs, matrix values, square-root function, or trigonometric built-ins. Accordingly, this first version uses scalar components rather than vector/mesh objects. Length helpers return **squared** lengths; triangle helpers return cross-product components or squared cross-product magnitude instead of requiring square roots. Cone lateral-area helpers accept a slant height.

All dimensions use the caller's chosen unit system. The library does not create or render geometry files; it provides the math needed to build procedural modeling tools on top of Y_.
