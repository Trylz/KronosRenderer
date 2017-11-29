![logo](logo.jpg?raw=true)
# Trylz Renderer

A physically based renderer with realtime DirectX 12 preview written in c++.  
Requires 64 bits Windows 10 and at least SSE4 to run.  
Trylz will automatically choose at runtime beetween SSE4 and AVX2 depending on the device.

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
Used for image processing.

 - [WxWidgets](https://www.wxwidgets.org/)  
A cross-platform GUI library.

 - [FeatureDetector](https://github.com/Mysticial/FeatureDetector)  
Determine CPU features supported.

## Utilities
- [wxFormBuilder](https://github.com/wxFormBuilder/wxFormBuilder)  
A RAD tool for wxWidgets GUI design.

## Features

### Offline renderer:  
- Multithreaded CPU Path tracer
- Bounding volume hierarchy accelerator with implementation from [Scratchapixel](https://www.scratchapixel.com/) 
- Refractive materials 
- Post-process denoising  
- Depth of field
- Area lights
- Soft shadows
- Ambient occlusion
- Image filtering(Brightness, Hue, bloom...)
- Save renders to file

### Realtime renderer:
- DirectX 12 Api

### Offline and Realtime renderer: 
- Physically based rendering
- Directionnal and omni lights  
- Cube maps 
- Transparent materials  

This is a work in progress. There is soo much to do.  


## Upcoming features:  

### Offline renderer:
- Participating medium (homogeneous and heterogeneous)
- Subsurface scattering ([Jensen & al](http://jbit.net/~sparky/bssrdf.pdf))
- Experiment bidirectionnal path tracing
- Plugins for modeling softwares.

### Realtime renderer:
- Shadow mapping
- Antiliasing (MSAA or FXAA or SMAA)
- Vulkan Api support with runtime switch
- Depth of field
- Subsurface scattering ([SSS](http://www.iryoku.com/separable-sss/))

### Offline and Realtime renderer: 
- Spotlights
- Normal mapping

### UI:
- Undo / Redo actions
- Material edition
- Lights edition(draw a representation, add gizmo manipulations)

