# Windows Setup Guide

Get the project building and running on Windows in under 10 minutes.

---

## Prerequisites

Install the following tools before continuing.  
All commands below are run in **PowerShell** or **Windows Terminal**.

| Tool | Version | Download |
|------|---------|----------|
| **Git** | ≥ 2.40 | https://git-scm.com/download/win |
| **CMake** | ≥ 3.20 | https://cmake.org/download/ (add to PATH during install) |
| **Visual Studio 2022** | Community is free | https://visualstudio.microsoft.com/ |

> During the Visual Studio installer select the workload  
> **"Desktop development with C++"** — this installs MSVC, the Windows SDK, and the C++ toolchain.

---

## 1. Clone the repository

```powershell
git clone https://github.com/MyNameIsDotPy/voxel-engine.git
cd voxel-engine
```

---

## 2. Configure with CMake

CMake will automatically download all dependencies (**GLFW**, **GLM**, **GLAD**) via `FetchContent` — no manual library installation needed.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
```

> **Note:** The first configure step fetches dependencies from GitHub; it may take 1–2 minutes depending on your connection.

---

## 3. Build

```powershell
cmake --build build --config Release
```

The compiled executable will be at:

```
build\Release\voxel-engine.exe
```

---

## 4. Run

```powershell
.\build\Release\voxel-engine.exe
```

The shaders are automatically copied next to the executable by the build system, so no extra steps are needed.

---

## Controls

| Input | Action |
|-------|--------|
| Left-mouse drag | Orbit the camera |
| Scroll wheel | Zoom in / out |
| `ESC` | Quit |

---

## Troubleshooting

### "CMake not found"
Make sure CMake was added to `PATH`. Re-run the CMake installer and check **"Add CMake to the system PATH"**, then restart your terminal.

### "No C++ compiler found"
Open the Visual Studio Installer, select **Modify** on VS 2022, and ensure **"Desktop development with C++"** is checked.

### "Failed to open shader file"
Always run the executable from the directory that contains the `shaders/` folder, or run it directly from `build\Release\` where the build system copies them.

### OpenGL version errors
Your GPU must support **OpenGL 3.3 core**. Update your graphics drivers if you see context-creation errors. Intel integrated graphics from 2012+ and any discrete GPU from 2010+ should be fine.
