# subface

An implementation of Loop subdivision surfaces.

# Compiling

```
git clone https://github.com/chaosink/subface.git
cd subface
mkdir build
cd build
cmake ..
make
```

# Usage

```
./subface model_file.obj
```

* camera controlling

key | function
-|-
(`Shift` +) `W`/`S`/`A`/`D`/`Q`/`E` | (slowly) move forward / backward / left / right / down / up
(`Shift` +) `J`/`L`/`I`/`K` | (slowly) turn left / right / up / down
`-`/`=` | slow down / speed up movement
`[`/`]` | slow down / speed up turning
`Space` | reset camera
`mouse scroll wheel` | adjust field of view

* rendering

key | function
-|-
`0` - `9` | subdivision level, `0` for the original mesh(default)
`Tab` | switch rendering mode: face, wireframe, face + wireframe(default)
`N` | enable / disable(default) smooth rendering
`C` | enable(default) / disable face culling
`O` | export the subdivided mesh as OBJ file

# Results

* original mesh, faces

![level-0_face.png](./result/level-0_face.png)

* original mesh, wireframe with face culling

![level-0_wireframe_cull-face.png](./result/level-0_wireframe_cull-face.png)

* original mesh, wireframe without face culling

![level-0_wireframe_no-cull-face.png](./result/level-0_wireframe_no-cull-face.png)

* original mesh, flat

![level-0_flat.png](./result/level-0_flat.png)

* original mesh, smooth

![level-0_smooth.png](./result/level-0_smooth.png)

* level 1, flat

![level-1_flat.png](./result/level-1_flat.png)

* level 1, smooth

![level-1_smooth.png](./result/level-1_smooth.png)

* level 2, flat

![level-2_flat.png](./result/level-2_flat.png)

* level 2, smooth

![level-2_smooth.png](./result/level-2_smooth.png)

* level 3, flat

![level-3_flat.png](./result/level-3_flat.png)

* level 3, smooth

![level-3_smooth.png](./result/level-3_smooth.png)

* level 4, flat

![level-4_flat.png](./result/level-4_flat.png)

* level 4, smooth

![level-4_smooth.png](./result/level-4_smooth.png)
