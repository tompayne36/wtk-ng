# WTK-NG Archaeology Report — Space Rocks

**Scope:** read-only analysis of this repository. No WTK-NG implementation has
been started. The repository is a flat, legacy Windows project directory rather
than a Git checkout; the original `wt.h` and WTK libraries are not included.

## Executive finding

Space Rocks is a good first target for a *thin C-compatible WTK façade*, not a
full game engine. Its runtime needs a retained scene graph, deterministic
frame/task traversal, transforms, procedural and NFF geometry, TGA/JPEG
textures, WAV playback, mouse/keyboard input, a desktop window, and a small
immediate overlay API. Optional legacy UI, stereo, networking, and hardware
sensors should be isolated behind replaceable adapters.

The first useful desktop milestone is a non-GUI Space Rocks build that preserves
the original `rocks_gui.c` gameplay source and runs with mouse/keyboard,
textured scene, ship, rocks, missiles, collisions, and audio. It may initially
omit the legacy WTK widget UI, external sensors, stereo, and networking.

## Repository inventory

### Source and project material

| Kind | Files | Notes |
|---|---|---|
| Main game | `rocks_gui.c` (1,529 lines) | main loop; scene construction; gameplay; overlays |
| Supporting game code | `gui.c` (1,203), `misc.c` (1,042), `mouse.c` (60), `sensor.c` (118), `network.c` (359), `readini.c` (110) | GUI, utility/audio/animation, input, legacy hardware, optional multicast networking, settings |
| Headers | `rocks.h`, `globals.h` | game types, globals, compile-time asset names |
| Small sample | `Simple.c` (167) | separate WTK sample; not linked by Space Rocks |
| Legacy projects | `rocks.dsp/.dsw/.mak/.mdp/.ncb/.opt/.plg`; `SpaceRocks_{OGL,D3D,WSOGL}.*`; `wtkgui42.*`; `wtkdgui42.*` | Visual C++ 4.x-era targets for GUI/OpenGL/Direct3D and WorldServer variants |
| Historical binaries | `rocks.exe`, `rocks_nt.exe`, `.lib`, `.exp`, `.ilk`, `MFC42.DLL`, `MSVCRT.DLL` | reference artifacts only; do not build on them |

The normal GUI project links `wtkgui42.lib`, `wtkmfc42.lib`, `winmm.lib`,
`wsock32.lib`, `opengl32.lib`, and `glu32.lib`. The WorldServer variant also
links `client.lib` and `wtkmt.lib`. These identify the historical integration;
they are not usable implementation dependencies on macOS.

### Assets

| Asset group | Files | Source references / role |
|---|---|---|
| Models | `SHIP2.NFF`, `station.nff`, `UNIVERSE.NFF`, `gameover.nff`, `poly.nff`, `RCFONT3D.NFF` | ship, alien, textured space shell, 3D font, optional quad; loaded from `rocks_gui.c`/`misc.c` |
| Material sidecars | `ship.mat`, `station.mat`, `UNI` | implicitly associated by NFF material-table names (`ship`, `station`, `uni`) |
| Lights | `LIGHTS` | loaded by `WTlightnode_load(root, "lights")` |
| Static textures | `GRAVEL.TGA`, `SCREEN.TGA`, `SCREEN2.tga`, `MY_STARS.TGA`, `HOURGLS.TGA`, `M100.TGA`, `M31.TGA`, `DISKIL.TGA`, `Logo3r.tga`, `Logo3r000.tga` | asteroid, shield/explosion, universe shell, logo/HUD |
| Explosion sequence | `bang00.jpg`…`bang57.jpg` | `misc.c` loads `bang00`…`bang56`; all are 128×128 JPEGs; `bang57` is currently unreferenced |
| Audio | `explo.wav`, `humm.wav`, `clank.wav`, `gameover.wav`, `fire.wav`, `STARTUP.WAV`, `SHUTDOWN.WAV`, `alien2.wav` | loaded in `setup_sounds`; `atmos.wav` and `RICOCHET.WAV` are not referenced |
| GUI/text | `about.bmp`, `readme.txt`, `asteroids.txt`, `other_rocks.txt` | About dialog and readme; latter two are unreferenced reference text |
| Other / unreferenced | `ENTRPRS.FLT`, `RCFONT3D.NFF`, `Logo3r000.tga`, `WTKCODES`, `wtkcodes.000/.001` | FLT is only advertised in a chooser; 3D font file is not used (game loads `gameover.nff`) |

`DISKIL.TGA` is indexed 8-bit 64×64; other TGA assets are 24-bit, several RLE
encoded. WAV files are PCM mono (8-bit/11–22 kHz or 16-bit/22 kHz). The loader
must resolve case-insensitively for legacy content (`UNIVERSE.NFF` is requested
as `universe.nff`; similarly for several assets) or normalize packaged names.

### NFF evidence

The files are textual NFF, not a single simplistic format:

* v3.00 contains named objects, `mtable` references, vertices, polygons,
  `matid`, optional `norm x y z`, optional `uv u v`, optional `both`, and
  texture references such as `_V_my_stars.tga`.
* v2.10 (`gameover.nff`, font) contains vertex/polygon data and packed RGB
  polygon colours.
* `UNIVERSE.NFF` is a multi-object inward-facing textured shell.
* Sidecar material files define ambient/diffuse/specular/emission/shininess and
  alpha or opacity. `LIGHTS` defines an ambient light and a directed light.

Direct NFF parsing is practical and should be implemented before any conversion
pipeline. FLT, OBJ, VRML, 3DS, and DXF are only UI-advertised here; defer them
until a target actually needs them.

## Space Rocks WTK API inventory

The following symbols are referenced by the Space Rocks build sources (not the
separate `Simple.c`). Types are `FLAG`, `WTp2`, `WTp3`, `WTpq`, `WTq`, `WTm3`,
`WTnode`, `WTnodepath`, `WTgeometry`, `WTpoly`, `WTvertex`, `WTmtable`,
`WTviewpoint`, `WTwindow`, `WTtask` (opaque use), `WTsensor`,
`WTmouse_rawdata`, `WTsound`, `WTsounddevice`, `WTfont3d`, `WTui`,
`WTserialname`, and `WTtype`.

| Subsystem | Required symbols |
|---|---|
| Runtime | `WTuniverse_new/delete/ready/go/setactions/getrootnodes/getwindows/getviewpoints/getrendering/setrendering/framerate/avgframerate` |
| Math | `WTp3_init/copy/add/subtract/mults/norm/mag/distance/print`; `WTq_init/mult/2dir`; `WTeuler_2q/2m3`; `WTdir_2q` |
| Nodes/transforms | `WTgroupnode_new`; `WTmovnode_load/instance/attach/getattachment/numattachments/rotateaxis`; `WTmovgeometrynode_new`; `WTmovsepnode_new`; `WTmovswitchnode_new`; `WTswitchnode_setwhichchild`; `WTnode_{delete,enable,isenabled,getchild,numchildren,get/setdata,getgeometry,get/settranslation,get/setorientation,get/setrotation,translate,rotate,rotateq,getradius,getmidpoint,numpolys,print,setname}` |
| Paths/collision | `WTnodepath_new`, `WTnodepath_intersectbbox` |
| Geometry | `WTgeometry_{begin,close,newvertex,beginpoly,newblock,newsphere,getpolys,getvertices,get/setvertexposition,beginedit,endedit,recomputestats,scale,setrgb,settexture,changetexture,setmtable,setmatid,getradius}`; `WTpoly_{addvertex,addvertexptr,getvertex,numvertices,next,delete,close,settexture}`; `WTvertex_next` |
| Materials/textures/lights | `WTmtable_new/newentry/setvalue`; `WTtexture_load/cache/replace/setfilter/getmemory`; `WTlightnode_load` |
| View/window/render | `WTviewpoint_copy/getposition/setposition/setorientation/setparallax/getparallax/moveto`; `WTwindow_new/delete/next/getposition/getviewpoint/setviewpoint/set{bg,hi,yon}value/setfgactions/setdrawfn/numpolys/zoomviewpoint/zoomviewtonode`; 2D `setcolor/setlinewidth/setlinestyle/drawline/drawtext/drawtexture/gettextextents`; 3D `setcolor/setpointsize/drawpoints/drawlines` |
| Task scheduling | `WTtask_new` |
| Input/sensors | `WTkeyboard_open/getkey`; `WTsensor_{get/set sensitivity,get/set angularrate,setupdatefn,get/setrecord,getrawdata,getrotation,getmiscdata}`; `WTmouse_new/rawupdate/whichwindow`; constructors for Bird, Boom, CrystalEyesVR, Fastrak, Formula, Geoball, 5DT Glove, i-glasses, InsideTrak, Isotrak II, JoySerial, Logitech, Polhemus, Precision, Red Baron, SpaceBall, SpaceControl |
| Audio | `WTsounddevice_open/close/update/setparam`; `WTsound_load/delete/play/stop/setparam/setposition/setnodepath/setdonefn` |
| UI | `WTui_init/go/manage/delete`; constructors for form, WTK window, label, frame, menubar, popup, menu item, pushbutton, scrolled list/text, scale, file selection, text input, message box; `WTui_setcallback/insertitem/dimitem/setmenutext/gettext` |
| Fonts | `WTfont3d_load/getspacing`; `WTgeometry_newtext3d` |
| Optional network | `WTnet_open/additem/next/removeitem`; conditional `WTclient_initialize/shareproperty` |

Relevant constants/macros include display/window modes, render masks, frame
spaces, sound device/parameter constants, material properties, texture filters,
image format, UI events/attachment attributes, mouse buttons, arrows, line
mode, `WTNODE_TRANSFORM`, and `SERIAL1/2`. Exact historical numeric values are
only necessary where application code combines masks (notably render flags).

## Behavioral semantics to preserve

1. **Transform spaces matter.** Rocks and missiles translate in `WTFRAME_PARENT`;
   ship thrust and sensor rotation use `WTFRAME_LOCAL`. The scene graph must
   compose parent/world transforms correctly. Node paths are formed from node to
   root and collisions occur in world space.
2. **Tasks are mutation-capable.** A per-node task can delete its own node.
   Traverse against a stable next pointer/snapshot, and define deletion-safe
   ownership. Tasks scale with `deltaT()` from the game, so scheduler cadence
   must be stable but need not impose fixed gameplay time.
3. **Frame ordering is observable.** The universe action moves the ship, tests
   collisions and spawns entities; node tasks then update rocks/missiles/alien
   and effects; rendering draws the resulting graph plus overlays. Validate this
   ordering against the historical executable when available. Input must be
   available before `actions()`.
4. **Bounding-box collision is sufficient initially.** `WTnodepath_intersectbbox`
   is used for missile/rock, ship/shield/rock, and optional rock/rock tests.
   Rock/rock also has a distance-radius alternative. Do not substitute mesh
   collision in Phase 1.
5. **Node attachment and instancing are meaningful.** The shield attaches to
   the ship; alien instances a cached loaded model. Geometry may be shared but
   transforms/data must be per-instance.
6. **Dynamic geometry/material updates are visible.** Rocks are triangulated and
   vertex-distorted; explosion textures are changed every frame; shield and
   explosion opacity update through material-table entries. Material state must
   be per geometry/material ID rather than a global colour.
7. **Camera semantics include two viewpoints.** Outside and cockpit views switch;
   the cockpit camera tracks ship world position/orientation each action. Near,
   far, parallax, perspective, and stereo settings are exposed. Stereo can be
   deferred but parallax must remain stored and harmless.
8. **Audio is spatial.** Sound positions and node paths exist; an initial
   implementation may map position to pan/attenuation, with listener at active
   viewpoint and rolloff near `UNIVERSE_SIZE`.
9. **UI and callbacks are re-entrant concerns.** Legacy menu/dialog callbacks
   alter rendering, recreate entities, and delete UI. Keep the UI adapter out
   of core scene ownership.

## What Space Rocks needs, by priority

| Classification | Scope |
|---|---|
| Initial executable | C ABI/header, math/quaternion, universe loop, one desktop window, keyboard/mouse, scene graph/transforms, nodes/groups/switches/attachments/instances, geometry primitives and dynamic edit, NFF+MAT+LIGHTS loader, TGA/JPEG textures, render flags sufficient for textured/shaded/wireframe, camera, AABB collision, tasks, 2D/3D overlays, WAV sound |
| Complete Space Rocks gameplay | animated JPEG explosion textures and alpha, material table opacity, 3D-font mesh/text, ship shield, alien, score/game-over, optional second viewpoint, render/texture menu modes |
| Useful for Rover/Sailing later | articulated hierarchy/pivots, generic model loaders (3DS/DXF/FLT), animation/morphing, terrain, richer paths/object names, navigation, sensor abstraction, lights/textures/material variants, networking semantics |
| Stub initially | MFC-style `WTui` widgets/file chooser, all named serial/VR device constructors, CrystalEyes/red-blue/two-window stereo, WorldServer APIs, multicast `WTnet`, alternate historical sound drivers |
| Defer/obsolete | real serial device support, proprietary HMD/display drivers, DiamondWare/VSI/Crystal River/SGI sound backends, direct dependency on MFC, Winsock-1 APIs, historical licensing/code files |

The executable milestone can avoid the GUI branch by setting `use_gui = FALSE`
or supplying a compatibility UI shim. Keeping the GUI source compilable is
still required; no rewrite to a new application architecture is justified.

## Non-WTK dependencies and modernization issues

* Standard C/POSIX: `stdio`, `stdlib`, `string`, `math`, and `gethostname`
  (WorldServer only). Code includes `<string.h>` but uses legacy Windows
  `lstrcat` in the conditional WorldServer path.
* Windows-specific build/runtime assumptions: Visual C++ 4.x, MFC GUI libs,
  WinMM, Winsock, OpenGL 1.x/GLU, DirectSound/Direct3D project variants, PE
  binaries and bundled CRT/MFC DLLs.
* C modernization: `void main`, implicit declarations/prototypes, a definition-
  containing `globals.h` (unsafe with modern separate compilation), old pointer-
  to-int UI casts, potential NULL sensor dereference, and legacy spelling/errors.
  These are acceptable small, documented application-source fixes.
* Networking is unfinished/buggy by inspection (fall-through cases, payload
  inconsistencies, and `use_network == TRUE` typo) and is not a Phase-1 gate.

## Rover and Sailing incremental evidence

Neither a Rover tree nor `MK22.CPP` is present in this repository. Therefore
the contextual list of likely incremental needs is recorded as a validation
plan, not asserted as source-derived fact:

* **Rover:** terrain, articulated/hierarchical attachments, tasks and event
  ordering, sensors, viewpoints, lighting, navigation, textures, and networking.
* **Sailing:** C++ linkage to C API, articulated pivots, animation/morphs,
  geometry/polygon mutation, simulation math, and NFF/3DS/DXF model paths.

When supplied, inventory each source tree with the same symbol extraction and
run it as a regression target before expanding WTK-NG beyond a demonstrated
need. Rover must remain private unless separately licensed for redistribution.

## Recommended WTK-NG architecture

```
unchanged legacy application C/C++
              │  wt.h (stable C ABI)
              ▼
WTK-NG compatibility core
  math · scene graph · geometry/assets · tasks · camera · render commands
              │
platform adapters: desktop window/input · renderer · audio · optional UI/network
```

**Core:** modern C++ internally with opaque C handles and `extern "C"` exports.
Use RAII/shared asset ownership internally while preserving legacy explicit
delete calls. Separate immutable mesh/texture assets from node instances. Keep
the render command layer independent of a graphics API so OpenXR can later add
a different presentation/camera adapter without changing WTK application code.

**Renderer requirements from evidence:** indexed triangle/quad meshes; per-
vertex position, normal and UV; opaque/alpha material passes; directional plus
ambient light; texture upload and runtime pixel replacement; nearest/linear/
mipmap filters; dynamic buffers for geometry edits; wireframe and untextured
debug modes; 2D and simple 3D line/point/text overlays; perspective camera;
one or more viewports. No evidence yet requires skeletal animation, PBR,
physics, ECS, or a full game engine.

**Best Phase-1 technology choice on macOS:** SDL3 (or SDL2 if maturity wins) for
window/input/audio-device glue; a small OpenGL 3.3-core renderer using SDL's GL
context; `stb_image` for TGA/JPEG/BMP decode; an internal NFF/MAT parser; and
miniaudio or SDL audio for WAV playback/mixing. This is thin, portable to
Windows/Linux, and maps directly to the historical fixed-function feature set.

Alternatives:

| Choice | Benefit | Cost/risk |
|---|---|---|
| SDL + OpenGL core | smallest conceptual bridge from original OGL project; easy debug modes; Windows/Linux portability | macOS OpenGL is deprecated; needs a disciplined compatibility renderer |
| SDL + bgfx | backend portability (Metal/Vulkan/D3D/GL) | more abstraction and shader/build complexity for a small first target |
| SDL + Metal | strongest native macOS path | a Windows/Linux backend must be added sooner; less direct legacy bridge |
| GLFW + miniaudio | minimal windowing/input plus independent audio | must separately implement controller/device abstractions and UI glue |
| Full engine | editor/tooling | violates the desired thin compatibility boundary and makes source semantics harder to preserve |

Choose based on a short renderer spike only after validating texture alpha,
dynamic mesh updates, overlays, and two camera/viewports—not familiarity.

## Proposed phases

1. **Freeze evidence and define ABI.** Preserve this report; derive a minimal
   `wt.h` declaration set from the API table; add a compile-only Space Rocks
   target. Document each deliberate application compatibility edit.
2. **Headless compatibility core.** Math, node ownership/transforms, paths,
   AABB, task scheduler, geometry/material/texture data, and NFF/MAT/LIGHTS
   parser. Unit-test world/local transforms, attach/instance behavior, deletion
   during tasks, and NFF fixtures from this repository.
3. **Desktop rendering and input.** Window, event pump, keyboard/mouse sensor,
   camera, scene renderer, TGA/JPEG, texture filters, dynamic geometry and
   2D/3D overlays. Reach an interactive visible scene.
4. **Playable game.** Audio, animation sequence, shield/material alpha,
   game-over text, all normal gameplay paths; compare controls and timing with
   historical behavior where the legacy executable can be run.
5. **Compatibility/UI expansion.** WTK UI subset or an optional native adapter,
   stereo/multi-window, additional loaders only as required.
6. **Rover then Sailing.** Inventory first, add features proven necessary, and
   preserve private/reference boundaries.

## Risks and open questions

* Original `wt.h` and library behavior are absent; exact signatures, ownership,
  default rendering flags, quaternion conventions, and task ordering need
  recovery from an original SDK, binary observation, or compile diagnostics.
* Case-insensitive legacy asset lookup must be deliberately supported on macOS.
* Historical assets contain multiple NFF versions and material conventions; the
  parser needs fixture-driven validation before broader format support.
* `WTui` may be disproportionately expensive to faithfully emulate. Keep it
  source-compatible with a minimal adapter but do not make it a gameplay gate.
* Frame-rate-dependent legacy tuning (`deltaT`, `sens`) needs clamping/testing
  to avoid unstable modern high-refresh behavior while retaining feel.
* Audio API semantics (looping, completion callback, node-path spatialization)
  need a stated approximation contract.

## Clearly defined first executable milestone

On macOS, build and launch the original Space Rocks sources (including
`rocks_gui.c`) against WTK-NG and original assets, with **no source rewrite into
another engine**. In a single desktop window it must: load the ship, station,
universe, lights and textures; render textured/shaded and wireframe variants;
accept keyboard/mouse flight and fire; spawn/move/rotate rocks; move missiles;
perform AABB hit detection; update score/lives; and play WAV effects. The
legacy GUI, stereo, networking, non-mouse devices, and non-NFF formats may be
stubbed with explicit diagnostics for this milestone.

