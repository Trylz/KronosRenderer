![logo](logo.jpg?raw=true)
# Trylz Renderer

A physically based renderer with realtime DirectX 12 preview written in c++.  
Requires 64 bits Windows 10 and AVX2 to run.  

General features:  
- User interface with basic settings
- Create scenes from model files and save it in xml files
- Free play camera
- Render high quality images

Trylz Renderer is a free software. To use Trylz just give it some credits.

## 3rd parties  
- [Assimp](http://assimp.sourceforge.net)  
3d model loading. Currently Trylz only support Wavefront Obj.  

- [FreeImage](http://freeimage.sourceforge.net)  
Support of various image formats.  

- [RapidXml](http://rapidxml.sourceforge.net/)  
Xml reading / writing.  
 
 - [GLM](https://glm.g-truc.net/0.9.8/index.html)  
The base math library used.  

 - [OpenCV](https://opencv.org)  
Image processing. Currently only used for denoising.

 - [WxWidgets](https://www.wxwidgets.org/)  
A cross-platform GUI library.

## Features

### Offline renderer:  
- Multithreaded CPU Path tracer
- Bounding volume hierarchy accelerator with implementation from [Scratchapixel](https://www.scratchapixel.com/) 
- Refractive materials 
- Simple post-process denoising  
- Depth of field
- Area lights
- Soft shadows
- Ambient occlusion
- Post-processing
- Save renders to file

### Realtime renderer:
- DirectX 12 Api

### Offline and Realtime renderer: 
- Physically based rendering
- Directionnal and omni lights  
- Transparent materials  

This is a work in progress. There is soo much to do.  


## Upcoming features:  

### Offline renderer:
- Participating medium (homogeneous and heterogeneous)
- Experiment bidirectionnal path tracing
- Subsurface scattering ([Jensen & al](http://jbit.net/~sparky/bssrdf.pdf))

### Realtime renderer:
- Shadow mapping
- Antiliasing (MSAA or FXAA or SMAA)
- Vulkan Api support with runtime switch
- Depth of field
- Subsurface scattering ([SSS](http://www.iryoku.com/separable-sss/))

### Offline and Realtime renderer:
- Cube maps  
- Spotlights
- Normal mapping

### UI:
- Drag and drop files
- Material edition
- Undo / Redo actions
- Create lights  
- Edit lights(draw a representation, add gizmo manipulations)
