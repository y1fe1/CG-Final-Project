# 3DCGA Final Project

This is the Final Real-Time Demo I build using OpenGL and C++ with groupmates in TU Delft

## 1. Implemented Features

### 1.1. Minimal Features

- **Multiple Viewpoints**
- **Cube Mapping (Skybox)**
- **Normal Mapping**
- **Environment Mapping**
- **Shading**:
  - Simple shading:
    - Lambert + Blinn-Phong
    - Basic reflection model
  - Advanced shading: Textured PBR
    - Albedo, roughness, metallic, and ambient occlusion setting support to simulate better physic effect
- **Normal Mapping**
- **Material Textures**:
  - Supports Material and texture mapping from image input
  - PBR Texture support from Albedo,roughness,metallic and ambient occlusion mapping
- **Hierarchical Transformations**:
- **Smooth Paths (basic Bézier curves)**
  
### 1.2. Enriched Features:
- **Bézier Curves (extra part, moving at constant speed along a Bézier curve)**
- **HDR Skybox**
- **Image-Based Lighting**
  - Radiance Convolution Cube Mapping
  - Specular Reflection based on BRDF Lut and HDR Prefiltered Map Convolution 
- **Deferred Rendering**
- **Post Processing**

## 2. Contributor
- [Xchange7](https://github.com/Xchange7)
- [LvHTUD](https://github.com/LvHTUD)

## 3.Work Cited

### Resources

- **HDR Cubemap Texture**  
  Melas, A. (n.d.). “Free Galactic 8k HDRI.” [https://artstn.co/m/PNAR7](https://artstn.co/m/PNAR7)

- **PBR Material Textures**  
  Free PBR Materials, n.d. “Metals.” [https://freepbr.com/c/base-metals/page/5/](https://freepbr.com/c/base-metals/page/5/)

- **Celestial Body Textures and Skybox**  
  Crane, L. (n.d.). “Panorama to Cubemap.” [https://jaxry.github.io/panorama-to-cubemap/](https://jaxry.github.io/panorama-to-cubemap/)  
  NASA/Goddard Space Flight Center Scientific Visualization Studio (2020). “Deep Star Maps 2020.” [https://svs.gsfc.nasa.gov/4851](https://svs.gsfc.nasa.gov/4851)  
  Solar System Scope (n.d.). “Solar Textures.” [https://www.solarsystemscope.com/textures/](https://www.solarsystemscope.com/textures/)

### References

- **Atmospheric Scattering**  
  O’Neil, S. (2005). “Chapter 16. Accurate Atmospheric Scattering.” [https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering](https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-16-accurate-atmospheric-scattering)

- **Environment Mapping**  
  Baltacı, E.S. (2024). “Environment Mapping & HDR Lighting in C++ Using OpenGL.” [https://medium.com/@mrrsff/environment-mapping-hdr-lighting-in-c-using-opengl-c0530cb12579](https://medium.com/@mrrsff/environment-mapping-hdr-lighting-in-c-using-opengl-c0530cb12579)  
  Gordan, V. (2021). “OpenGL Tutorial 19 - Cubemaps & Skyboxes.” [https://youtu.be/8sVvxeKI9Pk](https://youtu.be/8sVvxeKI9Pk)  
  Vries, J. de (n.d.). “Cubemaps.” [https://learnopengl.com/Advanced-OpenGL/Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)

- **Parallax Mapping**  
  Vries, J. de (n.d.). “Parallax Mapping.” [https://learnopengl.com/Advanced-Lighting/Parallax-Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping)

- **PBR**  
  Vries, J. de (n.d.). “Theory.” [https://learnopengl.com/PBR/Theory](https://learnopengl.com/PBR/Theory)

- **IBL**  
  Gordan, V. (2021). “OpenGL Tutorial 29 - HDR (High-Dynamic-Range).” [https://youtu.be/1U-jym5WADU](https://youtu.be/1U-jym5WADU)  
  Vries, J. de (n.d.). “Diffuse irradiance.” [https://learnopengl.com/PBR/IBL/Diffuse-irradiance](https://learnopengl.com/PBR/IBL/Diffuse-irradiance)  
  Vries, J. de (n.d.). “Specular IBL.” [https://learnopengl.com/PBR/IBL/Specular-IBL](https://learnopengl.com/PBR/IBL/Specular-IBL)

- **Deferred Rendering**  
  Vries, J. de (n.d.). “Deferred Shading.” [https://learnopengl.com/Advanced-Lighting/Deferred-Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)

- **Post-Process**  
  Vries, J. de (n.d.). “Bloom.” [https://learnopengl.com/Advanced-Lighting/Bloom](https://learnopengl.com/Advanced-Lighting/Bloom)
