![logo](logo.jpg?raw=true)
# Trylz Renderer

A physically based renderer with realtime DirectX 12 preview written in c++.  
Requires 64 bits Windows 10 and AVX2 to run.  

General features:  
- User interface with basic settings
- Create scenes from model files and save it in xml files
- Free play camera
- Render high quality images

## 3rd parties  
- [Assimp](http://assimp.sourceforge.net)  
3d model loading. Currently Trylz only support Wavefront Obj.  

- [FreeImage](http://freeimage.sourceforge.net)  
Support of various image formats.  

- [RapidXml](http://rapidxml.sourceforge.net)  
Xml reading / writing.  
 
 - [GLM](http://glm.g-truc.net/0.9.8/index.html)  
The base math library used.  

 - [OpenCV](http://opencv.org)  
Image processing. Currently only used for denoising.

## Features

### Offline renderer:  
- Multithreaded CPU Path tracer
- Bounding volume hierarchy accelerator with implementation from [Scratchapixel](https://www.scratchapixel.com/) 
- Refractive materials 
- Post-process denoising  
- Depth of field
- Area lights
- Soft shadows

### Realtime renderer:
- DirectX 12 Api

### Offline and Realtime renderer: 
- Physically based rendering
- Directionnal and omni lights  
- Transparent materials  

This is a work in progress. There is soo much to do.  


## Incoming features:  

### Offline renderer:
- Progress bar to show rendering state
- Save images to file
- Ambient occlusion
- Participating medium (homogeneous and heterogeneous)
- Replace path tracing by bidirectionnal path tracing
- Subsurface scattering ([Jensen & al](http://jbit.net/~sparky/bssrdf.pdf))

### Realtime renderer:
- Shadow mapping
- Antiliasing (MSAA or FXAA or SMAA)
- Vulkan Api support with runtime switch
- Depth of field
- Subsurface scattering ([SSS](http://www.iryoku.com/separable-sss/))

### Offline and Realtime renderer:
- Spotlights
- Normal mapping
- Cube maps  

### UI:
- Replace native Win32 api by a cross plateform ui library  
- Undo / Redo actions
- Create lights  
- Edit lights(draw a representation, add gizmo manipulations)