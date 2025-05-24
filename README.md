# Watermill :)

**This project goal is to practice shaders and visual effects**
<!-- 
<img src="assets/IK_Gif.gif"  height=400> -->

---

## 📂 Project Structure

- `engine/` — Core rendering engine and Vulkan abstractions.
- `executables/default/` — The main implementation is inside this folder.
- `shared/` — Common utilities and shared components.
- `assets/` — Fonts, images, and other media assets.
- `submodules/` — External dependencies (e.g., Eigen).

---

## 🛠️ Build Instructions

First, ensure you have installed the Vulkan SDK and CMake (version 3.10+).

1. Clone the repository with submodules:
   ```bash
   git clone --recurse-submodules https://github.com/MohammadFakhreddin/Watermill.git

2. Create a build directory:
   ```bash
   mkdir build
   cd build
3. Configure the project with CMake:
   ```bash
   cmake ..
4. Build the project:
   ```bash
   cmake --build .