![logo](logo.jpg?raw=true)
# Trylz Renderer

A physically based renderer with realtime DirectX 12 preview, written in c++.  
General features:  
- User interface with basic settings
- Create scenes from model files and save it in xml files
- Free play camera
- Render high quality images and export them

## 3rd parties  
- [Assimp](http://assimp.sourceforge.net/)  
3d model loading. Currently Trylz only support Wavefront Obj.  

- [FreeImage](http://freeimage.sourceforge.net/)  
Support of various image formats.  

- [RapidXml](http://rapidxml.sourceforge.net/)  
Xml reading / writing.  
 
 - [GLM](http://glm.g-truc.net/0.9.8/index.html/)  
The base math library used.  

 - [OpenCV](http://docs.opencv.org/2.4/index.html/)  
Image processing. Currently only used for denoising.

## Features

### Offline renderer:  
- Multithreaded CPU Path tracer
- Bounding volume hierarchy accelerator with implementation from [Scratchapixel](https://www.scratchapixel.com/)- Post-process denoising 
- Area lights
- Soft shadows

### Realtime renderer:
- DirectX 12 Api

### Offline and Realtime renderer: 
- Physically based rendering
- Directionnal and omni lights  

This is a work in progress. There is soo much to do.  


## Incoming features:  

### Offline renderer:
- Progress bar to show rendering state
- Save images to file
- Replace Path tracing by Bidirectionnal path tracing
- Refractive materials 
- Ambient occlusion
- Subsurface scattering ([Jensen & al](http://jbit.net/~sparky/bssrdf.pdf))

### Realtime renderer:
- Shadow mapping
- Antiliasing (FXAA or SMAA)
- Vulkan Api support with runtime switch
- Subsurface scattering ([SSS](http://www.iryoku.com/separable-sss/))

### Offline and Realtime renderer:
- Spotlights
- Normal mapping
- Cube maps  

### UI:
- Undo / Redo actions
- Create lights  
- Edit lights(draw a representation, add gizmo manipulations)


